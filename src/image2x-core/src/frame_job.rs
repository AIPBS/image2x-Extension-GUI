//! One bounded IFRNet or RIFE folder pass, with clean retries and PNG checks.
//!
//! Qt owns successive 2x passes and progress polling. Explicit cancel_frames
//! requests stop active passes; disconnect alone does not cancel a job.

use std::collections::HashSet;
use std::fs;
use std::path::{Component, Path, PathBuf};
use std::sync::{Mutex, OnceLock};
use std::time::{Duration, Instant};

use serde::Serialize;

use crate::frame_cancel::Cancellation;
use crate::frame_process::{failure, run};
use crate::process::ProcessResult;
use crate::protocol::FrameJob;
use crate::runtime::{is_executable, runtime_root};

/// ProcessResult wire fields plus the number of validated output frames.
#[derive(Debug, Serialize)]
pub struct FrameResult {
    #[serde(flatten)]
    pub process: ProcessResult,
    pub frame_count: u32,
}

static OUTPUTS: OnceLock<Mutex<HashSet<PathBuf>>> = OnceLock::new();

struct OutputLease(PathBuf);

impl OutputLease {
    fn acquire(path: PathBuf) -> Result<Self, String> {
        // The mutex protects only reservations, not engine execution. The lease
        // lives through all attempts and pipe joins, so another connection cannot
        // clean or overwrite this job's output while its child is still alive.
        let mut outputs = OUTPUTS
            .get_or_init(Default::default)
            .lock()
            .map_err(|_| "output reservation lock is poisoned")?;
        if outputs
            .iter()
            .any(|other| path.starts_with(other) || other.starts_with(&path))
        {
            return Err("output folder is already used by another frame job".into());
        }
        outputs.insert(path.clone());
        Ok(Self(path))
    }
}

impl Drop for OutputLease {
    fn drop(&mut self) {
        if let Some(outputs) = OUTPUTS.get() {
            if let Ok(mut outputs) = outputs.lock() {
                outputs.remove(&self.0);
            }
        }
    }
}

struct Prepared {
    executable: PathBuf,
    directory: PathBuf,
    output: PathBuf,
    args: Vec<String>,
    width: usize,
    _lease: OutputLease,
}

