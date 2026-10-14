//! CAIN/DAIN folder jobs against immutable fake runtimes, never real models.
#![cfg(unix)]

#[path = "support/fake_engine.rs"]
mod fake_engine;

use std::fs;
use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::time::{Duration, Instant};

use image2x_core::frame_job::run_frame_job;
use image2x_core::protocol::FrameJob;

static NEXT: AtomicU64 = AtomicU64::new(0);
const ENGINES: [&str; 2] = ["cain-ncnn-vulkan", "dain-ncnn-vulkan"];
const SUCCESS: &str = "number=1; while [ \"$number\" -le \"${frames:-2}\" ]; do name=$(printf \"$format\" \"$number\"); cp \"$input/1.png\" \"$output/$name\"; number=$((number + 1)); done";

struct Fixture {
    root: PathBuf,
    engine: &'static str,
}

impl Fixture {
    fn new(engine: &'static str, behavior: &str) -> Self {
        let root = PathBuf::from(format!(
            "/tmp/opencode/i2/cain dain-{}-{}",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        let fixture = Self { root, engine };
        fs::create_dir_all(fixture.runtime().join(fixture.model())).unwrap();
        for network in fixture.networks() {
            for extension in ["param", "bin"] {
                fs::write(
                    fixture
                        .runtime()
                        .join(fixture.model())
                        .join(format!("{network}.{extension}")),
                    b"fake model",
                )
                .unwrap();
            }
        }
        fs::create_dir_all(fixture.root.join("input")).unwrap();
        let hex = "89504e470d0a1a0a0000000d4948445200000001000000010804000000b51c0c020000000b4944415478da6364f80f00010501012718e3660000000049454e44ae426082";
        let png: Vec<u8> = (0..hex.len())
            .step_by(2)
            .map(|index| u8::from_str_radix(&hex[index..index + 2], 16).unwrap())
            .collect();
        fs::write(fixture.root.join("input/1.png"), png).unwrap();
        let script = format!("printf '%s\\n' \"$@\" > argv\nwhile [ \"$#\" -gt 0 ]; do\n case \"$1\" in -i) input=$2; shift;; -o) output=$2; shift;; -m) model=$2; shift;; -f) format=$2; shift;; -n) frames=$2; shift;; esac\n shift\ndone\n{behavior}\n");
        fake_engine::install(&fixture.runtime().join(engine), &script).unwrap();
        fixture
    }

    fn runtime(&self) -> PathBuf {
        self.root
            .join("dependencies/distributable")
            .join(self.engine)
    }
    fn model(&self) -> &'static str {
        if self.engine == ENGINES[0] {
            "cain"
        } else {
            "best"
        }
    }
    fn networks(&self) -> &'static [&'static str] {
        if self.engine == ENGINES[0] {
            &["cain"]
        } else {
            &["ctxnet", "depthnet", "flownet", "interpolation"]
        }
    }

    fn job(&self) -> FrameJob {
        let mut job = FrameJob {
            engine: self.engine.into(),
            model: self.model().into(),
            input_path: self.root.join("input").to_string_lossy().into_owned(),
            output_path: self.root.join("output").to_string_lossy().into_owned(),
            args: ["-f", "%04d.png", "-g", "0,1", "-j", "4:2,2:4"]
                .map(str::to_owned)
                .to_vec(),
            timeout_ms: 2000,
            expected_frames: 2,
            retry_count: 0,
        };
        if self.engine == ENGINES[1] {
            job.args
                .extend(["-n", "6", "-t", "128,256"].map(str::to_owned));
            job.expected_frames = 6;
        }
        job
    }
}

impl Drop for Fixture {
    fn drop(&mut self) {
        fs::remove_dir_all(&self.root).unwrap();
    }
}

fn request(id: &str, job: &FrameJob) -> String {
    serde_json::json!({"type": "process_frames", "id": id, "engine": job.engine,
        "model": job.model, "input_path": job.input_path, "output_path": job.output_path,
        "args": job.args, "timeout_ms": job.timeout_ms, "expected_frames": job.expected_frames,
        "retry_count": job.retry_count})
    .to_string()
}

