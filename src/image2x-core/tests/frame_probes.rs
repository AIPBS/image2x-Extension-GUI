//! Pair-input probe and discovery tests. All engines/devices are synthetic.
#![cfg(unix)]

use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use image2x_core::process::run_engine_test;
use image2x_core::runtime::runtime_records;

static NEXT: AtomicU64 = AtomicU64::new(0);
const DEVICES: &str = "printf '[0 Hardware A] queueC=0\\n[1 llvmpipe] queueC=0\\n'; printf '[2 Hardware B]  queueC=0[1] queueG=0\\n' >&2";

struct Fixture {
    root: PathBuf,
    engine: &'static str,
    model: &'static str,
}

impl Fixture {
    fn new(engine: &'static str, model: &'static str, suffix: &str) -> Self {
        let root = PathBuf::from(format!(
            "/tmp/opencode/i2/pair probe-{}-{}",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        let fixture = Self {
            root,
            engine,
            model,
        };
        fs::create_dir_all(fixture.runtime().join(model)).unwrap();
        // Two distinct, valid 1x1 input PNG paths satisfy the pair-input API.
        let hex = "89504e470d0a1a0a0000000d4948445200000001000000010804000000b51c0c020000000b4944415478da6364f80f00010501012718e3660000000049454e44ae426082";
        let png: Vec<u8> = (0..hex.len())
            .step_by(2)
            .map(|index| u8::from_str_radix(&hex[index..index + 2], 16).unwrap())
            .collect();
        for name in ["first.png", "second.png"] {
            fs::write(fixture.root.join(name), &png).unwrap();
        }
        let script = format!("#!/bin/sh\nprintf '%s\\n' \"$@\" > argv\nwhile [ \"$#\" -gt 0 ]; do\n case \"$1\" in -0) first=$2; shift;; -1) second=$2; shift;; -o) output=$2; shift;; -m) model=$2; shift;; esac\n shift\ndone\n[ \"$first\" != \"$second\" ] && [ -f \"$first\" ] && [ -f \"$second\" ] && [ -d \"$model\" ] || exit 4\n{DEVICES}\n{suffix}\n");
        fs::write(fixture.program(), script).unwrap();
        fs::set_permissions(fixture.program(), fs::Permissions::from_mode(0o755)).unwrap();
        fixture
    }

    fn runtime(&self) -> PathBuf {
        self.root
            .join("dependencies/distributable")
            .join(self.engine)
    }
    fn program(&self) -> PathBuf {
        self.runtime().join(self.engine)
    }
    fn output(&self) -> PathBuf {
        self.root.join("probe.png")
    }

    fn args(&self, id: &str) -> Vec<String> {
        let mut args = vec![
            "-0".into(),
            self.root.join("first.png").to_string_lossy().into_owned(),
            "-1".into(),
            self.root.join("second.png").to_string_lossy().into_owned(),
            "-o".into(),
            self.output().to_string_lossy().into_owned(),
            "-j".into(),
            "1:1:1".into(),
            "-g".into(),
            id.into(),
            "-m".into(),
            self.runtime()
                .join(self.model)
                .to_string_lossy()
                .into_owned(),
        ];
        if self.engine == "dain-ncnn-vulkan" {
            args.extend(["-t".into(), "128".into()]);
        }
        args
    }

    fn probe(&self, id: &str, device: &str) -> image2x_core::process::EngineTestResult {
        run_engine_test(
            self.program().to_str().unwrap(),
            &self.args(id),
            &self.runtime(),
            2000,
            &self.output(),
            device,
        )
    }
}

impl Drop for Fixture {
    fn drop(&mut self) {
        fs::remove_dir_all(&self.root).unwrap();
    }
}

#[test]
fn cain_dain_inventory_reports_directory_executable_and_model_availability_only() {
    for (engine, model) in [("cain-ncnn-vulkan", "cain"), ("dain-ncnn-vulkan", "best")] {
        let fixture = Fixture::new(engine, model, "cp \"$first\" \"$output\"");
        let record = || {
            runtime_records(&fixture.root)
                .into_iter()
                .find(|record| record.engine == engine)
                .unwrap()
        };
        assert!(record().available);
        assert_eq!(record().executable, fixture.program());
        fs::set_permissions(fixture.program(), fs::Permissions::from_mode(0o644)).unwrap();
        assert_eq!(record().missing, [fixture.program()]);
        fs::remove_file(fixture.program()).unwrap();
        assert_eq!(record().missing, [fixture.program()]);
        fs::remove_dir_all(fixture.runtime().join(model)).unwrap();
        assert!(record().missing.contains(&fixture.runtime().join(model)));
        for extension in ["param", "bin"] {
            fs::write(
                fixture.runtime().join(format!("{model}.{extension}")),
                b"not a model directory",
            )
            .unwrap();
        }
        assert!(!record().available);
        assert!(record().missing.contains(&fixture.runtime().join(model)));
        fs::remove_dir_all(fixture.runtime()).unwrap();
        assert!(record().missing.contains(&fixture.runtime()));
        assert!(!record().available);

        let job = image2x_core::protocol::FrameJob {
            engine: engine.into(),
            model: model.into(),
            input_path: String::new(),
            output_path: String::new(),
            args: vec![],
            timeout_ms: 100,
            expected_frames: 2,
            retry_count: 0,
        };
        let result = image2x_core::frame_job::run_frame_job(&fixture.root, &job);
        assert!(!result.process.started);
        assert!(result
            .process
            .diagnostic
            .contains("unsupported frame engine"));
    }
}

#[test]
fn pair_probe_forwards_distinct_inputs_selected_model_cwd_and_dain_tile() {
    for (engine, model) in [
        ("rife-ncnn-vulkan", "rife-v2.4"),
        ("cain-ncnn-vulkan", "cain"),
        ("dain-ncnn-vulkan", "best"),
        ("ifrnet-ncnn-vulkan", "IFRNet_Vimeo90K"),
    ] {
        let fixture = Fixture::new(engine, model, "cp \"$first\" \"$output\"");
        let result = fixture.probe("2", "hardware_gpu");
        assert!(result.passed, "{}", result.diagnostic);
        assert_eq!(result.device_name, "Hardware B");
        assert_eq!(
            fs::read_to_string(fixture.runtime().join("argv")).unwrap(),
            fixture.args("2").join("\n") + "\n"
        );
        assert_eq!(
            fs::read(fixture.output()).unwrap(),
            fs::read(fixture.root.join("first.png")).unwrap()
        );
    }
}

#[test]
fn selected_gpu_id_controls_hardware_vs_software_acceptance() {
    let fixture = Fixture::new("cain-ncnn-vulkan", "cain", "cp \"$first\" \"$output\"");
    let software = fixture.probe("1", "hardware_gpu");
    assert!(!software.passed);
    assert!(!software.device_accepted);
    assert_eq!(software.device_name, "llvmpipe");
    assert!(fixture.probe("1", "software_cpu").passed);
    assert!(fixture.probe("0,2", "hardware_gpu").passed);
    assert!(!fixture.probe("0,1", "hardware_gpu").passed);
    let missing = fixture.probe("9", "hardware_gpu");
    assert!(!missing.passed);
    assert!(missing.diagnostic.contains("device 9 was not reported"));
    for invalid in ["-1", "", "invalid"] {
        let result = fixture.probe(invalid, "hardware_gpu");
        assert!(!result.started);
        assert!(!result.passed);
        assert!(result.diagnostic.contains("nonnegative Vulkan"));
    }
}

#[test]
fn verbose_pair_probe_drains_pipes_and_retains_bounded_logs() {
    let fixture = Fixture::new(
        "dain-ncnn-vulkan",
        "best",
        "head -c 1100000 /dev/zero; head -c 1100000 /dev/zero >&2; cp \"$first\" \"$output\"",
    );
    let result = fixture.probe("2", "hardware_gpu");
    assert!(result.passed, "{}", result.diagnostic);
    assert_eq!(result.stdout.len(), 1024 * 1024);
    assert_eq!(result.stderr.len(), 1024 * 1024);
}

#[test]
fn pair_probe_timeout_kills_pipe_owning_helpers_within_budget() {
    let fixture = Fixture::new("dain-ncnn-vulkan", "best", "sleep 30 & wait");
    let start = Instant::now();
    let result = run_engine_test(
        fixture.program().to_str().unwrap(),
        &fixture.args("2"),
        &fixture.runtime(),
        100,
        &fixture.output(),
        "hardware_gpu",
    );
    assert!(result.started);
    assert!(result.finished);
    assert!(!result.passed);
    assert!(result.diagnostic.contains("deadline"));
    assert!(start.elapsed() < Duration::from_secs(1));
}

#[test]
fn probe_failures_cannot_pass_even_with_output_and_hardware_record() {
    for suffix in [
        "cp \"$first\" \"$output\"; exit 7",
        "cp \"$first\" \"$output\"; echo failed >&2",
        "head -c 1100000 /dev/zero >&2; echo failed >&2; cp \"$first\" \"$output\"",
        "exit 0",
    ] {
        let fixture = Fixture::new("cain-ncnn-vulkan", "cain", suffix);
        let result = fixture.probe("2", "hardware_gpu");
        assert!(!result.passed);
        assert!(!result.diagnostic.is_empty());
    }
    let fixture = Fixture::new("cain-ncnn-vulkan", "cain", "cp \"$first\" \"$output\"");
    fs::remove_file(fixture.program()).unwrap();
    let result = fixture.probe("2", "hardware_gpu");
    assert!(!result.started);
    assert!(!result.passed);
    assert!(!result.diagnostic.is_empty());
}

#[test]
fn stale_output_is_cleared_and_missing_pair_input_fails() {
    let fixture = Fixture::new("cain-ncnn-vulkan", "cain", "exit 0");
    fs::copy(fixture.root.join("first.png"), fixture.output()).unwrap();
    assert!(!fixture.probe("2", "hardware_gpu").passed);
    assert!(!fixture.output().exists());
    fs::remove_file(fixture.root.join("second.png")).unwrap();
    let result = fixture.probe("2", "hardware_gpu");
    assert_eq!(result.exit_code, Some(4));
    assert!(!result.passed);
    fs::create_dir(fixture.output()).unwrap();
    let result = fixture.probe("2", "hardware_gpu");
    assert!(!result.started);
    assert!(result.diagnostic.contains("cannot clear probe output"));
}

#[test]
fn existing_test_engine_wire_reports_pair_probe_errors_and_selected_device() {
    let fixture = Fixture::new("cain-ncnn-vulkan", "cain", "cp \"$first\" \"$output\"");
    let mut request = serde_json::json!({"type": "test_engine", "id": "pair-probe", "program": fixture.program(), "args": fixture.args("2"), "cwd": fixture.runtime(), "output_path": fixture.output(), "timeout_ms": 1000, "device": "hardware_gpu"});
    let response = image2x_core::handle_line(&request.to_string(), &fixture.root).unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["event"], "engine_result");
    assert_eq!(response["data"]["passed"], true);
    assert_eq!(response["data"]["device_name"], "Hardware B");
    request["program"] = serde_json::json!(fixture.runtime().join("not-installed"));
    let response = image2x_core::handle_line(&request.to_string(), &fixture.root).unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["ok"], true); // Transport success, not engine success.
    assert_eq!(response["data"]["started"], false);
    assert_eq!(response["data"]["passed"], false);
    assert!(!response["data"]["diagnostic"].as_str().unwrap().is_empty());
}

