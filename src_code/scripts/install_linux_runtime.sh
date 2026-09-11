#!/usr/bin/env bash
# Installs the supported Linux Waifu2x NCNN Vulkan runtime and all of its
# open-source model directories. The release is pinned and checksum-verified
# so the application and test workspace use the same known engine layout.

set -euo pipefail

readonly VERSION="20250915"
readonly ARCHIVE="waifu2x-ncnn-vulkan-${VERSION}-linux.zip"
readonly URL="https://github.com/nihui/waifu2x-ncnn-vulkan/releases/download/${VERSION}/${ARCHIVE}"
readonly SHA256="848e0fba55657d34da90b775b8139e9806dc754798b029f95e106ba8850a731f"

if [[ "${1:-}" == "" || "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
    printf 'Usage: %s <application-directory>\n' "$0"
    exit 2
fi

APPLICATION_DIRECTORY="$(realpath "$1")"
ENGINE_DIRECTORY="${APPLICATION_DIRECTORY}/dependencies/engines/waifu2x-ncnn-vulkan"

if [[ ! -d "$APPLICATION_DIRECTORY" ]]; then
    printf 'Application directory does not exist: %s\n' "$APPLICATION_DIRECTORY" >&2
    exit 1
fi

if [[ -e "$ENGINE_DIRECTORY" ]]; then
    printf 'Runtime already exists at %s. Remove it explicitly before reinstalling.\n' "$ENGINE_DIRECTORY" >&2
    exit 1
fi

for command in curl sha256sum unzip; do
    if ! command -v "$command" >/dev/null 2>&1; then
        printf 'Required command is unavailable: %s\n' "$command" >&2
        exit 1
    fi
done

WORK_DIRECTORY="$(mktemp -d)"
trap 'rm -rf "$WORK_DIRECTORY"' EXIT

ARCHIVE_PATH="${WORK_DIRECTORY}/${ARCHIVE}"
EXTRACT_DIRECTORY="${WORK_DIRECTORY}/extract"

printf 'Downloading waifu2x-ncnn-vulkan %s...\n' "$VERSION"
curl --fail --location --progress-bar --output "$ARCHIVE_PATH" "$URL"
printf '%s  %s\n' "$SHA256" "$ARCHIVE_PATH" | sha256sum --check --status

mkdir -p "$EXTRACT_DIRECTORY"
unzip -q "$ARCHIVE_PATH" -d "$EXTRACT_DIRECTORY"

shopt -s nullglob
entries=("${EXTRACT_DIRECTORY}"/*)
if [[ "${#entries[@]}" -ne 1 || ! -d "${entries[0]}" ]]; then
    printf 'Unexpected archive layout in %s\n' "$ARCHIVE_PATH" >&2
    exit 1
fi

required_models=(
    "models-cunet"
    "models-upconv_7_anime_style_art_rgb"
    "models-upconv_7_photo"
)
if [[ ! -x "${entries[0]}/waifu2x-ncnn-vulkan" ]]; then
    printf 'Archive does not contain the expected executable and models.\n' >&2
    exit 1
fi
for model_directory in "${required_models[@]}"; do
    if [[ ! -d "${entries[0]}/${model_directory}" ]]; then
        printf 'Archive is missing required model directory: %s\n' "${model_directory}" >&2
        exit 1
    fi
done

mkdir -p "$(dirname "$ENGINE_DIRECTORY")"
mv "${entries[0]}" "$ENGINE_DIRECTORY"
printf 'Installed Linux runtime at %s\n' "$ENGINE_DIRECTORY"