fn cancel(application: &std::path::Path, target: &str) -> bool {
    let line =
        serde_json::json!({"type": "cancel_frames", "id": "stop-new-engine", "target_id": target})
            .to_string();
    let response = image2x_core::handle_line(&line, application).unwrap();
    let response: serde_json::Value = serde_json::from_str(&response).unwrap();
    assert_eq!(response["event"], "cancel_frames");
    response["data"]["cancelled"].as_bool().unwrap()
}

fn await_running(
    fixture: &Fixture,
    worker: std::thread::JoinHandle<String>,
) -> std::thread::JoinHandle<String> {
    let deadline = Instant::now() + Duration::from_secs(2);
    while !fixture.runtime().join("running").exists() && Instant::now() < deadline {
        std::thread::yield_now();
    }
    if !fixture.runtime().join("running").exists() && worker.is_finished() {
        panic!(
            "fake child did not start; response: {}",
            worker.join().unwrap()
        );
    }
    assert!(
        fixture.runtime().join("running").exists(),
        "fake child must start"
    );
    worker
}

#[test]
fn cain_2x_and_dain_requested_single_pass_count_preserve_exact_flags_and_paths() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, SUCCESS);
        let mut job = fixture.job();
        let mut expected_args = job.args[2..].to_vec();
        for flag in ["-i", "-o", "-m"] {
            job.args.extend([flag.into(), "/wrong/path".into()]);
        }
        for (flag, path) in [
            ("-i", fixture.root.join("input")),
            ("-o", fixture.root.join("output")),
            ("-m", fixture.runtime().join(fixture.model())),
        ] {
            expected_args.extend([flag.into(), path.to_string_lossy().into_owned()]);
        }
        expected_args.extend(["-f".into(), "%04d.png".into()]);
        let line =
            image2x_core::handle_line(&request(&format!("success-{engine}"), &job), &fixture.root)
                .unwrap();
        let result: serde_json::Value = serde_json::from_str(&line).unwrap();
        assert_eq!(result["event"], "process_frames");
        assert_eq!(result["data"]["output_valid"], true);
        assert_eq!(result["data"]["frame_count"], job.expected_frames);
        assert_eq!(
            fs::read_to_string(fixture.runtime().join("argv")).unwrap(),
            expected_args.join("\n") + "\n"
        );
        assert_eq!(
            fs::read_dir(&job.output_path).unwrap().count(),
            job.expected_frames as usize
        );
    }
}

#[test]
fn every_required_model_pair_is_checked_for_missing_and_empty_files() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, SUCCESS);
        let model = fixture.runtime().join(fixture.model());
        for network in fixture.networks() {
            for extension in ["param", "bin"] {
                let name = format!("{network}.{extension}");
                let path = model.join(&name);
                fs::remove_file(&path).unwrap();
                let missing = run_frame_job(&fixture.root, &fixture.job());
                assert!(!missing.process.started, "{missing:?}");
                assert!(missing.process.diagnostic.contains(&name));
                fs::write(&path, b"").unwrap();
                let empty = run_frame_job(&fixture.root, &fixture.job());
                assert!(!empty.process.started, "{empty:?}");
                assert!(empty.process.diagnostic.contains(&name));
                fs::write(path, b"fake model").unwrap();
            }
        }
        fs::remove_dir_all(model).unwrap();
        assert!(!run_frame_job(&fixture.root, &fixture.job()).process.started);
    }
}

#[test]
fn model_aliases_escaped_files_and_unknown_engines_are_rejected() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, SUCCESS);
        let mut job = fixture.job();
        fs::rename(
            fixture.runtime().join(fixture.model()),
            fixture.runtime().join("alias"),
        )
        .unwrap();
        job.model = "alias".into();
        let result = run_frame_job(&fixture.root, &job);
        assert!(!result.process.started);
        assert!(result
            .process
            .diagnostic
            .contains("unsupported model directory"));
        fs::rename(
            fixture.runtime().join("alias"),
            fixture.runtime().join(fixture.model()),
        )
        .unwrap();
        let path = fixture
            .runtime()
            .join(fixture.model())
            .join(format!("{}.bin", fixture.networks()[0]));
        fs::remove_file(&path).unwrap();
        std::os::unix::fs::symlink(fixture.root.join("input/1.png"), path).unwrap();
        assert!(!run_frame_job(&fixture.root, &fixture.job()).process.started);
        job.engine = "unimplemented-frame-engine".into();
        assert!(run_frame_job(&fixture.root, &job)
            .process
            .diagnostic
            .contains("unsupported frame engine"));
    }
}

