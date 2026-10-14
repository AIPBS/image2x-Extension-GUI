// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

use std::path::Path;
use std::process::{Command, Stdio};
use std::thread::sleep;
use std::time::{Duration, Instant};

use serde::Serialize;

pub use crate::engine_probe::run_engine_test;
use crate::protocol::ImageJobStage;

#[derive(Debug, Serialize)]
pub struct ProcessResult {
    pub started: bool,
    pub finished: bool,
    pub exit_code: Option<i32>,
    pub timed_out: bool,
    pub output_valid: bool,
    pub stdout: String,
    pub stderr: String,
    pub diagnostic: String,
}

#[derive(Debug, Serialize)]
pub struct EngineTestResult {
    pub started: bool,
    pub finished: bool,
    pub output_valid: bool,
    pub device_accepted: bool,
    pub passed: bool,
    pub exit_code: Option<i32>,
    pub device_name: String,
    pub diagnostic: String,
    pub stdout: String,
    pub stderr: String,
}

pub fn run_command(
    program: &str,
    args: &[String],
    cwd: &Path,
    timeout_ms: u64,
    output_path: Option<&Path>,
) -> ProcessResult {
    run_command_with_environment(program, args, cwd, timeout_ms, output_path, &[])
}

pub fn run_image_job(
    program: &str,
    cwd: &Path,
    stages: &[ImageJobStage],
    timeout_ms: u64,
    retry_count: u32,
) -> ProcessResult {
    let mut last_result = ProcessResult {
        started: false,
        finished: false,
        exit_code: None,
        timed_out: false,
        output_valid: false,
        stdout: String::new(),
        stderr: String::new(),
        diagnostic: "image job has no stages".to_owned(),
    };

    for _attempt in 0..=retry_count {
        let mut completed = true;
        for stage in stages {
            if !Path::new(&stage.input_path).is_file() {
                last_result = ProcessResult {
                    started: false,
                    finished: false,
                    exit_code: None,
                    timed_out: false,
                    output_valid: false,
                    stdout: String::new(),
                    stderr: String::new(),
                    diagnostic: format!("input image is unavailable: {}", stage.input_path),
                };
                completed = false;
                break;
            }
            last_result = run_command(
                program,
                &stage.args,
                cwd,
                timeout_ms,
                Some(Path::new(&stage.output_path)),
            );
            if !last_result.started
                || !last_result.finished
                || last_result.exit_code != Some(0)
                || !last_result.output_valid
            {
                completed = false;
                break;
            }
        }
        if completed {
            return last_result;
        }
        for stage in stages {
            let _ = std::fs::remove_file(&stage.output_path);
        }
    }
    last_result
}

fn run_command_with_environment(
    program: &str,
    args: &[String],
    cwd: &Path,
    timeout_ms: u64,
    output_path: Option<&Path>,
    environment: &[(String, String)],
) -> ProcessResult {
    let mut command = Command::new(program);
    command
        .args(args)
        .current_dir(cwd)
        .stdin(Stdio::null())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped());
    command.envs(environment.iter().map(|(name, value)| (name, value)));
    let child_result = command.spawn();
    let mut child = match child_result {
        Ok(child) => child,
        Err(error) => {
            return ProcessResult {
                started: false,
                finished: false,
                exit_code: None,
                timed_out: false,
                output_valid: false,
                stdout: String::new(),
                stderr: String::new(),
                diagnostic: error.to_string(),
            };
        }
    };

    let deadline = Instant::now() + Duration::from_millis(timeout_ms);
    let mut timed_out = false;
    loop {
        match child.try_wait() {
            Ok(Some(_)) => break,
            Ok(None) if Instant::now() >= deadline => {
                timed_out = true;
                let _ = child.kill();
                let _ = child.wait();
                break;
            }
            Ok(None) => sleep(Duration::from_millis(10)),
            Err(error) => {
                let _ = child.kill();
                let _ = child.wait();
                return ProcessResult {
                    started: true,
                    finished: false,
                    exit_code: None,
                    timed_out: false,
                    output_valid: false,
                    stdout: String::new(),
                    stderr: String::new(),
                    diagnostic: error.to_string(),
                };
            }
        }
    }

    let output = child.wait_with_output();
    let output = match output {
        Ok(output) => output,
        Err(error) => {
            return ProcessResult {
                started: true,
                finished: false,
                exit_code: None,
                timed_out,
                output_valid: false,
                stdout: String::new(),
                stderr: String::new(),
                diagnostic: error.to_string(),
            };
        }
    };
    let output_valid = output_path.map(is_nonempty_file).unwrap_or(true);
    let exit_code = output.status.code();
    let diagnostic = if timed_out {
        "process exceeded its deadline".to_owned()
    } else if !output.status.success() {
        format!("process exited with code {:?}", exit_code)
    } else if !output_valid {
        "process completed without a non-empty output file".to_owned()
    } else {
        String::new()
    };
    ProcessResult {
        started: true,
        finished: true,
        exit_code,
        timed_out,
        output_valid,
        stdout: String::from_utf8_lossy(&output.stdout).into_owned(),
        stderr: String::from_utf8_lossy(&output.stderr).into_owned(),
        diagnostic,
    }
}

