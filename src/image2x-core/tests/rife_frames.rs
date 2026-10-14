//! Fake RIFE runtimes exercise the shared frame API without installed engines.
#![cfg(unix)]

#[path = "support/fake_engine.rs"]
mod fake_engine;

use std::fs;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use image2x_core::frame_job::run_frame_job;
use image2x_core::protocol::FrameJob;
use image2x_core::runtime::runtime_records;

static NEXT: AtomicU64 = AtomicU64::new(0);
const ENGINE: &str = "rife-ncnn-vulkan";
const SUCCESS: &str =
    "cp \"$input/1.png\" \"$output/001.png\"; cp \"$input/1.png\" \"$output/002.png\"";

struct Fixture(PathBuf);

impl Fixture {
    fn new(script: &str) -> Self {
        let root = PathBuf::from(format!(
            "/tmp/opencode/i2/rife frames-{}-{}",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        let fixture = Self(root);
        fs::create_dir_all(fixture.runtime()).unwrap();
        fs::create_dir_all(fixture.0.join("input")).unwrap();
        let hex = "89504e470d0a1a0a0000000d4948445200000001000000010804000000b51c0c020000000b4944415478da6364f80f00010501012718e3660000000049454e44ae426082";
        let png: Vec<u8> = (0..hex.len())
            .step_by(2)
            .map(|index| u8::from_str_radix(&hex[index..index + 2], 16).unwrap())
            .collect();
        fs::write(fixture.0.join("input/1.png"), png).unwrap();
        fixture.program(script);
        fixture
    }

    fn runtime(&self) -> PathBuf {
        self.0.join("dependencies/distributable").join(ENGINE)
    }

    fn model(&self, name: &str) {
        let folder = self.runtime().join(name);
        fs::create_dir_all(&folder).unwrap();
        let networks: &[&str] = if name.starts_with("rife-v4") {
            &["flownet"]
        } else {
            &["flownet", "contextnet", "fusionnet"]
        };
        for network in networks {
            for extension in ["param", "bin"] {
                fs::write(folder.join(format!("{network}.{extension}")), b"fake model").unwrap();
            }
        }
    }

    fn program(&self, script: &str) {
        let path = self.runtime().join(ENGINE);
        // A deterministic shell test leaf records the actual argv received by
        // the fake runtime. Production launches engines directly, not a shell.
        fake_engine::install(&path, &format!("#!/bin/sh\nprintf '%s\\n' \"$@\" > argv\nwhile [ \"$#\" -gt 0 ]; do\n case \"$1\" in -i) input=$2; shift;; -o) output=$2; shift;; -m) model=$2; shift;; esac\n shift\ndone\n{script}\n")).unwrap();
    }

    fn job(&self, model: &str) -> FrameJob {
        FrameJob {
            engine: ENGINE.into(),
            model: model.into(),
            input_path: self.0.join("input").to_string_lossy().into_owned(),
            output_path: self.0.join("output").to_string_lossy().into_owned(),
            args: vec!["-f".into(), "%03d.png".into()],
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

fn request(id: &str, job: &FrameJob) -> String {
    serde_json::json!({"type": "process_frames", "id": id, "engine": job.engine,
        "model": job.model, "input_path": job.input_path, "output_path": job.output_path,
        "args": job.args, "timeout_ms": job.timeout_ms, "expected_frames": job.expected_frames,
        "retry_count": job.retry_count})
    .to_string()
}

#[test]
fn selected_v4_model_options_and_canonical_paths_reach_runtime() {
    let fixture = Fixture::new(SUCCESS);
    fixture.model("rife-v4.26-large");
    fixture.model("rife-v4.6");
    let mut job = fixture.job("rife-v4.26-large");
    job.args.extend(
        [
            "-x",
            "-u",
            "-g",
            "0,1",
            "-j",
            "4:2,2:4",
            "-i",
            "/wrong/input",
            "-o",
            "/wrong/output",
            "-m",
            "/wrong/model",
        ]
        .map(str::to_owned),
    );
    let line = image2x_core::handle_line(&request("rife-options", &job), &fixture.0).unwrap();
    let response: serde_json::Value = serde_json::from_str(&line).unwrap();
    assert_eq!(response["event"], "process_frames");
    assert_eq!(response["data"]["output_valid"], true);
    assert_eq!(response["data"]["frame_count"], 2);
    let argv = fs::read_to_string(fixture.runtime().join("argv")).unwrap();
    let expected = [
        "-x",
        "-u",
        "-g",
        "0,1",
        "-j",
        "4:2,2:4",
        "-i",
        &job.input_path,
        "-o",
        &job.output_path,
        "-m",
        fixture.runtime().join(&job.model).to_str().unwrap(),
        "-f",
        "%03d.png",
    ]
    .join("\n")
        + "\n";
    assert_eq!(argv, expected);
}

#[test]
fn old_models_work_without_the_inventory_sentinel_and_reject_incomplete_pairs() {
    for model in ["rife", "rife-anime", "rife-v2.4", "rife-v3.1"] {
        let fixture = Fixture::new(SUCCESS);
        fixture.model(model);
        let inventory = runtime_records(&fixture.0)
            .into_iter()
            .find(|record| record.engine == ENGINE)
            .unwrap();
        assert!(!inventory.available);
        assert_eq!(inventory.missing, [fixture.runtime().join("rife-v4.6")]);
        let result = run_frame_job(&fixture.0, &fixture.job(model));
        assert!(result.process.output_valid, "{}", result.process.diagnostic);
    }
    for file in [
        "flownet.param",
        "flownet.bin",
        "contextnet.param",
        "contextnet.bin",
        "fusionnet.param",
        "fusionnet.bin",
    ] {
        let fixture = Fixture::new(SUCCESS);
        fixture.model("rife-v2.4");
        fs::remove_file(fixture.runtime().join("rife-v2.4").join(file)).unwrap();
        let result = run_frame_job(&fixture.0, &fixture.job("rife-v2.4"));
        assert!(!result.process.started);
        assert!(result.process.diagnostic.contains(file));
    }
}

#[test]
fn missing_empty_and_escaped_v4_models_are_rejected_without_fallback() {
    let fixture = Fixture::new(SUCCESS);
    fixture.model("rife-v4.6");
    let result = run_frame_job(&fixture.0, &fixture.job("rife-v4.26"));
    assert!(!result.process.started);
    assert!(result.process.diagnostic.contains("model"));
    for file in ["flownet.param", "flownet.bin"] {
        fixture.model("rife-v4.26");
        fs::remove_file(fixture.runtime().join("rife-v4.26").join(file)).unwrap();
        assert!(
            !run_frame_job(&fixture.0, &fixture.job("rife-v4.26"))
                .process
                .started
        );
        fixture.model("rife-v4.26");
        fs::write(fixture.runtime().join("rife-v4.26").join(file), b"").unwrap();
        assert!(
            !run_frame_job(&fixture.0, &fixture.job("rife-v4.26"))
                .process
                .started
        );
    }
    fixture.model("rife-v4.26");
    let file = fixture.runtime().join("rife-v4.26/flownet.bin");
    fs::remove_file(&file).unwrap();
    std::os::unix::fs::symlink(fixture.0.join("input/1.png"), file).unwrap();
    assert!(
        !run_frame_job(&fixture.0, &fixture.job("rife-v4.26"))
            .process
            .started
    );
}

#[test]
fn inventory_uses_the_existing_rife_v4_6_sentinel() {
    let fixture = Fixture::new(SUCCESS);
    fixture.model("rife-v4.6");
    let record = runtime_records(&fixture.0)
        .into_iter()
        .find(|record| record.engine == ENGINE)
        .unwrap();
    assert!(record.available);
    assert_eq!(record.executable, fixture.runtime().join(ENGINE));
    fs::remove_file(fixture.runtime().join(ENGINE)).unwrap();
    let result = run_frame_job(&fixture.0, &fixture.job("rife-v4.6"));
    assert!(!result.process.started);
    assert!(result.process.diagnostic.contains("executable"));
}

#[test]
fn rife_retries_incomplete_output_but_does_not_hide_engine_failures() {
    let fixture = Fixture::new(&format!("if [ ! -f attempted ]; then touch attempted; touch \"$output/stale.png\"; exit 0; fi; {SUCCESS}"));
    fixture.model("rife-v4.26");
    let mut job = fixture.job("rife-v4.26");
    job.retry_count = 1;
    assert!(run_frame_job(&fixture.0, &job).process.output_valid);
    for suffix in [
        "echo 'MemoryData layer failed' >&2; exit 1",
        "echo failed >&2",
    ] {
        let fixture = Fixture::new(&format!("{SUCCESS}; {suffix}"));
        fixture.model("rife-v4.26");
        let result = run_frame_job(&fixture.0, &fixture.job("rife-v4.26"));
        assert!(!result.process.output_valid);
        assert!(result.process.diagnostic.contains("failed"));
        assert_eq!(fs::read_dir(fixture.0.join("output")).unwrap().count(), 0);
    }
}

#[test]
fn rife_cancel_stops_active_pass_and_cleans_registry_and_output() {
    let fixture = Fixture::new(
        "echo attempt >> attempts; touch \"$output/partial.png\"; touch running; sleep 30 & wait",
    );
    fixture.model("rife-v4.26");
    let mut job = fixture.job("rife-v4.26");
    job.timeout_ms = 5000;
    job.retry_count = 3;
    let application = fixture.0.clone();
    let line = request("cancel-rife-job", &job);
    let worker =
        std::thread::spawn(move || image2x_core::handle_line(&line, &application).unwrap());
    let deadline = Instant::now() + Duration::from_secs(2);
    while !fixture.runtime().join("running").exists() && Instant::now() < deadline {
        std::thread::yield_now();
    }
    assert!(fixture.runtime().join("running").exists());
    let cancel = r#"{"type":"cancel_frames","id":"stop-rife","target_id":"cancel-rife-job"}"#;
    let start = Instant::now();
    let response: serde_json::Value =
        serde_json::from_str(&image2x_core::handle_line(cancel, &fixture.0).unwrap()).unwrap();
    assert_eq!(response["event"], "cancel_frames");
    assert_eq!(response["data"]["cancelled"], true);
    let result: serde_json::Value = serde_json::from_str(&worker.join().unwrap()).unwrap();
    assert!(start.elapsed() < Duration::from_secs(1));
    assert_eq!(result["data"]["output_valid"], false);
    assert_eq!(result["data"]["timed_out"], false);
    assert!(result["data"]["diagnostic"]
        .as_str()
        .unwrap()
        .contains("cancelled"));
    assert_eq!(
        fs::read_to_string(fixture.runtime().join("attempts")).unwrap(),
        "attempt\n"
    );
    assert_eq!(fs::read_dir(&job.output_path).unwrap().count(), 0);
    let response: serde_json::Value =
        serde_json::from_str(&image2x_core::handle_line(cancel, &fixture.0).unwrap()).unwrap();
    assert_eq!(response["data"]["cancelled"], false);
}

#[test]
fn rife_timeout_budget_is_shared_across_retries() {
    let fixture = Fixture::new("echo attempt >> attempts; sleep 30 & wait");
    fixture.model("rife-v4.26");
    let mut job = fixture.job("rife-v4.26");
    job.timeout_ms = 100;
    job.retry_count = 3;
    let start = Instant::now();
    let result = run_frame_job(&fixture.0, &job);
    assert!(result.process.timed_out);
    assert!(!result.process.output_valid);
    assert!(start.elapsed() < Duration::from_secs(1));
    assert_eq!(
        fs::read_to_string(fixture.runtime().join("attempts")).unwrap(),
        "attempt\n"
    );
}

#[test]
fn runtime_root_override_is_used_without_mutating_test_process_environment() {
    use std::io::{BufRead, BufReader, Write};
    use std::os::unix::net::UnixStream;
    use std::process::{Child, Command};
    struct Server(Child);
    impl Drop for Server {
        fn drop(&mut self) {
            let _ = self.0.kill();
            let _ = self.0.wait();
        }
    }
    let fixture = Fixture::new(SUCCESS);
    fixture.model("rife-v4.26");
    let socket = fixture.0.join("core.socket");
    let _server = Server(
        Command::new(env!("CARGO_BIN_EXE_image2x-core"))
            .args([
                "--socket",
                socket.to_str().unwrap(),
                "--application-directory",
                "/application/not/installed",
            ])
            .env(
                "IMAGE2X_RUNTIME_DIRECTORY",
                fixture.runtime().parent().unwrap(),
            )
            .spawn()
            .unwrap(),
    );
    let deadline = Instant::now() + Duration::from_secs(2);
    let mut stream = loop {
        if let Ok(stream) = UnixStream::connect(&socket) {
            break stream;
        }
        assert!(Instant::now() < deadline);
        std::thread::yield_now();
    };
    stream
        .set_read_timeout(Some(Duration::from_secs(2)))
        .unwrap();
    writeln!(
        stream,
        "{}",
        request("rife-runtime-override", &fixture.job("rife-v4.26"))
    )
    .unwrap();
    let mut response = String::new();
    BufReader::new(stream).read_line(&mut response).unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["data"]["output_valid"], true);
    assert!(Path::new(&fixture.0.join("output/001.png")).is_file());
}
