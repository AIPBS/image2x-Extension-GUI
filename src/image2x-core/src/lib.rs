// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

pub mod manifest;
pub mod process;
pub mod protocol;
pub mod runtime;

use std::path::Path;

use serde_json::Value;
use thiserror::Error;

use manifest::{model_records, ModelRecord};
use process::{run_command, run_engine_test, ProcessResult};
use protocol::{Request, Response};
use runtime::{runtime_records, RuntimeRecord};

#[derive(Debug, Error)]
pub enum CoreError {
    #[error("invalid request: {0}")]
    InvalidRequest(#[from] serde_json::Error),
    #[error("request is missing an id")]
    MissingRequestId,
    #[error("unsupported request type: {0}")]
    UnsupportedRequest(String),
}

pub fn handle_line(line: &str, application_directory: &Path) -> Result<String, CoreError> {
    let request: Request = serde_json::from_str(line)?;
    let response = match request {
        Request::Hello { id } => Response::ok(
            id,
            "hello",
            serde_json::json!({
                "protocol": protocol::PROTOCOL_VERSION,
            "capabilities": ["hello", "ping", "list_models", "validate_runtime", "run", "process_image"],
            }),
        ),
        Request::Ping { id } => Response::ok(id, "pong", serde_json::json!({})),
        Request::ListModels { id } => {
            let models: Vec<ModelRecord> = model_records(application_directory);
            Response::ok(id, "models", serde_json::json!({ "models": models }))
        }
        Request::ValidateRuntime { id } => {
            let runtimes: Vec<RuntimeRecord> = runtime_records(application_directory);
            Response::ok(id, "runtime", serde_json::json!({ "runtimes": runtimes }))
        }
        Request::Run {
            id,
            program,
            args,
            cwd,
            output_path,
            timeout_ms,
        } => {
            let working_directory = cwd
                .map(std::path::PathBuf::from)
                .unwrap_or_else(|| application_directory.to_path_buf());
            let result = run_command(
                &program,
                &args,
                &working_directory,
                timeout_ms.unwrap_or(60_000),
                output_path.as_deref().map(Path::new),
            );
            Response::ok(
                id,
                "run_result",
                serde_json::to_value(result).unwrap_or_else(
                    |_| serde_json::json!({ "error": "failed to serialize process result" }),
                ),
            )
        }
        Request::ProcessImage {
            id,
            engine,
            model,
            input_path,
            output_path,
            args,
            timeout_ms,
        } => {
            let runtime = runtime_records(application_directory)
                .into_iter()
                .find(|record| record.engine == engine && record.available);
            let model_available = model_records(application_directory)
                .into_iter()
                .any(|record| record.name == model && record.available);
            let result = match runtime {
                None => ProcessResult {
                    started: false,
                    finished: false,
                    exit_code: None,
                    timed_out: false,
                    output_valid: false,
                    stdout: String::new(),
                    stderr: String::new(),
                    diagnostic: format!("runtime or model is unavailable: {engine}/{model}"),
                },
                Some(_) if !model_available => ProcessResult {
                    started: false,
                    finished: false,
                    exit_code: None,
                    timed_out: false,
                    output_valid: false,
                    stdout: String::new(),
                    stderr: String::new(),
                    diagnostic: format!("model is unavailable: {model}"),
                },
                Some(_) if !Path::new(&input_path).is_file() => ProcessResult {
                    started: false,
                    finished: false,
                    exit_code: None,
                    timed_out: false,
                    output_valid: false,
                    stdout: String::new(),
                    stderr: String::new(),
                    diagnostic: format!("input image is unavailable: {input_path}"),
                },
                Some(runtime) => run_command(
                    runtime.executable.to_string_lossy().as_ref(),
                    &args,
                    &runtime.directory,
                    timeout_ms,
                    Some(Path::new(&output_path)),
                ),
            };
            Response::ok(
                id,
                "image_result",
                serde_json::to_value(result).unwrap_or_else(
                    |_| serde_json::json!({ "error": "failed to serialize image result" }),
                ),
            )
        }
        Request::TestEngine {
            id,
            program,
            args,
            cwd,
            output_path,
            timeout_ms,
            device,
        } => {
            let result = run_engine_test(
                &program,
                &args,
                Path::new(&cwd),
                timeout_ms,
                Path::new(&output_path),
                &device,
            );
            Response::ok(
                id,
                "engine_result",
                serde_json::to_value(result).unwrap_or_else(
                    |_| serde_json::json!({ "error": "failed to serialize engine result" }),
                ),
            )
        }
    };
    Ok(response.to_json_line())
}

pub fn error_line(line: &str, error: &CoreError) -> String {
    let id = serde_json::from_str::<Value>(line)
        .ok()
        .and_then(|value| value.get("id").and_then(Value::as_str).map(str::to_owned))
        .unwrap_or_else(|| "unknown".to_owned());
    Response::error(id, error.to_string()).to_json_line()
}
