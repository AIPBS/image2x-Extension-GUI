//! Raw engine compatibility probes using the bounded frame process supervisor.
//! Pair-input options are forwarded unchanged; Qt still decodes the result image.

use std::fs;
use std::path::Path;
use std::time::{Duration, Instant};

use crate::frame_cancel::Cancellation;
use crate::frame_process::{failure, run_with_environment};
use crate::process::{EngineTestResult, ProcessResult};

/// Run the existing test_engine argv API with continuous pipe draining and
/// process-group cleanup. This is a probe, not typed folder-job engine support.
pub fn run_engine_test(
    program: &str,
    args: &[String],
    cwd: &Path,
    timeout_ms: u64,
    output_path: &Path,
    device: &str,
) -> EngineTestResult {
    let output = if output_path.is_absolute() {
        output_path.to_path_buf()
    } else {
        cwd.join(output_path)
    };
    let prepared = (|| -> Result<Instant, String> {
        if !matches!(device, "hardware_gpu" | "software_cpu") {
            return Err(format!("unknown engine-test device criterion: {device}"));
        }
        selected_ids(args)?;
        if timeout_ms == 0 {
            return Err("engine test timeout must be positive".into());
        }
        let deadline = Instant::now()
            .checked_add(Duration::from_millis(timeout_ms))
            .ok_or("engine test timeout is too large")?;
        match fs::remove_file(&output) {
            Ok(()) => (),
            Err(error) if error.kind() == std::io::ErrorKind::NotFound => (),
            Err(error) => {
                return Err(format!(
                    "cannot clear probe output {}: {error}",
                    output.display()
                ))
            }
        }
        Ok(deadline)
    })();
    let mut process = match prepared {
        Ok(deadline) => run_with_environment(
            Path::new(program),
            args,
            cwd,
            deadline,
            &Cancellation::local(),
            &software_vulkan_environment(device),
        ),
        Err(message) => failure(message),
    };
    // Keep the existing nonempty-file contract; the GUI performs image decoding.
    // A failed engine may leave an artifact, but passed must remain false.
    if process.started {
        process.output_valid =
            fs::metadata(&output).is_ok_and(|metadata| metadata.is_file() && metadata.len() > 0);
        if process.diagnostic.is_empty() && !process.output_valid {
            process.diagnostic = "process completed without a non-empty output file".into();
        }
    }
    result(process, args, device)
}

fn result(process: ProcessResult, args: &[String], device: &str) -> EngineTestResult {
    let reported = reported_device(&process.stdout, &process.stderr, args);
    let device_name = reported.as_ref().cloned().unwrap_or_default();
    let software_device = is_software_device(&device_name);
    let device_accepted = match device {
        "hardware_gpu" => !device_name.is_empty() && !software_device,
        "software_cpu" => !device_name.is_empty() && software_device,
        _ => false,
    };
    let mut diagnostic = process.diagnostic;
    if diagnostic.is_empty() {
        diagnostic = match reported {
            Err(message) => message,
            Ok(_) if device_name.is_empty() => "the engine did not report a Vulkan device".into(),
            Ok(_) if !device_accepted => format!("unexpected Vulkan device: {device_name}"),
            Ok(_) => String::new(),
        };
    }
    EngineTestResult {
        started: process.started,
        finished: process.finished,
        output_valid: process.output_valid,
        device_accepted,
        passed: process.started
            && process.finished
            && !process.timed_out
            && process.exit_code == Some(0)
            && process.output_valid
            && device_accepted
            && diagnostic.is_empty(),
        exit_code: process.exit_code,
        device_name,
        diagnostic,
        stdout: process.stdout,
        stderr: process.stderr,
    }
}

fn software_vulkan_environment(device: &str) -> Vec<(String, String)> {
    if device != "software_cpu" {
        return Vec::new();
    }
    if let Ok(icd) = std::env::var("IMAGE2X_SOFTWARE_VULKAN_ICD") {
        if !icd.is_empty() {
            return vec![("VK_ICD_FILENAMES".to_owned(), icd)];
        }
    }
    [
        "/usr/share/vulkan/icd.d/lvp_icd.json",
        "/etc/vulkan/icd.d/lvp_icd.json",
    ]
    .iter()
    .find(|path| Path::new(path).is_file())
    .map(|path| vec![("VK_ICD_FILENAMES".to_owned(), (*path).to_owned())])
    .unwrap_or_default()
}

fn selected_ids(args: &[String]) -> Result<Option<Vec<u32>>, String> {
    let mut selected = None;
    for (index, arg) in args.iter().enumerate() {
        if arg != "-g" {
            continue;
        }
        let value = args.get(index + 1).ok_or("missing value for -g")?;
        let ids = value
            .split(',')
            .map(|id| id.trim().parse::<u32>())
            .collect::<Result<Vec<_>, _>>()
            .map_err(|_| format!("-g requires nonnegative Vulkan device IDs; got {value}"))?;
        selected = Some(ids);
    }
    Ok(selected)
}

pub(crate) fn reported_device(
    stdout: &str,
    stderr: &str,
    args: &[String],
) -> Result<String, String> {
    let devices: Vec<_> = stdout
        .lines()
        .chain(stderr.lines())
        .filter_map(|line| {
            let queue = line.find("queueC=")?;
            let header = &line[..queue];
            let start = header.find('[')?;
            let end = header.rfind(']')?;
            if start >= end {
                return None;
            }
            let mut fields = header[start + 1..end].splitn(2, char::is_whitespace);
            let index = fields.next()?.parse::<u32>().ok()?;
            let name = fields.next()?.trim();
            (!name.is_empty()).then_some((index, name))
        })
        .collect();
    let Some(selected) = selected_ids(args)? else {
        // Retain legacy auto-selection behavior only when argv has no -g.
        return Ok(devices
            .first()
            .map(|(_, name)| (*name).to_owned())
            .unwrap_or_default());
    };
    let names = selected
        .iter()
        .map(|selected| {
            devices
                .iter()
                .find(|(index, _)| index == selected)
                .map(|(_, name)| *name)
                .ok_or_else(|| format!("selected Vulkan device {selected} was not reported"))
        })
        .collect::<Result<Vec<_>, _>>()?;
    Ok(names.join("; "))
}

fn is_software_device(device_name: &str) -> bool {
    let normalized = device_name.to_ascii_lowercase();
    ["llvmpipe", "lavapipe", "softpipe", "swiftshader"]
        .iter()
        .any(|name| normalized.contains(name))
}
