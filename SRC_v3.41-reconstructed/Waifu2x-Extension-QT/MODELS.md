# Model Inventory — License Boundaries

This file documents every model weight referenced by the reconstructed engine
integrations, with explicit license status.

## ⚠️ PROPRIETARY — Do NOT distribute

These models were custom-trained by Aaron Feng and are bundled only in the
proprietary v3.139.01 binary release. They are NOT open-source and are NOT
included in this reconstructed source tree. Attempting to distribute them
without permission may violate copyright.

| Model | Engine | Scale | Style | Size |
|-------|--------|-------|-------|------|
| Anime-HQ-W4xEX | Real-ESRGAN | 4x | 2D Anime | ~64 MB |
| Photo-HQ-W4xEX | Real-ESRGAN | 4x | 3D Photo | ~64 MB |
| AnimeVideo-MiniV1.8-W2xEX | Real-ESRGAN | 2x | 2D Anime | ~0.1 MB |
| Omni-MiniV2-W2xEX | Real-ESRGAN | 2x | 2D Anime | ~0.2 MB |
| Omni-Smallv2-W2xEX | Real-ESRGAN | 2x | 2D Anime | ~1.2 MB |
| Omni-TurboV1.5-W2xEX | Real-ESRGAN | 2x | 2D Anime | ~0.6 MB |
| Photo-Small-W2xEX | Real-ESRGAN | 2x | 3D Photo | ~2.3 MB |
| Universal-FastV2-W2xEX | Real-ESRGAN | 2x | 3D Photo | ~2.3 MB |
| Photo-Conservative-x4 | Real-ESRGAN | 4x | 3D Photo | ~32 MB |

**Total proprietary models: 9 (18 files, ~166 MB)**

The engine code (`realesrgan_ncnn_vulkan.cpp`) references these models in
`comboBox_Model_2D_RealESRGAN` and `comboBox_Model_3D_RealESRGAN` but
will gracefully skip them if the `.bin`/`.param` files are absent.

## ✅ OPEN-SOURCE — Safe to distribute

All other models in the binary bundle are from upstream open-source projects
and can be freely redistributed under their respective licenses.

| Source | Models | License |
|--------|--------|---------|
| Real-ESRGAN (xinntao) | realesrgan-x4plus, realesrgan-x4plus-anime, realesr-animevideov3, realesr-general-x4v3, etc. (14 models) | BSD-3 |
| Real-CUGAN (Bilibili) | models-se/pro/nose, 2x/3x/4x conservative/denoise/no-denoise (42 models) | MIT |
| RIFE (megvii-research, nihui) | v1.0 through v4.26-large, anime/HD/UHD (75 models) | MIT |
| IFRNet (ltkong218, nihui) | GoPro/Vimeo90K, S/L variants (12 models) | MIT |
| CAIN (myungsub, nihui) | cain.bin (2 files) | MIT |
| DAIN (baowenbo, nihui) | best/flownet + interpolation (8 files) | MIT |
| Waifu2x (nagadomi, nihui) | cunet, upconv_7_anime, upconv_7_photo (60+ files) | MIT |
| SRMD (cszn, nihui) | srmd/srmdnf x2/x3/x4 (12 files) | MIT |
| RealSR (JianZhang, nihui) | DF2K/DF2K_JPEG x4 (4 files) | MIT |
| Anime4K (bloc97) | ACNetHDNL0-3.onnx (4 files) | MIT |

**Total open-source models: ~170 models, ~860 MB**

## How the engine code handles missing models

The model registry (`engine_registry.h`) lists all models. Each engine's
`ReadSettings()` function attempts to build the model path — if the file
doesn't exist on disk, the engine call will fail with a clear error message
rather than crashing.

To use open-source models, download them from the respective upstream repos
and place them in the corresponding engine subdirectory (see `engine_registry.h`
for the exact directory per model).

## For the W2xEX models

These models are included in the official v3.139.01 binary release from Aaron Feng
(available at https://github.com/AaronFeng753/Waifu2x-Extension-GUI/releases).

If you have legally obtained that release, you can use the models with this
reconstructed build by copying them into place:

```
# From the official v3.139.01 binary bundle:
cp -r waifu2x-extension-gui/realesrgan-ncnn-vulkan/models/*W2xEX* \
     <your-build>/realesrgan-ncnn-vulkan/models/
cp -r waifu2x-extension-gui/realesrgan-ncnn-vulkan/models/*W4xEX* \
     <your-build>/realesrgan-ncnn-vulkan/models/
cp -r waifu2x-extension-gui/realesrgan-ncnn-vulkan/models/Photo-Conservative* \
     <your-build>/realesrgan-ncnn-vulkan/models/
```

This reconstructed project does NOT distribute these models. It only provides
the engine code to use them if you already have them legally. Your use of the
models is governed by the license terms of the official release you obtained
them from.
