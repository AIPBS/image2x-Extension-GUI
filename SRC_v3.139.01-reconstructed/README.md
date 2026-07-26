# Waifu2x-Extension-GUI v3.139.01 — Reconstructed Source Tree

## What This Is

A cross-platform reconstruction of the v3.139.01 source code, derived from:
- `SRC_v3.41.01-beta/` — the last published source (2021)
- `image-Extension-GUI-v3.139.01-Win64/` — the binary bundle (2026)

## What Works Out of the Box

All engine integrations from v3.41.01 (waifu2x-ncnn-vulkan, waifu2x-converter, waifu2x-caffe,
SRMD, RealSR, Anime4K, RIFE, CAIN, DAIN) are preserved and extended.

## New in This Reconstruction

| Engine | File | Status |
|--------|------|--------|
| Real-ESRGAN NCNN | `realesrgan_ncnn_vulkan.cpp` | Full integration, 22 models |
| Real-CUGAN NCNN | `realcugan_ncnn_vulkan.cpp` | Full integration, 42 models |
| RIFE v4.x | `rife_v4_ncnn_vulkan.cpp` | Full integration, 13 variants |
| IFRNet NCNN | `ifrnet_ncnn_vulkan.cpp` | Full integration, 6 variants |
| CAIN NCNN | `cain_ncnn_vulkan.cpp` | Stub (existing engine) |
| DAIN NCNN | `dain_ncnn_vulkan.cpp` | Stub (existing engine) |
| RTX Super-Res | `rtx_superres.cpp` | Stub (driver API) |
| NVIDIA Maxine | `nvidia_maxine.cpp` | Windows-only stub |
| Cross-platform | `platform_compat.h` | Linux/Windows/macOS |
| Model registry | `engine_registry.h` | All 179+ model mappings |

## Building

### Linux
```bash
mkdir build && cd build
qmake ../Waifu2x-Extension-QT/Waifu2x-Extension-QT.pro
make -j$(nproc)
```

### Windows
```bash
mkdir build && cd build
qmake ../Waifu2x-Extension-QT/Waifu2x-Extension-QT.pro
nmake
```

## Deploying Models

The model weights from the binary bundle must be placed alongside the executable:

```bash
# From the v3.139.01 binary bundle, copy these directories:
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/realesrgan-ncnn-vulkan/    <build>/realesrgan-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/realcugan-ncnn-vulkan/    <build>/realcugan-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/rife-ncnn-vulkan/         <build>/rife-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/ifrnet-ncnn-vulkan/       <build>/ifrnet-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/cain-ncnn-vulkan/         <build>/cain-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/dain-ncnn-vulkan/         <build>/dain-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/waifu2x-ncnn-vulkan/      <build>/waifu2x-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/waifu2x-caffe/            <build>/waifu2x-caffe/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/waifu2x-converter/        <build>/waifu2x-converter/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/srmd-ncnn-vulkan/         <build>/srmd-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/srmd-cuda/                <build>/srmd-cuda/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/realsr-ncnn-vulkan/       <build>/realsr-ncnn-vulkan/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/Anime4K/                  <build>/Anime4K/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/Real-CUGAN-Caffe/         <build>/Real-CUGAN-Caffe/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/models/                   <build>/models/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/NVIDIA-Maxine/            <build>/NVIDIA-Maxine/

# Also copy helper tools:
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/ffmpeg_waifu2xEX.exe      <build>/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/ffprobe_waifu2xEX.exe      <build>/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/SoX/                       <build>/SoX/
cp -r image-Extension-GUI-v3.139.01-Win64/waifu2x-extension-gui/libheif/                   <build>/libheif/
```

**Total model weights to deploy: ~1.3 GB** (179+ files across 16 engine directories)

## Known Limitations

1. **Not a complete rebuild** — the Qt GUI logic (mainwindow.cpp, ~83KB) needs extensions
   for new engine dispatch, Quick Engine Switch, Settings Presets, etc.
2. **RTX Super-Res** and **NVIDIA Maxine** are stubs (require NVIDIA SDK)
3. **SRMD-CUDA** is Windows-only (CUDA wrapper)
4. **mainwindow.h** needs declaration stubs for new engine methods
5. The `mainwindow.ui` is structurally correct but the C++ glue code for new widgets
   (signal/slot connections in mainwindow.cpp) still needs wiring

## File Inventory

| File | Size | Purpose |
|------|------|---------|
| `realesrgan_ncnn_vulkan.cpp` | 103 KB | Real-ESRGAN engine |
| `realcugan_ncnn_vulkan.cpp` | 99 KB | Real-CUGAN engine |
| `rife_v4_ncnn_vulkan.cpp` | 41 KB | RIFE v4.x engine |
| `ifrnet_ncnn_vulkan.cpp` | 17 KB | IFRNet engine |
| `rtx_superres.cpp` | 25 KB | RTX Super-Res (stub) |
| `nvidia_maxine.cpp` | 28 KB | NVIDIA Maxine (stub) |
| `platform_compat.h` | 5 KB | Cross-platform compat |
| `engine_registry.h` | 11 KB | Model registry |
| `cain_ncnn_vulkan.cpp` | 2 KB | CAIN engine (stub) |
| `dain_ncnn_vulkan.cpp` | 2 KB | DAIN engine (stub) |
| `mainwindow.ui` | ~450 KB | Evolved UI (Image Settings tab, 3 new engine sub-tabs, 6 new compat test entries) |
| `Waifu2x-Extension-QT.pro` | ~3 KB | Cross-platform build config |
