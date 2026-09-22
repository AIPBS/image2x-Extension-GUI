#!/usr/bin/env bash
# Test every installed Linux model/runtime on the selected hardware GPU.
set -euo pipefail

[[ "$#" -eq 2 ]] || {
    printf 'Usage: %s <application-directory> <input-png>\n' "$0"
    exit 2
}

APPLICATION_DIRECTORY="$(realpath "$1")"
INPUT_IMAGE="$(realpath "$2")"
SCRIPT_DIRECTORY="$(dirname "$(realpath "$0")")"
OUTPUT_DIRECTORY="$(mktemp -d)"
RESULTS_FILE="${OUTPUT_DIRECTORY}/matrix.tsv"
FAILED=0
trap 'rm -rf "$OUTPUT_DIRECTORY"' EXIT

[[ -d "$APPLICATION_DIRECTORY" ]] || { printf 'Application directory does not exist: %s\n' "$APPLICATION_DIRECTORY" >&2; exit 1; }
[[ -f "$INPUT_IMAGE" ]] || { printf 'Input image does not exist: %s\n' "$INPUT_IMAGE" >&2; exit 1; }

GPU_CACHE_DIRECTORY="${COMET_GPU_CACHE_DIRECTORY:-${FIREFIRE_GPU_CACHE:-}}"
if [[ -n "$GPU_CACHE_DIRECTORY" ]]; then
    [[ -f "$GPU_CACHE_DIRECTORY/icd.json" ]] || {
        printf 'GPU cache ICD does not exist: %s\n' "$GPU_CACHE_DIRECTORY/icd.json" >&2
        exit 1
    }
    export VK_ICD_FILENAMES="$GPU_CACHE_DIRECTORY/icd.json"
    export LD_LIBRARY_PATH="$GPU_CACHE_DIRECTORY/lib:/usr/lib/wsl/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

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
    if timeout --kill-after=5s 180s "$executable" "$@"; then
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

run_frame_case() {
    local name="$1"
    local executable="$2"
    local model="$3"
    shift 3
    local output="$OUTPUT_DIRECTORY/${name}.png"
    run_case "$name" "$executable" "$output" "$@" -m "$model" -g 0
}

if ! "$SCRIPT_DIRECTORY/test_linux_model_matrix.sh" "$APPLICATION_DIRECTORY" "$INPUT_IMAGE"; then
    FAILED=1
fi

ENGINE_DIRECTORY="${APPLICATION_DIRECTORY}/dependencies/engines"
REALESRGAN="${ENGINE_DIRECTORY}/realesrgan-ncnn-vulkan/realesrgan-ncnn-vulkan"
if [[ -x "$REALESRGAN" ]]; then
    for model in \
        Anime-HQ-W4xEX AnimeVideo-MiniV1.8-W2xEX Omni-MiniV2-W2xEX \
        Omni-Smallv2-W2xEX Omni-TurboV1.5-W2xEX Photo-HQ-W4xEX \
        Photo-Small-W2xEX Universal-FastV2-W2xEX Photo-Conservative-x4; do
        model_file="${ENGINE_DIRECTORY}/realesrgan-ncnn-vulkan/models/${model}.param"
        [[ -f "$model_file" ]] || continue
        run_case "proprietary-${model}" "$REALESRGAN" "${OUTPUT_DIRECTORY}/proprietary-${model}.png" \
            -i "$INPUT_IMAGE" -o "${OUTPUT_DIRECTORY}/proprietary-${model}.png" \
            -s 2 -n "$model" -t 32 -m models -g 0
    done
fi

FRAME_INPUT_DIRECTORY="${OUTPUT_DIRECTORY}/frames"
mkdir -p "$FRAME_INPUT_DIRECTORY"
cp "$INPUT_IMAGE" "${FRAME_INPUT_DIRECTORY}/frame0000.png"
cp "$INPUT_IMAGE" "${FRAME_INPUT_DIRECTORY}/frame0001.png"

RIFE="${ENGINE_DIRECTORY}/rife-ncnn-vulkan/rife-ncnn-vulkan"
if [[ -x "$RIFE" ]]; then
    for model_directory in "${ENGINE_DIRECTORY}/rife-ncnn-vulkan/"*/; do
        [[ -d "$model_directory" ]] || continue
        model="$(basename "$model_directory")"
        run_frame_case "rife-${model}" "$RIFE" "$model" \
            -0 "$INPUT_IMAGE" -1 "$INPUT_IMAGE" -o "${OUTPUT_DIRECTORY}/rife-${model}.png" -j 1:1:1
    done
fi

IFRNET="${ENGINE_DIRECTORY}/ifrnet-ncnn-vulkan/ifrnet-ncnn-vulkan"
if [[ -x "$IFRNET" ]]; then
    for model_directory in "${ENGINE_DIRECTORY}/ifrnet-ncnn-vulkan/"IFRNet*/; do
        [[ -d "$model_directory" ]] || continue
        model="$(basename "$model_directory")"
        output_directory="${OUTPUT_DIRECTORY}/ifrnet-${model}"
        mkdir -p "$output_directory"
        run_case "ifrnet-${model}" "$IFRNET" "${output_directory}/0001.png" \
            -i "$FRAME_INPUT_DIRECTORY" -o "$output_directory" -f '%04d.png' -j 1:1:1 \
            -m "$model" -g 0
    done
fi

for engine in cain-ncnn-vulkan dain-ncnn-vulkan; do
    executable="${ENGINE_DIRECTORY}/${engine}/${engine}"
    [[ -x "$executable" ]] || continue
    model=""
    for model_directory in "${ENGINE_DIRECTORY}/${engine}/"*/; do
        [[ -d "$model_directory" ]] || continue
        model="$(basename "$model_directory")"
        break
    done
    [[ -n "$model" ]] || continue
    run_frame_case "$engine" "$executable" "$model" \
        -0 "$INPUT_IMAGE" -1 "$INPUT_IMAGE" -o "${OUTPUT_DIRECTORY}/${engine}.png" -j 1:1:1
done

printf 'Matrix results: %s\n' "$RESULTS_FILE"
exit "$FAILED"
