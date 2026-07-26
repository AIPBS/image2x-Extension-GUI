#!/bin/bash
# download_models.sh — Download open-source model weights
#
# Downloads from: https://github.com/AIPBS/image2x-Extension-GUI/releases
# Release convention: open-model-set-<UPSTREAM_VERSION>
#   e.g. open-model-set-3.139.01
#
# Env vars (optional):
#   UPSTREAM_VERSION   — Aaron Feng release tag the models came from
#                        (default: auto-detect from latest open-model-set-* release)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODELS_DIR="$SCRIPT_DIR/../vendor/models-open-source"

MODELS_REPO="${MODELS_REPO:-AIPBS/image2x-Extension-GUI}"
UPSTREAM_REPO="${UPSTREAM_REPO:-AaronFeng753/Waifu2x-Extension-GUI}"

# ---- determine release tag ----
if [ -z "${UPSTREAM_VERSION:-}" ]; then
    echo "Auto-detecting latest open-model-set-* release..."
    LATEST_TAG=$(curl -sL "https://api.github.com/repos/${MODELS_REPO}/releases" 2>/dev/null | \
        grep -oP '"tag_name":\s*"\Kopen-model-set-[^"]+' | head -1)
    if [ -n "$LATEST_TAG" ]; then
        UPSTREAM_VERSION="$LATEST_TAG"
    fi
fi

if [ -z "${UPSTREAM_VERSION:-}" ]; then
    echo "ERROR: Could not find any open-model-set-* release."
    echo "Set UPSTREAM_VERSION manually:"
    echo "  UPSTREAM_VERSION=open-model-set-3.139.01 ./scripts/download_models.sh"
    exit 1
fi

MODELS_RELEASE="$UPSTREAM_VERSION"
# Extract upstream version number from tag: open-model-set-3.139.01 → v3.139.01
UPSTREAM_VER_NUM=$(echo "$UPSTREAM_VERSION" | sed 's/^open-model-set-//')
ARCHIVE="w2x-models-v${UPSTREAM_VER_NUM}-open-source.tar.gz"
DOWNLOAD_URL="https://github.com/${MODELS_REPO}/releases/download/${MODELS_RELEASE}/${ARCHIVE}"

# Already downloaded?
if [ -d "$MODELS_DIR" ] && [ "$(ls -A "$MODELS_DIR" 2>/dev/null)" ]; then
    echo "Models already present at $MODELS_DIR ($(find "$MODELS_DIR" -type f | wc -l) files)"
    echo "Delete to force re-download."
    exit 0
fi

echo "============================================"
echo " Downloading open-source model weights"
echo "============================================"
echo " Repo:    $MODELS_REPO"
echo " Release: $MODELS_RELEASE"
echo " URL:     $DOWNLOAD_URL"
echo " Dest:    $MODELS_DIR"
echo "============================================"

mkdir -p "$MODELS_DIR"

HTTP_CODE=$(curl -sLI -o /dev/null -w '%{http_code}' "$DOWNLOAD_URL" 2>/dev/null || echo "000")

if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "302" ]; then
    echo "Downloading..."
    curl -L --progress-bar -o "/tmp/${ARCHIVE}" "$DOWNLOAD_URL"
elif command -v gh &>/dev/null && gh auth status &>/dev/null 2>&1; then
    echo "Trying gh CLI..."
    gh release download "$MODELS_RELEASE" -R "$MODELS_REPO" -p "*.tar.gz" -O "/tmp/${ARCHIVE}" 2>/dev/null || {
        echo ""
        echo "ERROR: Could not download from $DOWNLOAD_URL"
        echo "Create the release with:"
        echo "  gh release create ${MODELS_RELEASE} w2x-models-*.tar.gz"
        exit 1
    }
else
    echo ""
    echo "ERROR: Release '$MODELS_RELEASE' not accessible."
    echo ""
    echo "Ensure it exists at:"
    echo "  https://github.com/${MODELS_REPO}/releases/tag/${MODELS_RELEASE}"
    echo ""
    echo "Or set a specific version:"
    echo "  UPSTREAM_VERSION=open-model-set-3.139.01 ./scripts/download_models.sh"
    exit 1
fi

echo "Extracting..."
tar -xzf "/tmp/${ARCHIVE}" -C "$MODELS_DIR" --strip-components=1
rm -f "/tmp/${ARCHIVE}"

echo "Done. $(find "$MODELS_DIR" -type f | wc -l) model files ready."