#[test]
fn software_probe_overrides_icd_while_hardware_probe_preserves_inherited_environment() {
    use std::io::{BufRead, BufReader, Write};
    use std::os::unix::net::UnixStream;
    use std::process::{Child, Command};
    struct Server(Child, PathBuf);
    impl Drop for Server {
        fn drop(&mut self) {
            let _ = self.0.kill();
            let _ = self.0.wait();
            fs::remove_dir_all(&self.1).unwrap();
        }
    }
    let root = PathBuf::from(format!(
        "/tmp/opencode/i2/probe-env-{}-{}",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    fs::create_dir_all(&root).unwrap();
    let socket = root.join("core.socket");
    // These synthetic strings are only observed by a fake shell probe. No ICD,
    // driver, or system environment is installed or modified by this test.
    let _server = Server(
        Command::new(env!("CARGO_BIN_EXE_image2x-core"))
            .arg("--socket")
            .arg(&socket)
            .arg("--application-directory")
            .arg(&root)
            .env("VK_ICD_FILENAMES", "inherited-test-icd")
            .env("IMAGE2X_SOFTWARE_VULKAN_ICD", "software-test-icd")
            .spawn()
            .unwrap(),
        root.clone(),
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
    let mut reader = BufReader::new(stream.try_clone().unwrap());
    let script = "printf '[0 Hardware A] queueC=0\\n[1 llvmpipe] queueC=0\\n'; printf 'icd=%s\\n' \"$VK_ICD_FILENAMES\"; printf output > \"$1\"";
    for (device, selected, icd) in [
        ("hardware_gpu", "0", "inherited-test-icd"),
        ("software_cpu", "1", "software-test-icd"),
    ] {
        let request = serde_json::json!({"type": "test_engine", "id": "env-probe", "program": "/bin/sh", "args": ["-c", script, "fake-probe", root.join("output.png").to_str().unwrap(), "-g", selected], "cwd": root, "output_path": "output.png", "timeout_ms": 1000, "device": device});
        writeln!(stream, "{request}").unwrap();
        let mut response = String::new();
        reader.read_line(&mut response).unwrap();
        let response: serde_json::Value = serde_json::from_str(&response).unwrap();
        assert_eq!(response["data"]["passed"], true);
        assert!(response["data"]["stdout"]
            .as_str()
            .unwrap()
            .contains(&format!("icd={icd}")));
    }
}
