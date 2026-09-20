# Change log

## v0.9.3 (2026-09-20)

### Changed
- Keep compatibility results scrollable so the progress bar and Start button
  remain visible.
- Align optional proprietary W2xEX model rows in three columns with the public
  compatibility results.
- Route Rust-owned CPU compatibility checks through the installed software
  Vulkan ICD while keeping llvmpipe and other software devices out of GPU
  results.
- Parse the whitespace used by Vulkan engine device reports so GPU and CPU
  compatibility results are not falsely reported as missing a device.

## v0.9.2 (2026-09-20)

### Fixed
- Allow the proprietary downloader to write directly to the persistent
  model cache instead of requiring model files in the source tree.

## v0.9.1 (2026-09-20)

### Fixed
- Keep the proprietary model section interactive when only some model files
  are missing; disable only unavailable model rows.

## v0.9.0 (2026-09-20)

### Changed
- Separate public and proprietary compatibility sections. Proprietary W2xEX
  rows now appear only when their authorized model files are detected and are
  tested through the installed Linux Real-ESRGAN runtime.
- Grey out unavailable proprietary rows and show the command needed to obtain
  the optional model files without bundling or redistributing them.
- Load translation catalogs from both standard build layouts and fall back to
  English without repeatedly showing a missing-file modal.
- Route generic Linux compatibility-engine checks through the Rust backend over
  the private Unix-socket JSON Lines protocol, while retaining Qt image decoding
  as the final artifact check.
- Remove the project-level permissive license exception. New files created by
  the Rust migration use standard GNU AGPLv3 headers without an additional
  permissive exception.
- Preserve existing Aaron Feng and AIPEAC header notices in inherited Qt files
  during the migration.

### Fixed
- Pin the regenerated public model archive checksum used by the Linux runtime
  installer.

## v0.8.2 (2026-09-18)

### Fixed
- Restore executable permissions on tested binaries before packaged release
  model validation.

## v0.8.1 (2026-09-18)

### Fixed
- Track the real `ui_routing.h` source instead of excluding it as a generated
  Qt header.
- Cache Rust build output and support private helper-release credentials in the
  release workflow.
- Cache downloaded public runtime and model archives during release packaging.

## v0.8.0 (2026-09-17)

### Added
- Bundle the native GPU support helper and offer Linux/WSL GPU diagnosis after
  a failed compatibility report.

## v0.7.0 (2026-09-17)

### Added
- Add reusable Qt logic, UI-policy, and packaged model test executables.
- Add separate regular-test and release workflows.
- Bundle the public Linux runtimes and model files in release artifacts while
  leaving device-specific Vulkan drivers to the installed system.

## v0.6.1 (2026-09-17)

### Fixed
- Show GPU and CPU compatibility controls for every engine result row.

## v0.6.0 (2026-09-17)

### Added
- Install every public Linux engine runtime required by the still-image model
  matrix from pinned, checksum-verified upstream archives.
- Add a reproducible 33-row Linux still-image model matrix runner.

## v0.5.0 (2026-09-17)

### Added
- Show separate GPU and CPU compatibility results for Vulkan engines.
- Fall back to a CPU compatibility check when the GPU check fails while
  keeping the GPU result manually selectable.
- Install every public Linux engine runtime required by the still-image model
  matrix from pinned, checksum-verified upstream archives.
- Expose the staged Linux Real-ESRGAN and Real-CUGAN image engines.
- Validate the selected Linux engine and exact model directory before processing.
- Require all three Waifu2x NCNN Vulkan model directories during Linux installation.
- Keep unsupported Real-CUGAN denoise levels disabled for each model variant.

### Fixed
- Bound Linux Compatibility Test engine waits, clean up timed-out processes, and use valid representative inputs and model arguments.

## v0.4.3 (2026-09-11)

### Fixed
- Use Qt image encoding on Linux instead of waiting for the Windows-only format converter, keeping the GUI responsive after processing.

## v0.4.2 (2026-09-11)

### Fixed
- Use the first Vulkan device for Linux Waifu2x processing because the CPU backend crashes in the supported runtime.

## v0.4.1 (2026-09-11)

### Changed
- Print Linux still-image engine launch, heartbeat, and exit diagnostics to stderr.

### Fixed
- Use Qt image encoding for Linux PNG preprocessing instead of waiting on the Windows-only converter.
- Skip the Windows-only APNG probe on Linux so still-image processing can reach the selected engine.
- Separate Linux engine arguments correctly so the selected model options are parsed by the runtime.

## v0.4.0 (2026-09-11)

### Added
- Wire Linux still-image dispatch and runtime/model validation for SRMD, RealSR, Real-ESRGAN, and Real-CUGAN.
- Add IFRNet to the frame-interpolation engine dispatch.

### Fixed
- Resolve packaged Linux engines under `dependencies/engines` while retaining the legacy flat-layout fallback.
- Use platform-correct executable names in the newly supported engine paths.

## v0.3.5 (2026-09-11)

### Added
- Show Compatibility Test results and Linux installation guidance in a large, scrollable popup window.

## v0.3.3 (2026-09-10)

### Added
- Tell Linux users which package-manager packages or application runtimes are needed when Compatibility Test checks fail.
- Install APNG tools in Linux CI so the APNG compatibility check is covered by the build environment.

## v0.3.2 (2026-09-10)

### Added
- Run Linux Compatibility Test checks for the installed RealSR, SRMD, Real-ESRGAN, Real-CUGAN, RIFE, CAIN, DAIN, and IFRNet runtimes.

### Fixed
- Report every visible Compatibility Test option and match the progress bar to all 26 checks.
- Use the CPU backend for supported Linux waifu2x processing when automatic Vulkan-device selection stalls in the test sandbox.
- Show a final Compatibility Test summary in the log so every checked and unchecked option has an explicit result.

## v0.3.1 (2026-09-10)

### Added
- Test installed FFmpeg, FFprobe, ImageMagick, Gifsicle, and SoX binaries in the Linux Compatibility Test using writable temporary inputs and outputs.

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
- **GitHub Release publishing** — push a `v*` tag (or run the workflow manually with `publish_release`) to create a Release with the Linux bundle attached. Models are not bundled; download them via `src/scripts/download_models.sh`.

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
