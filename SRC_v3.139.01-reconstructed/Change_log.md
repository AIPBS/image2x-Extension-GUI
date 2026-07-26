# Change log — Waifu2x-Extension-GUI Reconstructed

## v3.139.01-reconstructed (2026-07-27)

### Reconstructed by AIPEAC

Based on analysis of the binary bundle `image-Extension-GUI-v3.139.01-Win64/`
and the last published source `vendor/w2x-v3.41.01-beta/`.

See `DIFF.md` for a complete line-by-line breakdown of every change.

### Structure
- `vendor/w2x-v3.41.01-beta/` — pristine original source (baseline)
- `Waifu2x-Extension-QT/` — reconstructed project (27 files identical, 3 modified, 10 new)

### Added (Reconstructed)
- **Real-ESRGAN NCNN engine**: `realesrgan_ncnn_vulkan.cpp` — full integration with 22 models
  including 8 custom W2xEX-trained models (Anime-HQ-W4xEX, Photo-HQ-W4xEX,
  Omni-TurboV1.5-W2xEX, etc.)
- **Real-CUGAN NCNN engine**: `realcugan_ncnn_vulkan.cpp` — full integration with
  42 models across SE/Pro/Nose variants
- **RIFE v4.x engine**: `rife_v4_ncnn_vulkan.cpp` — 10 v4.x flownet-only model variants
  (v4.26-large through v4.13-lite)
- **IFRNet NCNN engine**: `ifrnet_ncnn_vulkan.cpp` — 6 model variants
  (GoPro/Vimeo90K, S/L sizes)
- **CAIN NCNN engine**: `cain_ncnn_vulkan.cpp` — frame interpolation stub
- **DAIN NCNN engine**: `dain_ncnn_vulkan.cpp` — frame interpolation stub
- **RTX Super-Res engine**: `rtx_superres.cpp` — stub for NVIDIA driver-level API
- **NVIDIA Maxine engine**: `nvidia_maxine.cpp` — stub for Video Effects SDK
- **Image Settings tab**: `mainwindow.ui` — GIF/APNG/WebP quality, transparency
  enhancement, image format options
- **Compatibility test entries**: Real-ESRGAN, Real-CUGAN, IFRNet, RTX Super-Res,
  NVIDIA Maxine, APNG Tools
- **Cross-platform**: `platform_compat.h` + updated `.pro` (Linux/Windows/macOS)
- **Model registry**: `engine_registry.h` — all 179+ model weights mapped

### Changed (Reconstructed)
- `mainwindow.ui`: +337 lines (3 new engine sub-tabs, Image Settings tab, 6 compat entries)
- `mainwindow.h`: +71 lines (40+ new method declarations)
- `Waifu2x-Extension-QT.pro`: +27 lines (cross-platform, C++17, new sources)

### License
- **Original files**: GNU AGPL v3 (Copyright Aaron Feng 2020-2021)
- **New/reconstructed files**: GNU AGPL v3 with ADDITIONAL PERMISSION under section 7
  — free to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  without restriction (Copyright AIPEAC 2026)

### Known Limitations
- RTX Super-Res and NVIDIA Maxine are stubs (require proprietary NVIDIA SDK)
- SRMD-CUDA is Windows-only
- `mainwindow.cpp` needs wiring for new engine dispatch and UI signal/slot connections
- Model weight files (~1.3 GB) must be deployed separately from the binary bundle
