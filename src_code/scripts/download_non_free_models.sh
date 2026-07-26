#!/bin/bash
# download_non_free_models.sh — Extract proprietary W2xEX models from
# an official Waifu2x-Extension-GUI release.
#
# This script can either:
#   a) Extract from a local copy you already downloaded:
#      ./download_non_free_models.sh ~/Downloads/Waifu2x-Extension-GUI-v3.139.01-Win64/
#      ./download_non_free_models.sh ~/Downloads/Waifu2x-Extension-GUI-v3.139.01-Win64.7z
#
#   b) Download directly from Aaron Feng's GitHub releases:
#      UPSTREAM_VERSION=v3.139.01 ./download_non_free_models.sh --fetch
#
#   c) Auto-detect latest upstream version:
#      ./download_non_free_models.sh --latest
#
# Env vars:
#   UPSTREAM_REPO     — GitHub repo (default: AaronFeng753/Waifu2x-Extension-GUI)
#   UPSTREAM_VERSION  — Release tag (default: auto-detect if --latest, else required)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODELS_DIR="$SCRIPT_DIR/../vendor/models-non-free"
UPSTREAM_REPO="${UPSTREAM_REPO:-AaronFeng753/Waifu2x-Extension-GUI}"

# ---- proprietary model manifest ----
# These are custom-trained by Aaron Feng. Do NOT redistribute.
PROPRIETARY_MODELS=(
    "realesrgan-ncnn-vulkan/models/Anime-HQ-W4xEX.bin"
    "realesrgan-ncnn-vulkan/models/Anime-HQ-W4xEX.param"
    "realesrgan-ncnn-vulkan/models/AnimeVideo-MiniV1.8-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/AnimeVideo-MiniV1.8-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Omni-MiniV2-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/Omni-MiniV2-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Omni-Smallv2-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/Omni-Smallv2-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Omni-TurboV1.5-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/Omni-TurboV1.5-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Photo-HQ-W4xEX.bin"
    "realesrgan-ncnn-vulkan/models/Photo-HQ-W4xEX.param"
    "realesrgan-ncnn-vulkan/models/Photo-Small-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/Photo-Small-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Universal-FastV2-W2xEX.bin"
    "realesrgan-ncnn-vulkan/models/Universal-FastV2-W2xEX.param"
    "realesrgan-ncnn-vulkan/models/Photo-Conservative-x4.bin"
    "realesrgan-ncnn-vulkan/models/Photo-Conservative-x4.param"
)

# ---------------------------------------------------
usage() {
    echo "Usage: $0 [--latest | --fetch | <path-to-official-release>]"
    echo ""
    echo "Options:"
    echo "  --latest       Auto-detect and download the latest upstream release"
    echo "  --fetch        Download a specific version (set UPSTREAM_VERSION env var)"
    echo "  <path>         Extract from a local release directory, .7z, or .zip"
    echo ""
    echo "Examples:"
    echo "  $0 ~/Downloads/Waifu2x-Extension-GUI-v3.139.01-Win64.7z"
    echo "  UPSTREAM_VERSION=v3.139.01 $0 --fetch"
    echo "  $0 --latest"
    echo ""
    echo "Upstream: https://github.com/${UPSTREAM_REPO}/releases"
    exit 1
}

fetch_upstream_version() {
    local mode="$1"  # "latest" or "specific"
    local version=""

    if [ "$mode" = "latest" ]; then
        echo "Fetching latest release tag from $UPSTREAM_REPO..."
        version=$(curl -sL "https://api.github.com/repos/${UPSTREAM_REPO}/releases/latest" | grep -oP '"tag_name":\s*"\K[^"]+')
        if [ -z "$version" ]; then
            echo "ERROR: Could not determine latest version from GitHub API."
            echo "Try: UPSTREAM_VERSION=v3.139.01 $0 --fetch"
            exit 1
        fi
        echo "Latest upstream version: $version"
        UPSTREAM_VERSION="$version"
    else
        UPSTREAM_VERSION="${UPSTREAM_VERSION:-}"
    fi

    if [ -z "$UPSTREAM_VERSION" ]; then
        echo "ERROR: Set UPSTREAM_VERSION env var, e.g.:"
        echo "  UPSTREAM_VERSION=v3.139.01 $0 --fetch"
        exit 1
    fi

    # Asset naming pattern from Aaron Feng's releases:
    #   Waifu2x-Extension-GUI-v{VERSION}-Win64.7z
    local asset="Waifu2x-Extension-GUI-${UPSTREAM_VERSION}-Win64.7z"
    local url="https://github.com/${UPSTREAM_REPO}/releases/download/${UPSTREAM_VERSION}/${asset}"

    echo "Downloading: $url"
    WORKDIR=$(mktemp -d)

    if command -v curl &>/dev/null; then
        curl -L --progress-bar -o "$WORKDIR/${asset}" "$url"
    elif command -v wget &>/dev/null; then
        wget -q --show-progress -O "$WORKDIR/${asset}" "$url"
    else
        echo "ERROR: Need curl or wget."
        exit 1
    fi

    echo "Extracting..."
    7z x "$WORKDIR/${asset}" -o"$WORKDIR" >/dev/null
    BUNDLE_DIR=$(find "$WORKDIR" -maxdepth 3 -type d -name "waifu2x-extension-gui" | head -1)
    if [ -z "$BUNDLE_DIR" ]; then
        echo "ERROR: Could not find waifu2x-extension-gui/ in archive."
        rm -rf "$WORKDIR"
        exit 1
    fi
    copy_models "$BUNDLE_DIR"
    rm -rf "$WORKDIR"
}

