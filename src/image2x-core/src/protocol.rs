// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

use serde::{Deserialize, Serialize};
use serde_json::Value;

pub const PROTOCOL_VERSION: u32 = 1;

#[derive(Debug, Deserialize)]
pub struct ImageJobStage {
    pub input_path: String,
    pub output_path: String,
    #[serde(default)]
    pub args: Vec<String>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "type", rename_all = "snake_case")]
pub enum Request {
    Hello {
        id: String,
    },
    Ping {
        id: String,
    },
    ListModels {
        id: String,
    },
    ValidateRuntime {
        id: String,
    },
    Run {
        id: String,
        program: String,
        #[serde(default)]
        args: Vec<String>,
        cwd: Option<String>,
        output_path: Option<String>,
        timeout_ms: Option<u64>,
    },
    ProcessImage {
        id: String,
        engine: String,
        model: String,
        input_path: String,
        output_path: String,
        #[serde(default)]
        args: Vec<String>,
        timeout_ms: u64,
        #[serde(default)]
        stages: Vec<ImageJobStage>,
        #[serde(default)]
        retry_count: u32,
    },
    TestEngine {
        id: String,
        program: String,
        #[serde(default)]
        args: Vec<String>,
        cwd: String,
        output_path: String,
        timeout_ms: u64,
        device: String,
    },
}

#[derive(Debug, Serialize)]
pub struct Response {
    pub id: String,
    pub ok: bool,
    pub event: String,
    pub data: Value,
}

impl Response {
    pub fn ok(id: String, event: &str, data: Value) -> Self {
        Self {
            id,
            ok: true,
            event: event.to_owned(),
            data,
        }
    }

    pub fn error(id: String, message: String) -> Self {
        Self {
            id,
            ok: false,
            event: "error".to_owned(),
            data: serde_json::json!({ "message": message }),
        }
    }

    pub fn to_json_line(&self) -> String {
        let mut line = serde_json::to_string(self).expect("response serialization must succeed");
        line.push('\n');
        line
    }
}

#[cfg(test)]
mod tests {
    use super::{Request, Response, PROTOCOL_VERSION};

    #[test]
    fn parses_ping_request() {
        let request: Request = serde_json::from_str(r#"{"type":"ping","id":"one"}"#)
            .expect("ping request should parse");
        assert!(matches!(request, Request::Ping { id } if id == "one"));
    }

    #[test]
    fn hello_response_contains_protocol_version() {
        let response = Response::ok(
            "one".to_owned(),
            "hello",
            serde_json::json!({ "protocol": PROTOCOL_VERSION }),
        );
        assert!(response.to_json_line().contains("\"protocol\":1"));
    }

    #[test]
    fn parses_process_image_request() {
        let request: Request = serde_json::from_str(
            r#"{
                "type":"process_image",
                "id":"image-one",
                "engine":"waifu2x-ncnn-vulkan",
                "model":"waifu2x-upconv-anime",
                "input_path":"/tmp/input.png",
                "output_path":"/tmp/output.png",
                "args":["-i","/tmp/input.png","-o","/tmp/output.png"],
                "timeout_ms":120000
            }"#,
        )
        .expect("image request should parse");
        assert!(matches!(
            request,
            Request::ProcessImage { engine, model, .. }
                if engine == "waifu2x-ncnn-vulkan" && model == "waifu2x-upconv-anime"
        ));
    }
}
