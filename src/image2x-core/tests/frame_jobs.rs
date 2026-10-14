//! Deterministic folder-job tests. Fake engines need no GPU or installed runtime.
#![cfg(unix)]

use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::Instant;

use image2x_core::frame_job::run_frame_job;
use image2x_core::protocol::FrameJob;

static NEXT: AtomicU64 = AtomicU64::new(0);

struct Fixture(PathBuf);

impl Fixture {
    fn new(script: &str) -> Self {
        let root = PathBuf::from(format!(
            "/tmp/opencode/i2/frame-test-{}-{}",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        let runtime = root.join("dependencies/distributable/ifrnet-ncnn-vulkan");
        fs::create_dir_all(runtime.join("IFRNet_Vimeo90K")).unwrap();
        fs::write(runtime.join("IFRNet_Vimeo90K/ifrnet.param"), b"model").unwrap();
        fs::create_dir_all(root.join("input")).unwrap();
        // Minimal valid 1x1 PNG, supplied to the fake engine as a known-good frame.
        let hex = "89504e470d0a1a0a0000000d4948445200000001000000010804000000b51c0c020000000b4944415478da6364f80f00010501012718e3660000000049454e44ae426082";
        let png: Vec<u8> = (0..hex.len())
            .step_by(2)
            .map(|index| u8::from_str_radix(&hex[index..index + 2], 16).unwrap())
            .collect();
        fs::write(root.join("input/1.png"), png).unwrap();
        let program = runtime.join("ifrnet-ncnn-vulkan");
        // This shell is only a deterministic OS test leaf. Production always
        // launches the selected executable with an argv array, never a shell.
        fs::write(&program, format!("#!/bin/sh\nwhile [ \"$#\" -gt 0 ]; do\n case \"$1\" in -i) input=$2; shift;; -o) output=$2; shift;; esac\n shift\ndone\n{script}\n")).unwrap();
        fs::set_permissions(program, fs::Permissions::from_mode(0o755)).unwrap();
        Self(root)
    }

    fn job(&self) -> FrameJob {
        FrameJob {
            engine: "ifrnet-ncnn-vulkan".into(),
            model: "IFRNet_Vimeo90K".into(),
            input_path: self.0.join("input").to_string_lossy().into_owned(),
            output_path: self.0.join("output").to_string_lossy().into_owned(),
            args: vec!["-f".into(), "%02d.png".into(), "-j".into(), "1:1:1".into()],
            timeout_ms: 2000,
            expected_frames: 2,
            retry_count: 0,
        }
    }
}

impl Drop for Fixture {
    fn drop(&mut self) {
        fs::remove_dir_all(&self.0).unwrap();
    }
}

const SUCCESS: &str =
    "cp \"$input/1.png\" \"$output/01.png\"; cp \"$input/1.png\" \"$output/02.png\"";

#[test]
fn succeeds_with_complete_png_sequence_and_wire_fields() {
    let fixture = Fixture::new(SUCCESS);
    let mut request = serde_json::to_value(serde_json::json!({
        "type": "process_frames", "id": "frames", "engine": "ifrnet-ncnn-vulkan",
        "model": "IFRNet_Vimeo90K", "input_path": fixture.job().input_path,
        "output_path": fixture.job().output_path, "args": fixture.job().args,
        "timeout_ms": 2000, "expected_frames": 2, "retry_count": 0
    }))
    .unwrap();
    let response = image2x_core::handle_line(&request.to_string(), &fixture.0).unwrap();
    request = serde_json::from_str(&response).unwrap();
    assert_eq!(request["event"], "process_frames");
    assert_eq!(request["data"]["output_valid"], true);
    assert_eq!(request["data"]["frame_count"], 2);
    assert_eq!(request["data"]["exit_code"], 0);
}

#[test]
fn rejects_missing_runtime_model_executable_and_input() {
    for missing in [
        "dependencies/distributable/ifrnet-ncnn-vulkan",
        "dependencies/distributable/ifrnet-ncnn-vulkan/IFRNet_Vimeo90K",
        "dependencies/distributable/ifrnet-ncnn-vulkan/ifrnet-ncnn-vulkan",
        "input",
    ] {
        let fixture = Fixture::new(SUCCESS);
        let path = fixture.0.join(missing);
        if path.is_dir() {
            fs::remove_dir_all(path).unwrap();
        } else {
            fs::remove_file(path).unwrap();
        }
        let result = run_frame_job(&fixture.0, &fixture.job());
        assert!(!result.process.started, "{missing}");
        assert!(!result.process.output_valid);
        assert!(!result.process.diagnostic.is_empty());
    }
}

#[test]
fn rejects_incomplete_empty_and_invalid_output() {
    for script in [
        "cp \"$input/1.png\" \"$output/01.png\"",
        "exit 0",
        "touch \"$output/01.png\" \"$output/02.png\"",
        "cp \"$input/1.png\" \"$output/02.png\"; cp \"$input/1.png\" \"$output/03.png\"",
    ] {
        let fixture = Fixture::new(script);
        let result = run_frame_job(&fixture.0, &fixture.job());
        assert!(result.process.finished, "{}", result.process.diagnostic);
        assert!(!result.process.output_valid);
        assert!(!result.process.diagnostic.is_empty());
        assert_eq!(fs::read_dir(fixture.0.join("output")).unwrap().count(), 0);
    }
}

#[test]
fn rejects_nonzero_and_failed_logs_even_with_complete_output() {
    for suffix in ["exit 7", "echo failed >&2"] {
        let fixture = Fixture::new(&format!("{SUCCESS}; {suffix}"));
        let result = run_frame_job(&fixture.0, &fixture.job());
        assert!(!result.process.output_valid);
        assert!(result.process.diagnostic.contains("failed"));
    }
}

#[test]
fn timeout_kills_group_including_pipe_owning_helpers() {
    let fixture = Fixture::new("sleep 30 & wait");
    let mut job = fixture.job();
    job.timeout_ms = 100;
    job.retry_count = 2;
    let start = Instant::now();
    let result = run_frame_job(&fixture.0, &job);
    assert!(result.process.timed_out);
    assert!(result.process.finished);
    assert!(start.elapsed().as_secs() < 3);
}

#[test]
fn drains_verbose_stdout_and_stderr_without_deadlock() {
    let fixture = Fixture::new(&format!(
        "head -c 1100000 /dev/zero; head -c 1100000 /dev/zero >&2; {SUCCESS}"
    ));
    let result = run_frame_job(&fixture.0, &fixture.job());
    assert!(result.process.output_valid, "{}", result.process.diagnostic);
    assert_eq!(result.process.stdout.len(), 1024 * 1024);
    assert_eq!(result.process.stderr.len(), 1024 * 1024);
}

#[test]
fn retries_with_clean_output() {
    let fixture = Fixture::new(&format!("if [ ! -f attempted ]; then touch attempted; touch \"$output/stale.png\"; exit 1; fi; {SUCCESS}"));
    let mut job = fixture.job();
    job.retry_count = 1;
    assert!(run_frame_job(&fixture.0, &job).process.output_valid);
}

#[test]
fn reserves_output_for_the_entire_child_lifetime() {
    let fixture = Fixture::new("touch running; sleep 30 & wait");
    let application = fixture.0.clone();
    let mut job = fixture.job();
    job.timeout_ms = 500;
    let worker = std::thread::spawn(move || run_frame_job(&application, &job));
    let marker = fixture
        .0
        .join("dependencies/distributable/ifrnet-ncnn-vulkan/running");
    let deadline = Instant::now() + std::time::Duration::from_secs(2);
    while !marker.exists() && Instant::now() < deadline {
        std::thread::yield_now();
    }
    assert!(marker.exists(), "fake engine must have started");
    let conflicting = run_frame_job(&fixture.0, &fixture.job());
    assert!(!conflicting.process.started);
    assert!(conflicting.process.diagnostic.contains("already used"));
    assert!(worker.join().unwrap().process.timed_out);
}

#[test]
fn canonical_request_paths_override_untrusted_path_arguments() {
    let fixture = Fixture::new(SUCCESS);
    let mut job = fixture.job();
    for flag in ["-i", "-o", "-m"] {
        job.args.extend([flag.into(), "/does/not/exist".into()]);
    }
    assert!(run_frame_job(&fixture.0, &job).process.output_valid);
}

#[test]
fn rejects_nonempty_output_and_model_traversal_without_deleting_data() {
    let fixture = Fixture::new(SUCCESS);
    let mut job = fixture.job();
    fs::create_dir_all(&job.output_path).unwrap();
    fs::write(PathBuf::from(&job.output_path).join("keep"), b"keep").unwrap();
    assert!(!run_frame_job(&fixture.0, &job).process.started);
    assert!(PathBuf::from(&job.output_path).join("keep").exists());
    job.model = "../escape".into();
    assert!(!run_frame_job(&fixture.0, &job).process.started);
}

fn frame_line(id: &str, job: &FrameJob) -> String {
    serde_json::json!({
        "type": "process_frames", "id": id, "engine": job.engine, "model": job.model,
        "input_path": job.input_path, "output_path": job.output_path, "args": job.args,
        "timeout_ms": job.timeout_ms, "expected_frames": job.expected_frames,
        "retry_count": job.retry_count,
    })
    .to_string()
}

fn cancel_response(application: &std::path::Path, target_id: &str) -> serde_json::Value {
    let response = image2x_core::handle_line(
        &serde_json::json!({"type": "cancel_frames", "id": "stop-request", "target_id": target_id})
            .to_string(),
        application,
    )
    .unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["id"], "stop-request");
    assert_eq!(response["event"], "cancel_frames");
    assert_eq!(response["ok"], true);
    response
}

#[test]
fn cancel_active_request_kills_helpers_cleans_output_and_suppresses_retries() {
    let fixture = Fixture::new(
        "echo attempt >> attempts; touch \"$output/stale.png\"; touch running; sleep 30 & wait",
    );
    let mut job = fixture.job();
    job.timeout_ms = 5000;
    job.retry_count = 3;
    let line = frame_line("cancel-active-job", &job);
    let application = fixture.0.clone();
    // These concurrent handlers are the same boundary used by independently
    // threaded sockets: cancellation does not wait on the frame connection.
    let worker =
        std::thread::spawn(move || image2x_core::handle_line(&line, &application).unwrap());
    let runtime = fixture
        .0
        .join("dependencies/distributable/ifrnet-ncnn-vulkan");
    let deadline = Instant::now() + std::time::Duration::from_secs(2);
    while !runtime.join("running").exists() && Instant::now() < deadline {
        std::thread::yield_now();
    }
    assert!(runtime.join("running").exists());
    let start = Instant::now();
    assert_eq!(
        cancel_response(&fixture.0, "cancel-active-job")["data"]["cancelled"],
        true
    );
    let response: serde_json::Value = serde_json::from_str(&worker.join().unwrap()).unwrap();
    assert!(start.elapsed() < std::time::Duration::from_secs(1));
    assert_eq!(response["event"], "process_frames");
    assert_eq!(response["data"]["finished"], true);
    assert_eq!(response["data"]["output_valid"], false);
    assert_eq!(response["data"]["timed_out"], false);
    assert!(response["data"]["diagnostic"]
        .as_str()
        .unwrap()
        .contains("cancelled"));
    assert_eq!(
        fs::read_to_string(runtime.join("attempts")).unwrap(),
        "attempt\n"
    );
    assert_eq!(fs::read_dir(&job.output_path).unwrap().count(), 0);
    assert_eq!(
        cancel_response(&fixture.0, "cancel-active-job")["data"]["cancelled"],
        false
    );

    // The completed cancellation cannot leak into a later request reusing ID.
    let next = Fixture::new(SUCCESS);
    let response =
        image2x_core::handle_line(&frame_line("cancel-active-job", &next.job()), &next.0).unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["data"]["output_valid"], true);
}

