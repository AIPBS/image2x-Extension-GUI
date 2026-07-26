#!/bin/bash
# download_models.sh — Download open-source model weights for Waifu2x-Extension-GUI
#
# This script downloads the model package from GitHub Releases.
# Upload the tarball as a release asset first:
#   gh release upload v3.139.01-models /tmp/w2x-models-v3.139.01-open-source.tar.gz
#
# Then this script downloads and extracts it into vendor/models-open-source/

set -euo pipefail

RELEASE_TAG="v3.139.01-models"
ARCHIVE="w2x-models-v3.139.01-open-source.tar.gz"
MODELS_DIR="$(dirname "$0")/../vendor/models-open-source"

# GitHub repo — update this to your actual repo
REPO="${GITHUB_REPOSITORY:-YOUR_USER/YOUR_REPO}"

DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${RELEASE_TAG}/${ARCHIVE}"

if [ -d "$MODELS_DIR" ] && [ "$(ls -A "$MODELS_DIR" 2>/dev/null)" ]; then
    echo "Models already present at $MODELS_DIR"
    echo "Delete this directory to force re-download."
    exit 0
fi

echo "Downloading open-source models (349 files, ~1.2 GB uncompressed)..."
echo "URL: $DOWNLOAD_URL"

mkdir -p "$MODELS_DIR"

if command -v curl &>/dev/null; then
    curl -L --progress-bar -o "/tmp/${ARCHIVE}" "$DOWNLOAD_URL"
elif command -v wget &>/dev/null; then
    wget -q --show-progress -O "/tmp/${ARCHIVE}" "$DOWNLOAD_URL"
else
    echo "ERROR: Neither curl nor wget found. Install one of them."
    exit 1
fi

echo "Extracting..."
tar -xzf "/tmp/${ARCHIVE}" -C "$MODELS_DIR" --strip-components=1
rm "/tmp/${ARCHIVE}"

echo "Done. $(find "$MODELS_DIR" -type f | wc -l) model files extracted."
