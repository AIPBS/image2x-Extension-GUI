# Change log

## v0.2.0 (2026-08-02)

### Added
- **GitHub Actions CI** (`.github/workflows/build.yml`) — compiles the Linux bundle on `ubuntu-latest` (Qt5), bundles Qt runtime libs + plugins so the artifact runs on machines without Qt dev packages, uploads the binary as an artifact, and runs an offscreen smoke test.
- **GitHub Release publishing** — push a `v*` tag (or run the workflow manually with `publish_release`) to create a Release with the Linux bundle attached. Models are not bundled; download them via `src_code/scripts/download_models.sh`.

## v0.1.0 (2026-07-27) — Initial reconstruction

### Baseline
- Aaron Feng `v3.41.01-beta` (AGPLv3) — last published open-source code
- Aaron Feng `v3.139.01`— check what opensource models they are current using

### Added
- **Real-ESRGAN NCNN engine** (`realesrgan_ncnn_vulkan.cpp`) — 22 models, 8 custom W2xEX mapped
- **Real-CUGAN NCNN engine** (`realcugan_ncnn_vulkan.cpp`) — 42 models, SE / Pro / Nose
- **RIFE v4.x engine** (`rife_v4_ncnn_vulkan.cpp`) — 13 model variants
- **IFRNet NCNN engine** (`ifrnet_ncnn_vulkan.cpp`) — 6 model variants
- **CAIN / DAIN NCNN engines** — stubs
- **RTX Super-Res** (`rtx_superres.cpp`) — NVIDIA driver API stub
- **NVIDIA Maxine** (`nvidia_maxine.cpp`) — Video Effects SDK stub
- **Image Settings tab** (`mainwindow.ui`) — GIF / APNG / WebP quality, transparency
- **6 new compatibility test entries** — Real-ESRGAN, Real-CUGAN, IFRNet, RTX, Maxine, APNG
- **Cross-platform** (`platform_compat.h`, updated `.pro`) — Linux / Windows / macOS, C++17
- **Model registry** (`engine_registry.h`) — 179+ model weight mappings
- **`download_models.sh`** — fetch open-source models from GitHub Releases
- **`download_non_free_models.sh`** — extract proprietary models from official release

### Modified from v3.41.01-beta
- `mainwindow.ui` (+337 lines) — engine sub-tabs, Image Settings tab, compat entries
- `mainwindow.h` (+71 lines) — 40+ new method declarations
- `Waifu2x-Extension-QT.pro` — cross-platform build, new source files

### License
- Original files: AGPLv3 (Aaron Feng 2020–2021)
- New files: AGPLv3 + permissive exception (AIPEAC 2026)
