# Change log

## v0.1.0 (2026-07-27) — Initial reconstruction

### Reconstructed from
- `vendor/w2x-v3.41.01-beta` — Aaron Feng's last published source (AGPLv3)
- `image-Extension-GUI-v3.139.01-Win64` — binary release analysis (proprietary)

### Added
- Real-ESRGAN NCNN engine (22 models, 8 custom W2xEX mapped)
- Real-CUGAN NCNN engine (42 models, SE/Pro/Nose)
- RIFE v4.x engine (13 model variants)
- IFRNet NCNN engine (6 model variants)
- CAIN / DAIN engine stubs
- RTX Super-Res stub (NVIDIA driver API)
- NVIDIA Maxine stub (Video Effects SDK)
- Image Settings tab (GIF/APNG/WebP quality, transparency)
- 6 new compatibility test entries
- Cross-platform support (Linux/Windows/macOS)
- `engine_registry.h` — 179+ model mappings
- `platform_compat.h` — platform detection, path resolution
- `download_models.sh` — open-source model retrieval
- `download_non_free_models.sh` — proprietary model extraction from official release

### Modified from v3.41.01-beta
- `mainwindow.ui`: +337 lines (engine tabs, Image Settings, compat entries)
- `mainwindow.h`: +71 lines (40+ new method declarations)
- `Waifu2x-Extension-QT.pro`: cross-platform, C++17, new sources

### License
- Original files: AGPLv3 (Aaron Feng 2020-2021)
- New files: AGPLv3 + permissive exception (AIPEAC 2026)
- See `DIFF.md` for complete file-by-file breakdown
