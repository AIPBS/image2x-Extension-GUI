#!/usr/bin/env bash
# Run every public still-image model in the installed Linux bundle.
set -euo pipefail

[[ "$#" -eq 2 ]] || {
    printf 'Usage: %s <application-directory> <input-png>\n' "$0"
    exit 2
}

APPLICATION_DIRECTORY="$(realpath "$1")"
INPUT_IMAGE="$(realpath "$2")"
ENGINE_DIRECTORY="${APPLICATION_DIRECTORY}/dependencies/engines"
OUTPUT_DIRECTORY="$(mktemp -d)"
RESULTS_FILE="${OUTPUT_DIRECTORY}/matrix.tsv"
FAILED=0
trap 'rm -rf "$OUTPUT_DIRECTORY"' EXIT

[[ -d "$APPLICATION_DIRECTORY" ]] || { printf 'Application directory does not exist: %s\n' "$APPLICATION_DIRECTORY" >&2; exit 1; }
[[ -f "$INPUT_IMAGE" ]] || { printf 'Input image does not exist: %s\n' "$INPUT_IMAGE" >&2; exit 1; }

valid_png() {
    python3 - "$1" <<'PY'
import struct
import sys

path = sys.argv[1]
with open(path, "rb") as stream:
    signature = stream.read(8)
    length = struct.unpack(">I", stream.read(4))[0]
    chunk = stream.read(4)
    width, height = struct.unpack(">II", stream.read(8)) if chunk == b"IHDR" and length >= 8 else (0, 0)
raise SystemExit(0 if signature == b"\x89PNG\r\n\x1a\n" and width > 0 and height > 0 else 1)
PY
}

run_case() {
    local name="$1"
    local executable="$2"
    local output="$3"
    shift 3
    local status=0
    rm -f "$output"
    if timeout --kill-after=5s 180s "$executable" "$@" -i "$INPUT_IMAGE" -o "$output" -g 0; then
        status=0
    else
        status=$?
    fi
    local result="FAIL"
    if [[ "$status" -eq 0 && -s "$output" ]] && valid_png "$output"; then
        result="PASS"
    fi
    printf '%s\t%s\t%s\t%s\n' "$result" "$name" "$status" "$output" | tee -a "$RESULTS_FILE"
    [[ "$result" == PASS ]] || FAILED=1
}

run_case_with_model() {
    local name="$1"
    local executable="$2"
    local model="$3"
    shift 3
    run_case "$name" "$executable" "${OUTPUT_DIRECTORY}/${name}.png" "$@" -m "$model"
}

W2X="${ENGINE_DIRECTORY}/waifu2x-ncnn-vulkan/waifu2x-ncnn-vulkan"
SRMD="${ENGINE_DIRECTORY}/srmd-ncnn-vulkan/srmd-ncnn-vulkan"
REALSR="${ENGINE_DIRECTORY}/realsr-ncnn-vulkan/realsr-ncnn-vulkan"
REALESRGAN="${ENGINE_DIRECTORY}/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan"
REALCUGAN="${ENGINE_DIRECTORY}/realcugan-ncnn-vulkan/realcugan-ncnn-vulkan"

run_case_with_model "waifu2x-cunet" "$W2X" "models-cunet" -s 2 -n 0 -t 32
run_case_with_model "waifu2x-upconv-anime" "$W2X" "models-upconv_7_anime_style_art_rgb" -s 2 -n 0 -t 32
run_case_with_model "waifu2x-upconv-photo" "$W2X" "models-upconv_7_photo" -s 2 -n 0 -t 32
run_case_with_model "srmd" "$SRMD" "models-srmd" -s 2 -n 0 -t 32
run_case_with_model "realsr-df2k" "$REALSR" "models-DF2K" -s 4 -t 32
run_case_with_model "realsr-df2k-jpeg" "$REALSR" "models-DF2K_JPEG" -s 4 -t 32

for model in \
    realesr-animevideov3-x2 realesr-animevideov3-x3 realesr-animevideov3-x4 \
    realesrgan-x4plus-anime RealESRGANv2-animevideo-xsx2 RealESRGANv2-animevideo-xsx4 \
    realesrgan-x4plus realesr-general-x4v3 realesr-general-wdn-x4v3; do
    run_case "realesrgan-${model}" "$REALESRGAN" "${OUTPUT_DIRECTORY}/realesrgan-${model}.png" -s 2 -n "$model" -t 32 -m models
done

while IFS=: read -r variant model scale denoise; do
    run_case "realcugan-${variant}-${model}" "$REALCUGAN" "${OUTPUT_DIRECTORY}/realcugan-${variant}-${model}.png" -s "$scale" -n "$denoise" -t 32 -m "$variant"
done <<'EOF'
models-se:up2x-conservative:2:-1
models-se:up2x-denoise1x:2:1
models-se:up2x-denoise2x:2:2
models-se:up2x-denoise3x:2:3
models-se:up2x-no-denoise:2:0
models-se:up3x-conservative:3:-1
models-se:up3x-denoise3x:3:3
models-se:up3x-no-denoise:3:0
models-se:up4x-conservative:4:-1
models-se:up4x-denoise3x:4:3
models-se:up4x-no-denoise:4:0
models-pro:up2x-conservative:2:-1
models-pro:up2x-denoise3x:2:3
models-pro:up2x-no-denoise:2:0
models-pro:up3x-conservative:3:-1
models-pro:up3x-denoise3x:3:3
models-pro:up3x-no-denoise:3:0
models-nose:up2x-no-denoise:2:0
EOF

printf 'Matrix results: %s\n' "$RESULTS_FILE"
exit "$FAILED"
