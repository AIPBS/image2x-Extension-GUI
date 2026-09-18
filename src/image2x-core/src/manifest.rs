// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

use serde::Serialize;

#[derive(Clone, Debug, Serialize)]
pub struct ModelRecord {
    pub name: &'static str,
    pub engine: &'static str,
    pub model: &'static str,
    pub distribution: &'static str,
}

pub fn public_model_records() -> Vec<ModelRecord> {
    let mut records = vec![
        ModelRecord {
            name: "waifu2x-cunet",
            engine: "waifu2x-ncnn-vulkan",
            model: "models-cunet",
            distribution: "open-weight",
        },
        ModelRecord {
            name: "waifu2x-upconv-anime",
            engine: "waifu2x-ncnn-vulkan",
            model: "models-upconv_7_anime_style_art_rgb",
            distribution: "open-weight",
        },
        ModelRecord {
            name: "waifu2x-upconv-photo",
            engine: "waifu2x-ncnn-vulkan",
            model: "models-upconv_7_photo",
            distribution: "open-weight",
        },
        ModelRecord {
            name: "srmd",
            engine: "srmd-ncnn-vulkan",
            model: "models-srmd",
            distribution: "open-weight",
        },
        ModelRecord {
            name: "realsr-df2k",
            engine: "realsr-ncnn-vulkan",
            model: "models-DF2K",
            distribution: "open-weight",
        },
        ModelRecord {
            name: "realsr-df2k-jpeg",
            engine: "realsr-ncnn-vulkan",
            model: "models-DF2K_JPEG",
            distribution: "open-weight",
        },
    ];
    for name in [
        "realesr-animevideov3-x2",
        "realesr-animevideov3-x3",
        "realesr-animevideov3-x4",
        "realesrgan-x4plus-anime",
        "RealESRGANv2-animevideo-xsx2",
        "RealESRGANv2-animevideo-xsx4",
        "realesrgan-x4plus",
        "realesr-general-x4v3",
        "realesr-general-wdn-x4v3",
    ] {
        records.push(ModelRecord {
            name,
            engine: "realesrgan-ncnn-vulkan",
            model: name,
            distribution: "open-weight",
        });
    }
    for (name, model) in [
        ("realcugan-se-up2x-conservative", "models-se"),
        ("realcugan-se-up2x-denoise1x", "models-se"),
        ("realcugan-se-up2x-denoise2x", "models-se"),
        ("realcugan-se-up2x-denoise3x", "models-se"),
        ("realcugan-se-up2x-no-denoise", "models-se"),
        ("realcugan-se-up3x-conservative", "models-se"),
        ("realcugan-se-up3x-denoise3x", "models-se"),
        ("realcugan-se-up3x-no-denoise", "models-se"),
        ("realcugan-se-up4x-conservative", "models-se"),
        ("realcugan-se-up4x-denoise3x", "models-se"),
        ("realcugan-se-up4x-no-denoise", "models-se"),
        ("realcugan-pro-up2x-conservative", "models-pro"),
        ("realcugan-pro-up2x-denoise3x", "models-pro"),
        ("realcugan-pro-up2x-no-denoise", "models-pro"),
        ("realcugan-pro-up3x-conservative", "models-pro"),
        ("realcugan-pro-up3x-denoise3x", "models-pro"),
        ("realcugan-pro-up3x-no-denoise", "models-pro"),
        ("realcugan-nose-up2x-no-denoise", "models-nose"),
    ] {
        records.push(ModelRecord {
            name,
            engine: "realcugan-ncnn-vulkan",
            model,
            distribution: "open-weight",
        });
    }
    records
}

pub fn model_records() -> Vec<ModelRecord> {
    let mut records = public_model_records();
    for name in [
        "Anime-HQ-W4xEX",
        "AnimeVideo-MiniV1.8-W2xEX",
        "Omni-MiniV2-W2xEX",
        "Omni-Smallv2-W2xEX",
        "Omni-TurboV1.5-W2xEX",
        "Photo-HQ-W4xEX",
        "Photo-Small-W2xEX",
        "Universal-FastV2-W2xEX",
        "Photo-Conservative-x4",
    ] {
        records.push(ModelRecord {
            name,
            engine: "realesrgan-ncnn-vulkan",
            model: name,
            distribution: "proprietary",
        });
    }
    records
}

#[cfg(test)]
mod tests {
    use super::{model_records, public_model_records};

    #[test]
    fn public_matrix_has_thirty_three_records() {
        assert_eq!(public_model_records().len(), 33);
        assert_eq!(model_records().len(), 42);
    }
}
