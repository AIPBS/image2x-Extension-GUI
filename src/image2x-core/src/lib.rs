// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

pub mod manifest;
pub mod process;
pub mod protocol;

use std::path::Path;

use serde_json::Value;
use thiserror::Error;

use manifest::{model_records, ModelRecord};
use process::run_command;
use protocol::{Request, Response};

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
                "capabilities": ["hello", "ping", "list_models", "run"],
            }),
        ),
        Request::Ping { id } => Response::ok(id, "pong", serde_json::json!({})),
        Request::ListModels { id } => {
            let models: Vec<ModelRecord> = model_records();
            Response::ok(id, "models", serde_json::json!({ "models": models }))
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
