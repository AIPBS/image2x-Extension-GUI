#!/bin/bash
# download_models.sh — Download open-source model weights
#
# Downloads the model package from GitHub Releases of this project.
# Set these env vars (or edit defaults below):
#   MODELS_REPO       — GitHub repo (default: auto-detect from git remote)
#   MODELS_RELEASE    — Release tag (default: v0.1.0-models)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MODELS_DIR="$SCRIPT_DIR/../vendor/models-open-source"

# ---- defaults (override via env) ----
MODELS_RELEASE="${MODELS_RELEASE:-}"
UPSTREAM_REPO="${UPSTREAM_REPO:-AaronFeng753/Waifu2x-Extension-GUI}"

# Try to auto-detect our repo from git remote
detect_repo() {
    local remote
    remote=$(git -C "$SCRIPT_DIR/.." remote get-url origin 2>/dev/null || true)
    if [[ "$remote" =~ github\.com[:/]([^/]+/[^/]+?)(\.git)?$ ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo ""
    fi
}

MODELS_REPO="${MODELS_REPO:-$(detect_repo)}"
if [ -z "$MODELS_REPO" ]; then
    echo "ERROR: Could not detect GitHub repo. Set MODELS_REPO env var."
    echo "  export MODELS_REPO=yourname/yourrepo"
    exit 1
fi

# Determine release tag from project version if not set
if [ -z "$MODELS_RELEASE" ]; then
    VERSION_FILE="$SCRIPT_DIR/../Waifu2x-Extension-QT/mainwindow.h"
    if [ -f "$VERSION_FILE" ]; then
        VERSION=$(grep -oP 'VERSION\s*=\s*"v[^"]*"' "$VERSION_FILE" | grep -oP 'v[^"]*')
        MODELS_RELEASE="${VERSION}-models"
    fi
fi
if [ -z "$MODELS_RELEASE" ]; then
    echo "ERROR: Could not determine release tag. Set MODELS_RELEASE env var."
    echo "  export MODELS_RELEASE=v0.1.0-models"
    exit 1
fi

# Already downloaded?
if [ -d "$MODELS_DIR" ] && [ "$(ls -A "$MODELS_DIR" 2>/dev/null)" ]; then
    echo "Models already present at $MODELS_DIR ($(find "$MODELS_DIR" -type f | wc -l) files)"
    echo "Delete this directory to force re-download."
    exit 0
fi

ARCHIVE="w2x-models-${MODELS_RELEASE}-open-source.tar.gz"
DOWNLOAD_URL="https://github.com/${MODELS_REPO}/releases/download/${MODELS_RELEASE}/${ARCHIVE}"

# Also support: direct from upstream if no own release yet
FALLBACK_URL="https://github.com/${UPSTREAM_REPO}/releases/latest"

echo "============================================"
echo " Downloading open-source model weights"
echo "============================================"
echo " Repo:    $MODELS_REPO"
echo " Release: $MODELS_RELEASE"
echo " URL:     $DOWNLOAD_URL"
echo " Dest:    $MODELS_DIR"
echo "============================================"

mkdir -p "$MODELS_DIR"

# Try primary URL first, then offer fallback instructions
HTTP_CODE=$(curl -sLI -o /dev/null -w '%{http_code}' "$DOWNLOAD_URL" 2>/dev/null || echo "000")

if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "302" ]; then
    echo "Downloading..."
    curl -L --progress-bar -o "/tmp/${ARCHIVE}" "$DOWNLOAD_URL"
elif command -v gh &>/dev/null && gh auth status &>/dev/null 2>&1; then
    echo "Primary URL not found. Trying gh CLI..."
    gh release download "$MODELS_RELEASE" -R "$MODELS_REPO" -p "*.tar.gz" -O "/tmp/${ARCHIVE}" 2>/dev/null || {
        echo ""
        echo "ERROR: Could not download models."
        echo ""
        echo "You need to either:"
        echo "  1. Upload the model package as a GitHub Release:"
        echo "     gh release create ${MODELS_RELEASE} /tmp/w2x-models-*.tar.gz"
        echo "  2. Or download models manually from the official v3.139.01 release:"
        echo "     (open-source engines only, no proprietary W2xEX models)"
        echo "     URL: $FALLBACK_URL"
        exit 1
    }
else
    echo ""
    echo "ERROR: Release '$MODELS_RELEASE' not found on $MODELS_REPO."
    echo ""
    echo "Create it with:"
    echo "  gh release create ${MODELS_RELEASE} /tmp/w2x-models-*.tar.gz"
    echo ""
    echo "Or: set MODELS_RELEASE to an existing tag, e.g.:"
    echo "  export MODELS_RELEASE=v0.1.0-models"
    exit 1
fi

echo "Extracting..."
tar -xzf "/tmp/${ARCHIVE}" -C "$MODELS_DIR" --strip-components=1
rm -f "/tmp/${ARCHIVE}"

echo ""
echo "Done. $(find "$MODELS_DIR" -type f | wc -l) model files ready."
