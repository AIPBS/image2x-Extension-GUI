#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIRECTORY="$(dirname "$(realpath "$0")")"
CHECKER="${SCRIPT_DIRECTORY}/check_vulkan_hardware_device.sh"
TEMP_DIRECTORY="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIRECTORY"' EXIT

expect_accept() {
    local label="$1"
    local line="$2"
    printf '%s\n' "$line" > "${TEMP_DIRECTORY}/runtime.log"
    if ! bash "$CHECKER" "${TEMP_DIRECTORY}/runtime.log" >/dev/null; then
        printf 'Expected hardware device accepted: %s\n' "$label" >&2
        exit 1
    fi
}

expect_reject() {
    local label="$1"
    local line="$2"
    printf '%s\n' "$line" > "${TEMP_DIRECTORY}/runtime.log"
    if bash "$CHECKER" "${TEMP_DIRECTORY}/runtime.log" >/dev/null 2>&1; then
        printf 'Expected device output rejected: %s\n' "$label" >&2
        exit 1
    fi
}

expect_accept "Direct3D12/Dozen" '[0 Direct3D12 (Dozen)]'
expect_accept "native hardware GPU" '[0 NVIDIA GeForce RTX 4090]'
expect_accept "selected hardware despite software at index 1" $'[0 Direct3D12 (Dozen)]\n[1 llvmpipe (LLVM 19)]'
expect_reject "selected software despite hardware at index 1" $'[0 llvmpipe (LLVM 19)]\n[1 NVIDIA GeForce RTX 4090]'
for software in llvmpipe lavapipe softpipe SwiftShader; do
    expect_reject "$software-only" "[0 $software (LLVM)]"
done
printf 'Vulkan device: initialization complete\n' > "${TEMP_DIRECTORY}/runtime.log"
if bash "$CHECKER" "${TEMP_DIRECTORY}/runtime.log" >/dev/null 2>&1; then
    printf 'Expected missing device output rejected\n' >&2
    exit 1
fi
expect_reject "unparseable device output" 'selected Vulkan device: NVIDIA GeForce RTX 4090'
printf 'Vulkan device acceptance tests passed\n'
