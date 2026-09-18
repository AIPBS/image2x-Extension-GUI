// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

use std::path::{Path, PathBuf};

use serde::Serialize;

#[derive(Clone, Debug, Serialize)]
pub struct RuntimeRecord {
    pub engine: &'static str,
    pub directory: PathBuf,
    pub executable: PathBuf,
    pub missing: Vec<PathBuf>,
    pub available: bool,
}

struct RuntimeDefinition {
    engine: &'static str,
    executable: &'static str,
    models: &'static [&'static str],
}

const RUNTIMES: &[RuntimeDefinition] = &[
    RuntimeDefinition {
        engine: "waifu2x-ncnn-vulkan",
        executable: "waifu2x-ncnn-vulkan",
        models: &[
            "models-cunet",
            "models-upconv_7_anime_style_art_rgb",
            "models-upconv_7_photo",
        ],
    },
    RuntimeDefinition {
        engine: "srmd-ncnn-vulkan",
        executable: "srmd-ncnn-vulkan",
        models: &["models-srmd"],
    },
    RuntimeDefinition {
        engine: "realsr-ncnn-vulkan",
        executable: "realsr-ncnn-vulkan",
        models: &["models-DF2K", "models-DF2K_JPEG"],
    },
    RuntimeDefinition {
        engine: "realesrgan-ncnn-vulkan",
        executable: "realesrgan-ncnn-vulkan",
        models: &["models"],
    },
    RuntimeDefinition {
        engine: "realcugan-ncnn-vulkan",
        executable: "realcugan-ncnn-vulkan",
        models: &["models-se", "models-pro", "models-nose"],
    },
];

pub fn runtime_records(application_directory: &Path) -> Vec<RuntimeRecord> {
    RUNTIMES
        .iter()
        .map(|definition| {
            let directory = application_directory
                .join("dependencies")
                .join("engines")
                .join(definition.engine);
            let executable = directory.join(definition.executable);
            let mut missing = Vec::new();
            if !directory.is_dir() {
                missing.push(directory.clone());
            }
            if !executable.is_file() || !is_executable(&executable) {
                missing.push(executable.clone());
            }
            for model in definition.models {
                let model_path = directory.join(model);
                let model_pair = directory.join(format!("{model}.param")).is_file()
                    && directory.join(format!("{model}.bin")).is_file();
                if !model_path.is_dir() && !model_pair {
                    missing.push(model_path);
                }
            }
            RuntimeRecord {
                engine: definition.engine,
                directory,
                executable,
                available: missing.is_empty(),
                missing,
            }
        })
        .collect()
}

#[cfg(unix)]
fn is_executable(path: &Path) -> bool {
    use std::os::unix::fs::PermissionsExt;

    std::fs::metadata(path)
        .map(|metadata| metadata.permissions().mode() & 0o111 != 0)
        .unwrap_or(false)
}

#[cfg(not(unix))]
fn is_executable(path: &Path) -> bool {
    path.is_file()
}

#[cfg(test)]
mod tests {
    use super::runtime_records;
    use std::path::Path;

    #[test]
    fn missing_application_reports_all_public_runtimes() {
        let records = runtime_records(Path::new("/does/not/exist/image2x"));
        assert_eq!(records.len(), 5);
        assert!(records.iter().all(|record| !record.available));
    }
}
