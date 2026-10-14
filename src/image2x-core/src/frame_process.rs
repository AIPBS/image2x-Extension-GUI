//! Bounded frame/probe supervision with continuously drained output pipes.

use std::io::{self, Read};
use std::path::Path;
use std::process::{Command, Stdio};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;
use std::time::{Duration, Instant};

use crate::frame_cancel::Cancellation;
use crate::process::ProcessResult;

#[cfg(unix)]
fn nonblocking(pipe: &impl std::os::fd::AsRawFd) -> io::Result<()> {
    extern "C" {
        fn fcntl(fd: i32, command: i32, ...) -> i32;
    }
    #[cfg(target_os = "linux")]
    const NONBLOCK: i32 = 0x800;
    #[cfg(not(target_os = "linux"))]
    const NONBLOCK: i32 = 0x4;
    // Reader threads must not wait forever if a helper escapes the process
    // group while retaining a pipe writer. O_NONBLOCK makes an empty pipe
    // return WouldBlock, so the reader can enforce its own deadline. Existing
    // descriptor flags are retained; the child owns the separate write end.
    // SAFETY: pipe owns a live descriptor; F_GETFL/F_SETFL use integer arguments.
    let flags = unsafe { fcntl(pipe.as_raw_fd(), 3) };
    if flags < 0 || unsafe { fcntl(pipe.as_raw_fd(), 4, flags | NONBLOCK) } < 0 {
        return Err(io::Error::last_os_error());
    }
    Ok(())
}

pub(crate) fn failure(message: impl Into<String>) -> ProcessResult {
    ProcessResult {
        started: false,
        finished: false,
        exit_code: None,
        timed_out: false,
        output_valid: false,
        stdout: String::new(),
        stderr: String::new(),
        diagnostic: message.into(),
    }
}

// A reader thread drains each kernel pipe while the engine runs. Keeping only
// the first MiB bounds memory, but continuing to read prevents a full pipe from
// blocking a verbose engine. The failure marker is detected even after the cap.
fn drain(
    mut pipe: impl Read,
    deadline: Instant,
    stopped: &AtomicBool,
) -> io::Result<(String, bool)> {
    let mut captured = Vec::new();
    let mut buffer = [0; 8192];
    let mut tail = String::new();
    let mut failed = false;
    let mut cleanup_deadline = None;
    loop {
        // When Stop kills the process group, pipe joins must use the short
        // cleanup allowance, not wait out the original (possibly hours) budget.
        if stopped.load(Ordering::Acquire) && cleanup_deadline.is_none() {
            cleanup_deadline = Instant::now().checked_add(Duration::from_millis(200));
        }
        if Instant::now() >= cleanup_deadline.map_or(deadline, |cleanup| deadline.min(cleanup)) {
            return Err(io::Error::new(
                io::ErrorKind::TimedOut,
                "pipe drain deadline exceeded",
            ));
        }
        let count = match pipe.read(&mut buffer) {
            Ok(count) => count,
            Err(error) if error.kind() == io::ErrorKind::Interrupted => continue,
            Err(error) if error.kind() == io::ErrorKind::WouldBlock => {
                thread::sleep(Duration::from_millis(10));
                continue;
            }
            Err(error) => return Err(error),
        };
        if count == 0 {
            break;
        }
        let text = String::from_utf8_lossy(&buffer[..count]).to_ascii_lowercase();
        failed |= format!("{tail}{text}").contains("failed");
        tail = text
            .chars()
            .rev()
            .take(5)
            .collect::<String>()
            .chars()
            .rev()
            .collect();
        let remaining = (1024 * 1024_usize).saturating_sub(captured.len());
        captured.extend_from_slice(&buffer[..count.min(remaining)]);
    }
    Ok((String::from_utf8_lossy(&captured).into_owned(), failed))
}

#[cfg(unix)]
fn kill_group(id: u32) {
    extern "C" {
        fn kill(pid: i32, signal: i32) -> i32;
    }
    // The negative PID addresses the child's private process group, not ours.
    // SIGKILL also closes inherited pipe writers in descendants, allowing the
    // drain threads to finish. Descendants that deliberately leave the group
    // are outside this supervision contract.
    // SAFETY: kill accepts integer IDs/signals; no pointers or memory are used.
    unsafe { kill(-(id as i32), 9) };
}

pub(crate) fn run(
    program: &Path,
    args: &[String],
    cwd: &Path,
    deadline: Instant,
    cancellation: &Cancellation,
) -> ProcessResult {
    run_with_environment(program, args, cwd, deadline, cancellation, &[])
}