fn prepare(application: &Path, job: &FrameJob) -> Result<Prepared, String> {
    if !matches!(
        job.engine.as_str(),
        "ifrnet-ncnn-vulkan" | "rife-ncnn-vulkan"
    ) {
        return Err(format!("unsupported frame engine: {}", job.engine));
    }
    let mut components = Path::new(&job.model).components();
    if !matches!(components.next(), Some(Component::Normal(_))) || components.next().is_some() {
        return Err("model must be a directory name".into());
    }
    if job.timeout_ms == 0 || job.expected_frames == 0 || job.retry_count > 100 {
        return Err(
            "timeout and expected_frames must be positive; retry_count must be <= 100".into(),
        );
    }
    let directory = fs::canonicalize(runtime_root(application).join(&job.engine))
        .map_err(|error| format!("runtime is unavailable: {error}"))?;
    let executable = directory.join(&job.engine);
    if !executable.is_file() || !is_executable(&executable) {
        return Err(format!(
            "runtime executable is unavailable: {}",
            executable.display()
        ));
    }
    let model = fs::canonicalize(directory.join(&job.model))
        .map_err(|error| format!("model is unavailable: {error}"))?;
    if !model.is_dir()
        || !model.starts_with(&directory)
        || fs::read_dir(&model)
            .map_err(|error| error.to_string())?
            .next()
            .is_none()
    {
        return Err("model directory is empty or outside the runtime".into());
    }
    validate_model_files(&job.engine, &job.model, &model)?;
    let input = fs::canonicalize(&job.input_path)
        .map_err(|error| format!("input folder is unavailable: {error}"))?;
    if !input.is_dir()
        || fs::read_dir(&input)
            .map_err(|error| error.to_string())?
            .filter_map(Result::ok)
            .all(|entry| entry.path().extension().is_none_or(|ext| ext != "png"))
    {
        return Err("input folder must contain PNG frames".into());
    }
    fs::create_dir_all(&job.output_path).map_err(|error| format!("output folder: {error}"))?;
    let output = fs::canonicalize(&job.output_path).map_err(|error| error.to_string())?;
    if output.starts_with(&input)
        || input.starts_with(&output)
        || output.starts_with(&directory)
        || directory.starts_with(&output)
    {
        return Err("output folder overlaps input or runtime".into());
    }
    let lease = OutputLease::acquire(output.clone())?;
    if fs::read_dir(&output)
        .map_err(|error| error.to_string())?
        .next()
        .is_some()
    {
        return Err("output folder must be empty before a frame job".into());
    }
    let mut args = Vec::new();
    let mut format = format!("%0{}d.png", job.expected_frames.to_string().len());
    let mut index = 0;
    while index < job.args.len() {
        let arg = &job.args[index];
        if matches!(arg.as_str(), "-i" | "-o" | "-m" | "-f") {
            let value = job
                .args
                .get(index + 1)
                .ok_or_else(|| format!("missing value for {arg}"))?;
            if arg == "-f" {
                format = value.clone();
            }
            index += 2;
        } else {
            args.push(arg.clone());
            index += 1;
        }
    }
    let width = format
        .strip_prefix("%0")
        .and_then(|value| value.strip_suffix("d.png"))
        .and_then(|value| value.parse::<usize>().ok())
        .filter(|width| (1..=20).contains(width))
        .ok_or("frame format must be %0Nd.png with N between 1 and 20")?;
    for (flag, value) in [("-i", input), ("-o", output.clone()), ("-m", model)] {
        args.extend([flag.to_owned(), value.to_string_lossy().into_owned()]);
    }
    args.extend(["-f".into(), format]);
    Ok(Prepared {
        executable,
        directory,
        output,
        args,
        width,
        _lease: lease,
    })
}

fn validate_model_files(engine: &str, name: &str, model: &Path) -> Result<(), String> {
    // Keep IFRNet's established nonempty-directory policy. RIFE v4 models use
    // only flownet; older named models also need contextnet and fusionnet.
    // These filesystem checks cannot prove compatibility with an installed
    // executable: unsupported NCNN layers still fail through the real process.
    let networks: &[&str] = match engine {
        "rife-ncnn-vulkan" if name.starts_with("rife-v4") => &["flownet"],
        "rife-ncnn-vulkan" => &["flownet", "contextnet", "fusionnet"],
        _ => &[],
    };
    for network in networks {
        for extension in ["param", "bin"] {
            let file = model.join(format!("{network}.{extension}"));
            let canonical = fs::canonicalize(&file).map_err(|error| {
                format!("model file is unavailable: {}: {error}", file.display())
            })?;
            let metadata = fs::metadata(&canonical).map_err(|error| error.to_string())?;
            if !canonical.starts_with(model) || !metadata.is_file() || metadata.len() == 0 {
                return Err(format!(
                    "model file is empty or outside model: {}",
                    file.display()
                ));
            }
        }
    }
    Ok(())
}

