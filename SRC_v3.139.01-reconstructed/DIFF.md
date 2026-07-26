# DIFF Manifest — Changes from v3.41.01-beta (Original) to v3.139.01-reconstructed

**Baseline:** `vendor/w2x-v3.41.01-beta/` — pristine copy of Aaron Feng's last published source  
**Reconstructed:** `Waifu2x-Extension-QT/` — evolved to match v3.139.01 binary structure  

## Files from vendor that remain IDENTICAL (27 files)

These files were NOT modified — the original v3.41.01-beta versions are used as-is:

| File | Lines |
|------|-------|
| `AnimatedPNG.cpp` | — |
| `Anime4K.cpp` | — |
| `CompatibilityTest.cpp` | — |
| `Current_File_Progress.cpp` | — |
| `CustomResolution.cpp` | — |
| `Donate.cpp` | — |
| `Finish_Action.cpp` | — |
| `Frame_Interpolation.cpp` | — |
| `Right-click_Menu.cpp` | — |
| `SystemTrayIcon.cpp` | — |
| `Web_Activities.cpp` | — |
| `checkupdate.cpp` | — |
| `files.cpp` | — |
| `gif.cpp` | — |
| `image.cpp` | — |
| `main.cpp` | — |
| `mainwindow.cpp` | — |
| `progressBar.cpp` | — |
| `realsr_ncnn_vulkan.cpp` | — |
| `settings.cpp` | — |
| `srmd-cuda.cpp` | — |
| `srmd_ncnn_vulkan.cpp` | — |
| `table.cpp` | — |
| `textBrowser.cpp` | — |
| `topsupporterslist.cpp/.h/.ui` | — |
| `video.cpp` | — |
| `waifu2x.cpp` | — |
| `waifu2x_caffe.cpp` | — |
| `waifu2x_converter.cpp` | — |
| `waifu2x_ncnn_vulkan.cpp` | — |
| All `.qrc` / `.ts` files | — |

## Modified files (3 files)

| File | Original | Reconstructed | Δ Lines | What Changed |
|------|----------|---------------|---------|--------------|
| `mainwindow.h` | 1093 | 1164 | +71 | Added 40+ method declarations, 5 new GPU lists, 7 new compat booleans, 4 new PreLoad settings |
| `mainwindow.ui` | 11227 | 11564 | +337 | Added 3 engine sub-tabs (Real-ESRGAN, Real-CUGAN, RTX Super-Res), Image Settings tab, 6 new compat test checkboxes |
| `Waifu2x-Extension-QT.pro` | 93 | 120 | +27 | Cross-platform detection, 8 new source files, C++17, platform_compat.h + engine_registry.h |

## New files (10 files, 8,246 total lines)

| File | Lines | Purpose |
|------|-------|---------|
| `realesrgan_ncnn_vulkan.cpp` | 2,317 | Real-ESRGAN engine (22 models, full integration) |
| `realcugan_ncnn_vulkan.cpp` | 2,252 | Real-CUGAN engine (42 models, full integration) |
| `rife_v4_ncnn_vulkan.cpp` | 1,102 | RIFE v4.x engine (13 model variants) |
| `ifrnet_ncnn_vulkan.cpp` | 468 | IFRNet engine (6 model variants) |
| `rtx_superres.cpp` | 685 | RTX Super-Res stub |
| `nvidia_maxine.cpp` | 781 | NVIDIA Maxine stub |
| `cain_ncnn_vulkan.cpp` | 59 | CAIN engine stub |
| `dain_ncnn_vulkan.cpp` | 56 | DAIN engine stub |
| `platform_compat.h` | 205 | Cross-platform compatibility |
| `engine_registry.h` | 321 | 179+ model weight registry |

## Root-level files (new)

| File | Purpose |
|------|---------|
| `README.md` | Project documentation |
| `Change_log.md` | Reconstruction changelog |
| `LICENSE` | AGPLv3 + permissive exception |
| `DIFF.md` | This file |

## License Boundaries

| Directory / File | License | Copyright |
|------------------|---------|-----------|
| `vendor/w2x-v3.41.01-beta/*` | AGPLv3 only | Aaron Feng (2020-2021) |
| `Waifu2x-Extension-QT/*` (original 27 files) | AGPLv3 only | Aaron Feng (2020-2021) |
| `Waifu2x-Extension-QT/*` (3 modified files) | AGPLv3 only (original portions) | Aaron Feng (2020-2021) |
| `Waifu2x-Extension-QT/*` (10 new files) | AGPLv3 + permissive exception | AIPEAC (2026) |
| `README.md`, `Change_log.md`, `LICENSE`, `DIFF.md` | Documentation | AIPEAC (2026) |

## How to Track Future Original Changes

```bash
# See what changed in the original source since the reconstruction baseline:
diff -r vendor/w2x-v3.41.01-beta/ Waifu2x-Extension-QT/ | grep "Only in vendor" | \
  awk '{print "CHANGED IN ORIGINAL: " $4}'

# Apply an original update: copy the new version to vendor, then diff against reconstructed:
cp /path/to/original/new_version.cpp vendor/w2x-v3.41.01-beta/
diff vendor/w2x-v3.41.01-beta/new_version.cpp Waifu2x-Extension-QT/new_version.cpp
```

## Statistics

| Metric | Count |
|--------|-------|
| Original vendor files | 42 |
| Reconstructed project files | 55 |
| Files preserved identical from original | 27 (plus .qrc/.ts) |
| Files modified from original | 3 |
| New files added | 10 (8,246 lines) |
| Root docs added | 4 |
| New engine integrations | 8 |
| New model weight entries | 179+ |
| Custom W2xEX models mapped | 8 |