#[test]
fn wrong_png_counts_reject_output_instead_of_inventing_engine_orchestration() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, SUCCESS);
        let mut job = fixture.job();
        job.expected_frames += 1;
        let result = run_frame_job(&fixture.root, &job);
        assert!(result.process.finished, "{result:?}");
        assert!(!result.process.output_valid);
        assert!(result
            .process
            .diagnostic
            .contains("incomplete frame output"));
        assert_eq!(fs::read_dir(&job.output_path).unwrap().count(), 0);
    }
}

#[test]
fn nonzero_exit_and_failure_logs_reject_even_complete_output() {
    for engine in ENGINES {
        for suffix in ["exit 7", "echo failed >&2"] {
            let fixture = Fixture::new(engine, &format!("{SUCCESS}; {suffix}"));
            let result = run_frame_job(&fixture.root, &fixture.job());
            assert!(result.process.finished, "{result:?}");
            assert!(!result.process.output_valid);
            assert!(result.process.diagnostic.contains("failed"));
            assert_eq!(
                fs::read_dir(fixture.root.join("output")).unwrap().count(),
                0
            );
        }
    }
}

#[test]
fn retries_clean_stale_output_for_both_engines() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, &format!("if [ ! -f attempted ]; then touch attempted; touch \"$output/stale.png\"; exit 1; fi; {SUCCESS}"));
        let mut job = fixture.job();
        job.retry_count = 1;
        let result = run_frame_job(&fixture.root, &job);
        assert!(result.process.output_valid, "{result:?}");
        assert_eq!(result.frame_count, job.expected_frames);
    }
}

#[test]
fn shared_timeout_kills_helpers_without_retrying() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, "echo attempt >> attempts; sleep 30 & wait");
        let mut job = fixture.job();
        job.timeout_ms = 100;
        job.retry_count = 3;
        let start = Instant::now();
        let result = run_frame_job(&fixture.root, &job);
        assert!(
            result.process.started && result.process.finished,
            "{result:?}"
        );
        assert!(result.process.timed_out, "{result:?}");
        assert!(!result.process.output_valid);
        assert!(start.elapsed() < Duration::from_secs(1));
        assert_eq!(
            fs::read_to_string(fixture.runtime().join("attempts")).unwrap(),
            "attempt\n"
        );
    }
}

#[test]
fn cancellation_kills_active_helpers_suppresses_retries_and_releases_registry() {
    for engine in ENGINES {
        let fixture = Fixture::new(engine, "echo attempt >> attempts; touch \"$output/partial.png\"; touch running; sleep 30 & wait");
        let mut job = fixture.job();
        job.timeout_ms = 5000;
        job.retry_count = 3;
        let application = fixture.root.clone();
        let id = format!("cancel-{engine}");
        let line = request(&id, &job);
        let worker =
            std::thread::spawn(move || image2x_core::handle_line(&line, &application).unwrap());
        let worker = await_running(&fixture, worker);
        let start = Instant::now();
        assert!(cancel(&fixture.root, &id));
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
        assert!(!cancel(&fixture.root, &id));
    }
}

#[test]
fn output_reservation_is_shared_between_cain_and_dain() {
    let cain = Fixture::new(ENGINES[0], "touch running; sleep 30 & wait");
    let dain = Fixture::new(ENGINES[1], SUCCESS);
    let cain_job = cain.job();
    let application = cain.root.clone();
    let line = request("cain-reserved-output", &cain_job);
    let worker =
        std::thread::spawn(move || image2x_core::handle_line(&line, &application).unwrap());
    let worker = await_running(&cain, worker);
    let mut dain_job = dain.job();
    dain_job.output_path = cain_job.output_path;
    let conflict = run_frame_job(&dain.root, &dain_job);
    assert!(!conflict.process.started);
    assert!(conflict.process.diagnostic.contains("already used"));
    assert!(cancel(&cain.root, "cain-reserved-output"));
    worker.join().unwrap();
}
