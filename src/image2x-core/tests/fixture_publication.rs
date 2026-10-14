//! Deterministically reproduce and prevent inherited-writer ETXTBSY races.
#![cfg(target_os = "linux")]

#[path = "support/fake_engine.rs"]
mod fake_engine;

use std::fs::{self, File, OpenOptions};
use std::io::{self, Read, Write};
use std::os::fd::AsRawFd;
use std::os::unix::fs::OpenOptionsExt;
use std::os::unix::net::UnixStream;
use std::path::PathBuf;
use std::time::Duration;

use image2x_core::process::run_engine_test;

extern "C" {
    fn fork() -> i32;
    fn read(fd: i32, buffer: *mut u8, count: usize) -> isize;
    fn write(fd: i32, buffer: *const u8, count: usize) -> isize;
    fn waitpid(pid: i32, status: *mut i32, options: i32) -> i32;
    fn kill(pid: i32, signal: i32) -> i32;
    fn _exit(status: i32) -> !;
}

struct InheritedWriter {
    pid: i32,
    socket: UnixStream,
}

impl InheritedWriter {
    fn hold(writer: File) -> io::Result<Self> {
        let (socket, child_socket) = UnixStream::pair()?;
        socket.set_read_timeout(Some(Duration::from_secs(2)))?;
        let fd = child_socket.as_raw_fd();
        // fork copies descriptors, including CLOEXEC writers. The child pauses
        // at the same boundary as an unrelated Command child before exec closes
        // those descriptors. Only async-signal-safe C calls run in the child:
        // taking Rust locks or allocating there could deadlock a threaded test.
        // SAFETY: fork has no arguments; child code below only uses live copied
        // descriptors, stack buffers, and read/write/_exit, never Rust destructors.
        let pid = unsafe { fork() };
        if pid < 0 {
            return Err(io::Error::last_os_error());
        }
        if pid == 0 {
            let ready = b'R';
            let mut release = 0;
            // SAFETY: fd is a live socket; both byte pointers reference valid
            // stack storage. _exit deliberately skips inherited Rust state.
            unsafe {
                if write(fd, &ready, 1) != 1 {
                    _exit(2);
                }
                read(fd, &mut release, 1);
                _exit(0);
            }
        }
        drop(child_socket);
        drop(writer); // Parent is closed; the forked child alone retains it.
        let mut holder = Self { pid, socket };
        let mut ready = [0];
        holder.socket.read_exact(&mut ready)?;
        if ready != *b"R" {
            return Err(io::Error::other("inherited-writer child was not ready"));
        }
        Ok(holder)
    }
}

impl Drop for InheritedWriter {
    fn drop(&mut self) {
        // Release/reap even on assertion failure. The child owns no process
        // helpers and immediately exits after the release byte. Kill is only
        // for a broken control socket, not a retry or a timing workaround.
        if self.socket.write_all(b"X").is_err() {
            // SAFETY: pid identifies our live, unreaped child; SIGKILL is integer 9.
            unsafe {
                kill(self.pid, 9);
            }
        }
        let mut status = 0;
        loop {
            // SAFETY: status is writable storage, and pid is our child ID.
            if unsafe { waitpid(self.pid, &mut status, 0) } >= 0 {
                break;
            }
            if io::Error::last_os_error().kind() != io::ErrorKind::Interrupted {
                break;
            }
        }
    }
}

struct Directory(PathBuf);
impl Drop for Directory {
    fn drop(&mut self) {
        fs::remove_dir_all(&self.0).unwrap();
    }
}

#[test]
fn inherited_closed_parent_writer_blocks_legacy_exec_but_not_immutable_fixture() {
    let directory = Directory(PathBuf::from(format!(
        "/tmp/opencode/i2/fixture-publication-{}",
        std::process::id()
    )));
    fs::create_dir_all(&directory.0).unwrap();
    let behavior = "printf '[0 Synthetic Hardware] queueC=0\\n'; printf output > result.png\n";
    let legacy = directory.0.join("legacy-engine");
    let mut writer = OpenOptions::new()
        .create_new(true)
        .write(true)
        .mode(0o755)
        .open(&legacy)
        .unwrap();
    writer
        .write_all(format!("#!/bin/sh\n{behavior}").as_bytes())
        .unwrap();
    let output = directory.0.join("result.png");
    let probe = |program: &std::path::Path| {
        run_engine_test(
            program.to_str().unwrap(),
            &[],
            &directory.0,
            1000,
            &output,
            "hardware_gpu",
        )
    };

    let inherited = InheritedWriter::hold(writer).unwrap();
    let blocked = probe(&legacy);
    assert!(
        !blocked.started,
        "legacy writer must prevent exec: {blocked:?}"
    );
    assert!(blocked.diagnostic.contains("os error 26"), "{blocked:?}");
    drop(inherited);
    let released = probe(&legacy);
    assert!(
        released.passed,
        "same inode must work after writer release: {released:?}"
    );

    let immutable = directory.0.join("immutable-engine");
    fake_engine::install(&immutable, behavior).unwrap();
    // Deliberately retain a write descriptor for the fixture's variable content
    // across fork. With the old executable publication this was ETXTBSY; with
    // the fixed layout it is ordinary shell data and cannot block kernel exec.
    let behavior_writer = OpenOptions::new()
        .write(true)
        .open(directory.0.join("fake-engine-behavior"))
        .unwrap();
    let _inherited = InheritedWriter::hold(behavior_writer).unwrap();
    let result = probe(&immutable);
    assert!(result.started, "immutable fixture must start: {result:?}");
    assert!(
        result.passed,
        "immutable fixture must finish successfully: {result:?}"
    );
}
