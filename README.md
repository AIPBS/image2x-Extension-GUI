# image2x-Extension-GUI

Cross-platform rebuild derived from Aaron Feng's Waifu2x-Extension-GUI.
Renamed as image2x-Extension-GUI for distinguishing purpose, but also clearly stating that this a fork.
Created because w2x does not work well on linux.

## Versions

| What | Version | License |
|------|---------|---------|
| **This project** |  | AGPLv3 |
| Aaron Feng original source | v3.41.01-beta | AGPLv3 |
| Aaron Feng latest binary | v3.139.01 | Proprietary |

## Structure

```
├── vendor/                          ← Upstream baseline (not our code)
│   └── w2x-v3.41.01-beta/          ← Pristine original source (diff baseline)
├── src/                             ← Our project
│   ├── image2x-core/                ← Rust business core (in migration)
│   ├── image2x-ui/                  ← Qt presentation client (in migration)
│   ├── image2x-ui/                  ← Qt presentation client
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
cd src

# Download open-source models (one-time):
./scripts/download_models.sh

# Build:
mkdir build && cd build
qmake ../image2x-ui/Waifu2x-Extension-QT.pro
make -j$(nproc)

# Generate catalogs beside the executable.
for ts in ../image2x-ui/translations/language_*.ts; do
  qm="$(basename "${ts%.ts}.qm")"
  lrelease "$ts" -qm "$qm"
done
```

## Linux runtime

The Qt executable needs inference runtimes and public model files in addition
to Qt libraries. Install the pinned, checksum-verified Linux runtime bundles
for the complete public still-image matrix beside the executable:

```bash
./scripts/install_linux_runtime.sh /path/to/application-directory
```

This creates the `dependencies/engines/` tree beside the executable for
Waifu2x, SRMD, RealSR, Real-ESRGAN, Real-CUGAN, and the separately tracked
frame engines. The application validates the selected engine and exact model
directory before processing and reports this same repair command when assets
are missing.

Run the complete public still-image matrix against an installed bundle with:

```bash
./scripts/test_linux_model_matrix.sh /path/to/application-directory /path/to/input.png
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

The application scans `vendor/models-non-free/` next to the source tree. For a
build stored elsewhere, set `IMAGE2X_PROPRIETARY_MODEL_ROOT` to that directory
before launching the application.

To store proprietary models in the persistent model cache instead, set
`MODEL_OUTPUT_DIRECTORY` when running the downloader:

```bash
MODEL_OUTPUT_DIRECTORY=/var/cache/image2x-models/non-free \
  ./scripts/download_non_free_models.sh --latest
```

## License

| Files | License |
|-------|---------|
| Original Aaron Feng files (`vendor/w2x-v3.41.01-beta/`) | GNU AGPL v3 |
| New Rust and UI files | GNU AGPLv3 |

Existing inherited Qt files retain their original copyright and header notices.
New files created during the Rust migration use the standard GNU AGPLv3 header
without an additional permissive exception.
> Check out [LICENSE](LICENSE) for the governing license text.