pub(crate) fn run_with_environment(
    program: &Path,
    args: &[String],
    cwd: &Path,
    deadline: Instant,
    cancellation: &Cancellation,
    environment: &[(String, String)],
) -> ProcessResult {
    if cancellation.is_cancelled() {
        return failure("frame job cancelled");
    }
    let mut command = Command::new(program);
    command
        .args(args)
        .current_dir(cwd)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    command.envs(environment.iter().map(|(name, value)| (name, value)));
    #[cfg(unix)]
    {
        use std::os::unix::process::CommandExt;
        // The OS assigns a new group before exec, so deadline cleanup cannot
        // accidentally signal the backend or another concurrent job.
        command.process_group(0);
    }
    let mut child = match command.spawn() {
        Ok(child) => child,
        Err(error) => return failure(error.to_string()),
    };
    // The small cleanup allowance lets killed pipe writers close before the
    // readers stop. It is not another engine attempt or an unbounded pipe join.
    let pipe_deadline = deadline
        .checked_add(Duration::from_millis(200))
        .unwrap_or(deadline);
    let stopped = Arc::new(AtomicBool::new(false));
    let stdout_stopped = Arc::clone(&stopped);
    let stderr_stopped = Arc::clone(&stopped);
    let stdout = child.stdout.take().map(|pipe| {
        thread::Builder::new().spawn(move || {
            #[cfg(unix)]
            nonblocking(&pipe)?;
            drain(pipe, pipe_deadline, &stdout_stopped)
        })
    });
    let stderr = child.stderr.take().map(|pipe| {
        thread::Builder::new().spawn(move || {
            #[cfg(unix)]
            nonblocking(&pipe)?;
            drain(pipe, pipe_deadline, &stderr_stopped)
        })
    });
    let readers = stdout
        .transpose()
        .and_then(|stdout| stderr.transpose().map(|stderr| (stdout, stderr)));
    let (stdout, stderr) = match readers {
        Ok(readers) => readers,
        Err(error) => {
            stopped.store(true, Ordering::Release);
            // OS thread creation can fail under resource pressure. Do not leave
            // a newly started engine behind when its output cannot be drained.
            #[cfg(unix)]
            kill_group(child.id());
            let _ = child.kill();
            let status = child.wait();
            let mut result = failure(format!("pipe reader startup failed: {error}"));
            result.started = true;
            result.finished = status.is_ok();
            result.exit_code = status.ok().and_then(|status| status.code());
            return result;
        }
    };
    let mut timed_out = false;
    let mut diagnostic = String::new();
    let status = loop {
        if cancellation.is_cancelled() {
            diagnostic = "frame job cancelled".into();
            #[cfg(unix)]
            kill_group(child.id());
            let _ = child.kill();
            break child.wait().ok();
        }
        match child.try_wait() {
            Ok(Some(status)) => break Some(status),
            Ok(None) if Instant::now() < deadline => thread::sleep(Duration::from_millis(20)),
            result => {
                timed_out = matches!(result, Ok(None));
                diagnostic = if timed_out {
                    "process exceeded its deadline".to_owned()
                } else {
                    result
                        .err()
                        .map(|error| error.to_string())
                        .unwrap_or_default()
                };
                #[cfg(unix)]
                kill_group(child.id());
                let _ = child.kill();
                break child.wait().ok();
            }
        }
    };
    // Even a normally exiting engine may leave pipe-owning helpers behind.
    #[cfg(unix)]
    kill_group(child.id());
    stopped.store(true, Ordering::Release);
    let mut join_reader =
        |reader: Option<thread::JoinHandle<io::Result<(String, bool)>>>| match reader
            .map(|reader| reader.join())
        {
            Some(Ok(Ok(output))) => output,
            Some(Ok(Err(error))) => {
                diagnostic.push_str(&format!("; pipe read failed: {error}"));
                (String::new(), true)
            }
            _ => {
                diagnostic.push_str("; pipe reader failed");
                (String::new(), true)
            }
        };
    let (stdout, stdout_failed) = join_reader(stdout);
    let (stderr, stderr_failed) = join_reader(stderr);
    let exit_code = status.and_then(|status| status.code());
    if diagnostic.is_empty() && (exit_code != Some(0) || stdout_failed || stderr_failed) {
        diagnostic = format!("process failed (exit code {exit_code:?})");
    }
    ProcessResult {
        started: true,
        finished: status.is_some(),
        exit_code,
        timed_out,
        output_valid: false,
        stdout,
        stderr,
        diagnostic,
    }
}
