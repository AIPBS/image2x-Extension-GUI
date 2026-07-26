# Waifu2x-Extension-GUI — Reconstructed

Cross-platform rebuild derived from Aaron Feng's Waifu2x-Extension-GUI.

## Versions

| What | Version | License |
|------|---------|---------|
| **This project** | **v0.1.0** | AGPLv3 + permissive exception |
| Aaron Feng original (vendor baseline) | v3.41.01-beta | AGPLv3 |
| Aaron Feng latest binary (analyzed) | v3.139.01 | Proprietary |

## Structure

```
├── vendor/
│   └── w2x-v3.41.01-beta/       ← Pristine original source (diff baseline)
├── Waifu2x-Extension-QT/        ← Reconstructed project
│   ├── MODELS.md                ← Model license boundaries
│   ├── engine_registry.h        ← 179+ model mappings
│   ├── platform_compat.h        ← Linux/Windows/macOS
│   └── ... (source + new engine files)
├── scripts/
│   ├── download_models.sh       ← Pull open-source models from GitHub Releases
│   └── download_non_free_models.sh ← Extract W2xEX models from official release
├── DIFF.md                      ← Every change from original, line by line
└── Change_log.md
```

## License

Two-tier:

| Files | License |
|-------|---------|
| Original Aaron Feng files (identical to vendor) | GNU AGPL v3 only |
| New/reconstructed files | AGPL v3 + permissive exception (effectively dual MIT) |

Files with `ADDITIONAL PERMISSION` in their header: use freely.
Files without it: strict AGPLv3.

## Building

```bash
# Download open-source models first:
./scripts/download_models.sh

# Build:
mkdir build && cd build
qmake ../Waifu2x-Extension-QT/Waifu2x-Extension-QT.pro
make -j$(nproc)
```

## Proprietary models

The W2xEX custom-trained models are NOT in this repo and NOT in any release.
Use `scripts/download_non_free_models.sh` with your own legally-obtained copy
of the official v3.139.01 release to extract them.

See `Waifu2x-Extension-QT/MODELS.md` for details.
