# Waifu2x-Extension-GUI v3.139.01 — Reconstructed

Cross-platform source code reconstruction of Waifu2x-Extension-GUI by Aaron Feng.

## Project Layout

```
SRC_v3.139.01-reconstructed/
├── vendor/
│   └── w2x-v3.41.01-beta/          ← PRISTINE original source (never modified)
│       ├── main.cpp                    Use this as the diff baseline
│       ├── mainwindow.h                to track what changed.
│       ├── mainwindow.ui
│       ├── ... (42 files, AGPLv3)
│       └── Waifu2x-Extension-QT.pro
├── Waifu2x-Extension-QT/           ← RECONSTRUCTED project (extends vendor)
│   ├── Waifu2x-Extension-QT.pro        Cross-platform, references vendor
│   ├── mainwindow.h                    Modified: +71 lines (new declarations)
│   ├── mainwindow.ui                   Modified: +337 lines (new tabs)
│   ├── realesrgan_ncnn_vulkan.cpp      NEW: 2,317 lines
│   ├── realcugan_ncnn_vulkan.cpp       NEW: 2,252 lines
│   ├── rife_v4_ncnn_vulkan.cpp         NEW: 1,102 lines
│   ├── ... (27 original files identical to vendor)
│   └── ... (10 new files total, 8,246 lines)
├── DIFF.md                          ← Complete delta from original
├── Change_log.md
├── README.md
└── LICENSE                          ← AGPLv3 + permissive exception
```

## Keeping in Sync with the Original

The `vendor/` directory holds the **pristine v3.41.01-beta** source. To see what changed:

```bash
# What files differ from the original?
diff -rq vendor/w2x-v3.41.01-beta/ Waifu2x-Extension-QT/

# How much did each file change? (see DIFF.md for the full breakdown)
wc -l vendor/w2x-v3.41.01-beta/*.cpp Waifu2x-Extension-QT/*.cpp
```

When the original author publishes new source code, replace `vendor/` with the
new version and re-run the diff to see what needs updating in the reconstruction.

## Origin

This project reconstructs the v3.139.01 source code from two sources:
- **`SRC_v3.41.01-beta/`** — the last published source code (2021, originally AGPLv3)
- **`image-Extension-GUI-v3.139.01-Win64/`** — the binary release bundle (2026, proprietary)

The original author switched from AGPLv3 to a proprietary "personal use only"
license on **2021-05-02**, stopped publishing source code, and developed 71+
binary-only releases privately. This reconstruction bridges the gap by
deriving the v3.139.01 structure from the binary bundle and changelog.

## License

### Two-Tier Licensing

| Files | License | Copyright |
|-------|---------|-----------|
| Original files from v3.41.01-beta | **GNU AGPL v3 only** | Aaron Feng (2020-2021) |
| New/reconstructed files (2026) | **AGPL v3 + permissive exception** | AIPEAC (2026) |

The permissive exception (AGPLv3 section 7) allows you to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell the new files without
restriction — effectively a dual AGPL/MIT-like license for those files.

Files bearing the `ADDITIONAL PERMISSION` notice in their header are dual-licensed.
Files without it (the original Aaron Feng files) remain strictly AGPLv3.

See [LICENSE](LICENSE) for details.

## What's Reconstructed

### Source Code
| Component | Files | Notes |
|-----------|-------|-------|
| Original engine integrations (7 engines) | 30 .cpp/.h/.ui | Preserved from v3.41.01-beta |
| Real-ESRGAN NCNN engine | `realesrgan_ncnn_vulkan.cpp` | 22 models, full integration |
| Real-CUGAN NCNN engine | `realcugan_ncnn_vulkan.cpp` | 42 models, full integration |
| RIFE v4.x engine | `rife_v4_ncnn_vulkan.cpp` | 13 model variants |
| IFRNet NCNN engine | `ifrnet_ncnn_vulkan.cpp` | 6 model variants |
| CAIN/DAIN engines | `cain_/dain_ncnn_vulkan.cpp` | Stub integrations |
| RTX Super-Res | `rtx_superres.cpp` | Stub (driver API) |
| NVIDIA Maxine | `nvidia_maxine.cpp` | Stub (Windows SDK) |
| Cross-platform header | `platform_compat.h` | Linux/Windows/macOS |
| Model registry | `engine_registry.h` | 179+ model mappings |
| Evolved UI | `mainwindow.ui` | 3 new engine sub-tabs, Image Settings tab, 6 new compat test entries |
| Build config | `Waifu2x-Extension-QT.pro` | C++17, cross-platform |

### Model Weights (Deploy Separately)
The model weights from the v3.139.01 binary bundle (~1.3 GB, 179+ files):

| Engine | Custom W2xEX Models | Standard Models |
|--------|--------------------|-----------------|
| Real-ESRGAN | 8 (Anime-HQ-W4xEX, Omni-TurboV1.5, etc.) | 14 |
| Real-CUGAN | 0 | 42 |
| RIFE | 0 | 75 |
| IFRNet | 0 | 12 |
| CAIN/DAIN | 0 | 10 |
| Others (w2x/SRMD/RealSR/Anime4K) | 0 | 60+ |

## Building

### Prerequisites
- Qt 5.15+ (core, gui, widgets, concurrent, multimedia)
- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- Linux: X11 development headers

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

### macOS
```bash
mkdir build && cd build
qmake ../Waifu2x-Extension-QT/Waifu2x-Extension-QT.pro
make -j$(sysctl -n hw.ncpu)
```

## Deploying Engine Binaries and Models

The engine executables and model weights from the v3.139.01 binary bundle
must be placed alongside the built executable. Copy these directories:

```
realesrgan-ncnn-vulkan/    (includes models/)
realcugan-ncnn-vulkan/     (includes models-se/pro/nose/)
rife-ncnn-vulkan/          (includes 19 model variants)
ifrnet-ncnn-vulkan/        (includes 6 model variants)
cain-ncnn-vulkan/
dain-ncnn-vulkan/
waifu2x-ncnn-vulkan/       (existing)
waifu2x-caffe/             (existing)
waifu2x-converter/         (existing)
srmd-ncnn-vulkan/          (existing)
srmd-cuda/                 (existing)
realsr-ncnn-vulkan/        (existing)
Anime4K/                   (existing)
Real-CUGAN-Caffe/          (existing)
models/                    (Anime4K ONNX models)
NVIDIA-Maxine/             (Windows only)
ffmpeg_waifu2xEX / ffprobe_waifu2xEX / SoX / libheif / ...
```

## Limitations

- RTX Super-Res and NVIDIA Maxine are stubs (require proprietary SDK)
- SRMD-CUDA is Windows-only (CUDA wrapper)
- `mainwindow.cpp` needs new dispatch logic for added engines
- `mainwindow.h` needs declaration stubs for new engine methods
- Not a drop-in replacement — requires Qt 5.15+ build environment

## Acknowledgements

Based on Waifu2x-Extension-GUI by Aaron Feng.
https://github.com/AaronFeng753/Waifu2x-Extension-GUI

Uses open-source engines from: nihui (ncnn-vulkan ports), xinntao (Real-ESRGAN),
Bilibili (Real-CUGAN), megvii-research (RIFE), ltkong218 (IFRNet),
myungsub (CAIN), baowenbo (DAIN), and others.