#[test]
fn unknown_completed_and_validation_failed_requests_are_not_cancellable() {
    let fixture = Fixture::new(SUCCESS);
    assert_eq!(
        cancel_response(&fixture.0, "unknown-frame-id")["data"]["cancelled"],
        false
    );
    let response = image2x_core::handle_line(
        &frame_line("completed-frame-id", &fixture.job()),
        &fixture.0,
    )
    .unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["data"]["output_valid"], true);
    assert_eq!(
        cancel_response(&fixture.0, "completed-frame-id")["data"]["cancelled"],
        false
    );

    let mut invalid = fixture.job();
    invalid.model = "missing-model".into();
    image2x_core::handle_line(&frame_line("invalid-frame-id", &invalid), &fixture.0).unwrap();
    assert_eq!(
        cancel_response(&fixture.0, "invalid-frame-id")["data"]["cancelled"],
        false
    );
}

#[test]
fn separate_socket_cancels_a_blocked_frame_connection() {
    use std::io::{BufRead, BufReader, Write};
    use std::os::unix::net::UnixStream;
    use std::process::{Child, Command};
    use std::time::Duration;

    struct Server(Child);
    impl Drop for Server {
        fn drop(&mut self) {
            let _ = self.0.kill();
            let _ = self.0.wait();
        }
    }
    let fixture = Fixture::new("touch \"$output/partial.png\"; touch running; sleep 2 & wait");
    let socket = fixture.0.join("core.socket");
    let _server = Server(
        Command::new(env!("CARGO_BIN_EXE_image2x-core"))
            .arg("--socket")
            .arg(&socket)
            .arg("--application-directory")
            .arg(&fixture.0)
            .spawn()
            .unwrap(),
    );
    let deadline = Instant::now() + Duration::from_secs(2);
    let mut frame_socket = loop {
        if let Ok(stream) = UnixStream::connect(&socket) {
            break stream;
        }
        assert!(
            Instant::now() < deadline,
            "backend socket must become ready"
        );
        std::thread::yield_now();
    };
    frame_socket
        .set_read_timeout(Some(Duration::from_secs(2)))
        .unwrap();
    let job = fixture.job();
    writeln!(frame_socket, "{}", frame_line("socket-frame-job", &job)).unwrap();
    let marker = fixture
        .0
        .join("dependencies/distributable/ifrnet-ncnn-vulkan/running");
    while !marker.exists() && Instant::now() < deadline {
        std::thread::yield_now();
    }
    assert!(marker.exists(), "frame socket must be executing its child");
    let mut cancel_socket = UnixStream::connect(&socket).unwrap();
    cancel_socket
        .set_read_timeout(Some(Duration::from_secs(1)))
        .unwrap();
    writeln!(cancel_socket, "{}", serde_json::json!({"type": "cancel_frames", "id": "socket-stop", "target_id": "socket-frame-job"})).unwrap();
    let mut line = String::new();
    BufReader::new(cancel_socket).read_line(&mut line).unwrap();
    let cancelled: serde_json::Value = serde_json::from_str(&line).unwrap();
    assert_eq!(cancelled["event"], "cancel_frames");
    assert_eq!(cancelled["data"]["cancelled"], true);
    line.clear();
    BufReader::new(frame_socket).read_line(&mut line).unwrap();
    let result: serde_json::Value = serde_json::from_str(&line).unwrap();
    assert_eq!(result["event"], "process_frames");
    assert_eq!(result["data"]["output_valid"], false);
    assert!(result["data"]["diagnostic"]
        .as_str()
        .unwrap()
        .contains("cancelled"));
    assert_eq!(fs::read_dir(&job.output_path).unwrap().count(), 0);
}