fn is_nonempty_file(path: &Path) -> bool {
    std::fs::metadata(path)
        .map(|metadata| metadata.is_file() && metadata.len() > 0)
        .unwrap_or(false)
}

#[cfg(test)]
mod tests {
    use super::{run_command, run_engine_test, run_image_job};
    use crate::engine_probe::reported_device;
    use crate::protocol::ImageJobStage;
    use std::path::Path;

    fn test_directory(name: &str) -> std::path::PathBuf {
        let path = std::path::PathBuf::from(format!(
            "/tmp/opencode/i2/process-test-{}-{name}",
            std::process::id()
        ));
        std::fs::create_dir_all(&path).expect("test directory should be created");
        path
    }

    #[test]
    fn missing_program_is_reported_without_panic() {
        let result = run_command(
            "/image2x/program/does-not-exist",
            &[],
            Path::new("/tmp"),
            100,
            None,
        );
        assert!(!result.started);
        assert!(!result.diagnostic.is_empty());
    }

    #[test]
    fn parses_vulkan_device_from_engine_output() {
        let device = reported_device("[0 llvmpipe (LLVM 18.1.3)] queueC=0", "", &[]).unwrap();
        assert_eq!(device, "llvmpipe (LLVM 18.1.3)");
    }

    #[test]
    fn parses_vulkan_device_with_multiple_spaces_before_queue() {
        let device = reported_device(
            "[0 llvmpipe (LLVM 21.1.8, 256 bits)]  queueC=0[1]  queueG=0",
            "",
            &[],
        )
        .unwrap();
        assert_eq!(device, "llvmpipe (LLVM 21.1.8, 256 bits)");
    }

    #[test]
    fn engine_test_requires_the_requested_device_class() {
        let directory = test_directory("software-device");
        let result = run_engine_test(
            "/bin/sh",
            &[
                "-c".to_owned(),
                "printf '[0 llvmpipe] queueC=0\\n'; printf x > output.png".to_owned(),
            ],
            &directory,
            1000,
            &directory.join("output.png"),
            "software_cpu",
        );
        assert!(result.passed);
        std::fs::remove_dir_all(directory).expect("test directory should be removed");
    }

    #[test]
    fn image_job_retries_stages_and_validates_outputs() {
        let directory = test_directory("image-job");
        let input = directory.join("input.png");
        let output = directory.join("output.png");
        std::fs::write(&input, b"input").expect("job input should be created");
        let stage = ImageJobStage {
            input_path: input.to_string_lossy().into_owned(),
            output_path: output.to_string_lossy().into_owned(),
            args: vec![
                "-c".to_owned(),
                "printf output > \"$1\"".to_owned(),
                "fake-engine".to_owned(),
                output.to_string_lossy().into_owned(),
            ],
        };
        let result = run_image_job("/bin/sh", &directory, &[stage], 1000, 1);
        assert!(result.started);
        assert!(result.finished);
        assert_eq!(result.exit_code, Some(0));
        assert!(result.output_valid);
        std::fs::remove_dir_all(directory).expect("test directory should be removed");
    }
}
