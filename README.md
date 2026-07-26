# image2x — Reconstructed Waifu2x-Extension-GUI

Cross-platform rebuild derived from Aaron Feng's Waifu2x-Extension-GUI.

## Versions

| What | Version | License |
|------|---------|---------|
| **This project** | **v0.1.0** | AGPLv3 + permissive exception |
| Aaron Feng original source | v3.41.01-beta | AGPLv3 |
| Aaron Feng latest binary | v3.139.01 | Proprietary |

## Structure

```
├── vendor/                          ← Upstream baseline (not our code)
│   └── w2x-v3.41.01-beta/          ← Pristine original source (diff baseline)
├── src_code/                        ← Our project
│   ├── Waifu2x-Extension-QT/       ← Reconstructed source + new engines
│   │   ├── MODELS.md               ← Model license boundaries & inventory
│   │   ├── engine_registry.h       ← 179+ model weight mappings
│   │   ├── platform_compat.h       ← Linux / Windows / macOS
│   │   └── ...
│   ├── scripts/
│   │   ├── download_models.sh          ← Fetch open-source models from our releases
│   │   └── download_non_free_models.sh ← Extract W2xEX models from official release
│   └── .gitignore
├── README.md                        ← You are here
└── Change_log.md
```

## Building

```bash
cd src_code

# Download open-source models (one-time):
./scripts/download_models.sh

# Build:
mkdir build && cd build
qmake ../Waifu2x-Extension-QT/Waifu2x-Extension-QT.pro
make -j$(nproc)
```

## Models

**Open-source models** (349 files, MIT/BSD) — pulled from our GitHub Releases:
```bash
./scripts/download_models.sh
```
Auto-detects the latest `open-model-set-*` release from `AIPBS/image2x-Extension-GUI`.

**Proprietary W2xEX models** (18 files, Aaron Feng) — extracted from the official release:
```bash
./scripts/download_non_free_models.sh --latest
```
These are NOT redistributed. The script downloads the official release directly
from Aaron Feng's repo and extracts only the model files.

## License

| Files | License |
|-------|---------|
| Original Aaron Feng files (`vendor/w2x-v3.41.01-beta/`) | GNU AGPL v3 |
| New/reconstructed files | AGPL v3 + permissive exception |

Files with `ADDITIONAL PERMISSION` in their header can be used freely.
Files without it are strict AGPLv3.