copy_models() {
    local bundle="$1"
    echo "Copying $(( ${#PROPRIETARY_MODELS[@]} )) model files from $bundle..."
    mkdir -p "$MODELS_DIR"

    local copied=0 missing=0
    for model in "${PROPRIETARY_MODELS[@]}"; do
        local src="$bundle/$model"
        local dst="$MODELS_DIR/$model"
        mkdir -p "$(dirname "$dst")"
        if [ -f "$src" ]; then
            cp "$src" "$dst"
            ((copied++))
        else
            echo "  NOT FOUND: $model"
            ((missing++))
        fi
    done

    echo ""
    echo "Done. $copied of $(( ${#PROPRIETARY_MODELS[@]} )) files copied to $MODELS_DIR/"
    if [ "$missing" -gt 0 ]; then
        echo "WARNING: $missing files not found. Check the release version."
    fi
    echo ""
    echo "============================================================"
    echo "  IMPORTANT: These models are PROPRIETARY (Aaron Feng)."
    echo "  Your use is governed by the license of the release you"
    echo "  obtained them from. Do NOT redistribute them."
    echo "============================================================"
}

# ---- main ----

# Already done?
if [ -d "$MODELS_DIR" ] && [ "$(ls -A "$MODELS_DIR" 2>/dev/null)" ] && [ "${1:-}" != "--fetch" ] && [ "${1:-}" != "--latest" ]; then
    echo "Models already present at $MODELS_DIR ($(find "$MODELS_DIR" -type f | wc -l) files)"
    echo "Delete to force re-download."
    exit 0
fi

case "${1:-}" in
    --latest)
        fetch_upstream_version "latest"
        ;;
    --fetch)
        fetch_upstream_version "specific"
        ;;
    ""|-h|--help)
        usage
        ;;
    *)
        # Local path
        SOURCE="$1"
        WORKDIR=""
        if [ -d "$SOURCE" ]; then
            BUNDLE_DIR="$SOURCE"
        elif [[ "$SOURCE" == *.7z ]]; then
            WORKDIR=$(mktemp -d)
            echo "Extracting 7z archive..."
            7z x "$SOURCE" -o"$WORKDIR" >/dev/null
            BUNDLE_DIR=$(find "$WORKDIR" -maxdepth 3 -type d -name "waifu2x-extension-gui" | head -1)
            if [ -z "$BUNDLE_DIR" ]; then
                echo "ERROR: waifu2x-extension-gui/ not found in archive."
                rm -rf "$WORKDIR"
                exit 1
            fi
        elif [[ "$SOURCE" == *.zip ]]; then
            WORKDIR=$(mktemp -d)
            echo "Extracting zip..."
            unzip -q "$SOURCE" -d "$WORKDIR"
            BUNDLE_DIR=$(find "$WORKDIR" -maxdepth 3 -type d -name "waifu2x-extension-gui" | head -1)
            if [ -z "$BUNDLE_DIR" ]; then
                echo "ERROR: waifu2x-extension-gui/ not found in archive."
                rm -rf "$WORKDIR"
                exit 1
            fi
        else
            echo "ERROR: Source must be a directory, .7z, or .zip."
            usage
        fi
        copy_models "$BUNDLE_DIR"
        [ -n "$WORKDIR" ] && rm -rf "$WORKDIR"
        ;;
esac