fn validate_output(folder: &Path, expected: u32, width: usize) -> Result<u32, String> {
    let entries = fs::read_dir(folder)
        .map_err(|error| error.to_string())?
        .collect::<Result<Vec<_>, _>>()
        .map_err(|error| error.to_string())?;
    if entries.len() != expected as usize {
        return Err(format!(
            "incomplete frame output: expected {expected}, found {}",
            entries.len()
        ));
    }
    for number in 1..=expected {
        let path = folder.join(format!("{number:0width$}.png"));
        if !fs::symlink_metadata(&path)
            .map_err(|error| error.to_string())?
            .file_type()
            .is_file()
        {
            return Err(format!("frame is not a regular file: {}", path.display()));
        }
        let bytes = fs::read(&path)
            .map_err(|error| format!("missing frame {}: {error}", path.display()))?;
        // Structural PNG checks reject empty, truncated, and mislabeled files.
        // Pixel decoding/CRC validation remains the consuming media layer's job.
        if bytes.len() < 45
            || !bytes.starts_with(b"\x89PNG\r\n\x1a\n")
            || &bytes[12..16] != b"IHDR"
            || bytes[16..20] == [0; 4]
            || bytes[20..24] == [0; 4]
            || &bytes[bytes.len() - 8..bytes.len() - 4] != b"IEND"
        {
            return Err(format!("invalid PNG frame: {}", path.display()));
        }
    }
    Ok(expected)
}

fn clean_output(folder: &Path) -> Result<(), String> {
    for entry in fs::read_dir(folder).map_err(|error| error.to_string())? {
        let path = entry.map_err(|error| error.to_string())?.path();
        if path.is_dir() && !path.is_symlink() {
            fs::remove_dir_all(&path).map_err(|error| error.to_string())?;
        } else {
            fs::remove_file(&path).map_err(|error| error.to_string())?;
        }
    }
    Ok(())
}

/// Execute one folder pass. retry_count is additional attempts; timeout_ms is
/// the total execution budget, shared by all attempts, not reset on retry.
pub fn run_frame_job(application: &Path, job: &FrameJob) -> FrameResult {
    run_cancellable(application, job, &mut Cancellation::local())
}

pub(crate) fn run_frame_request(application: &Path, id: &str, job: &FrameJob) -> FrameResult {
    let mut cancellation = match Cancellation::register(id) {
        Ok(token) => token,
        Err(message) => {
            return FrameResult {
                process: failure(message),
                frame_count: 0,
            }
        }
    };
    let mut result = run_cancellable(application, job, &mut cancellation);
    if cancellation.complete() {
        result.process.output_valid = false;
        if !result.process.diagnostic.contains("cancelled") {
            result.process.diagnostic =
                format!("frame job cancelled; {}", result.process.diagnostic);
        }
        result.frame_count = 0;
    }
    result
}

fn run_cancellable(
    application: &Path,
    job: &FrameJob,
    cancellation: &mut Cancellation,
) -> FrameResult {
    let prepared = match prepare(application, job) {
        Ok(prepared) => prepared,
        Err(message) => {
            return FrameResult {
                process: failure(message),
                frame_count: 0,
            }
        }
    };
    let Some(deadline) = Instant::now().checked_add(Duration::from_millis(job.timeout_ms)) else {
        return FrameResult {
            process: failure("timeout is too large"),
            frame_count: 0,
        };
    };
    let mut result = failure("frame job did not run");
    for _ in 0..=job.retry_count {
        if cancellation.is_cancelled() {
            result.diagnostic = "frame job cancelled".into();
            break;
        }
        result = run(
            &prepared.executable,
            &prepared.args,
            &prepared.directory,
            deadline,
            cancellation,
        );
        if result.diagnostic.is_empty() && result.finished && result.exit_code == Some(0) {
            match validate_output(&prepared.output, job.expected_frames, prepared.width) {
                Ok(frame_count) => {
                    if !cancellation.complete() {
                        result.output_valid = true;
                        return FrameResult {
                            process: result,
                            frame_count,
                        };
                    }
                    result.diagnostic = "frame job cancelled".into();
                }
                Err(message) => result.diagnostic = message,
            }
        }
        if let Err(error) = clean_output(&prepared.output) {
            result
                .diagnostic
                .push_str(&format!("; output cleanup failed: {error}"));
            break;
        }
        if cancellation.is_cancelled() {
            result.diagnostic = "frame job cancelled".into();
            break;
        }
        if result.timed_out || Instant::now() >= deadline {
            break;
        }
    }
    FrameResult {
        process: result,
        frame_count: 0,
    }
}
