# Change log

## Unreleased

## v0.3.0 (2026-09-10)

### Fixed
- Run Linux Compatibility Test input and output files from the writable application-data directory, allowing the test to complete in the read-only Firefire application sandbox.

## v0.2.0 (2026-09-10)

### Changed
- Widen the Aspect Ratio chooser so its full option labels remain visible.

### Added
- Add a Browse button next to Output path (Folder) for selecting the output directory.
- Ask first-time users whether to run the Compatibility Test, then open and start it only when they choose Yes.

### Fixed
- Show Compatibility Test controls on their actual tab after Links is moved to the end of the tab bar.

## v0.1.4 (2026-09-10)

### Fixed
- Make Files List controls visible and usable when their inherited icon assets are unavailable.
- Use the built-in file chooser when adding files on Linux Wayland.

## v0.1.3 (2026-09-09)

### Changed
- Compatibility Test result checkboxes can now be selected manually.
- Remove the Compatibility Test separator that crossed the final engine options.
- Point the settings-panel website actions at this project's repository and hide Top Supporters.

### Fixed
- Check for updates against this project's GitHub Releases instead of upstream update files and release links.

## v0.1.2 (2026-09-08)

### Added
- Linux runtime dependency layer for the supported waifu2x-ncnn-vulkan still-image workflow.
- Checksum-verified installer for the pinned upstream Linux waifu2x-ncnn-vulkan runtime and models.

### Fixed
- Prevent an infinite startup recursion when the application cannot create or replace `settings.ini`.
- Store settings in the per-user application configuration directory so read-only installations start normally.
- Remove stale Qt auto-connect warnings while preserving the visible Real-ESRGAN tile-size controls.
- Linux jobs now fail before processing with a repair command when their supported runtime is missing, rather than waiting indefinitely for a missing engine executable.
- Center application message dialogs on the main window, and make the language prompt dismissible.
- Show the first-run language dialog after the main window is mapped so startup does not stall on Wayland.
- Remove the obsolete Japanese instruction from the English, Simplified Chinese, and Traditional Chinese language picker.
- Skip the unsupported automatic full compatibility suite on Linux so startup and shutdown do not remain stuck on its 70% progress step.
- Make the manual Linux compatibility test exercise the installed still-image runtime and always finish instead of running the unavailable Windows engine checks.
- Show the Compatibility Test controls on the correct tab after the Image Settings tab was added.
- Keep Compatibility Test separators, section headings, and results in distinct layout rows.
- Use image2x-Extension-GUI branding in the main window title and startup log.
- Replace the inherited Donate page with a final Links page for the code and upstream repositories, and remove donation prompts.

## v0.1.1 (2026-08-02)

### Added
- **GitHub Actions CI** (`.github/workflows/build.yml`) — compiles the Linux bundle on `ubuntu-latest` (Qt5), bundles Qt runtime libs + plugins so the artifact runs on machines without Qt dev packages, uploads the binary as an artifact, and runs an offscreen smoke test.
- **Self-contained bundle** — RPATH + `qt.conf` set on the binary and all bundled libs/plugins, so `./Waifu2x-Extension-GUI` runs directly without a wrapper script or `LD_LIBRARY_PATH`.
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
