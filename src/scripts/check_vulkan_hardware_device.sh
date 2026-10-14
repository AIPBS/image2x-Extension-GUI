#!/usr/bin/env bash
# Accept only a reported Vulkan physical-device name that is not a software renderer.
set -euo pipefail

[[ "$#" -eq 1 ]] || { printf 'Usage: %s <runtime-log>\n' "$0" >&2; exit 2; }
log_file="$1"
[[ -f "$log_file" ]] || { printf 'Vulkan device log does not exist: %s\n' "$log_file" >&2; exit 1; }

python3 - "$log_file" <<'PY'
import re
import sys

path = sys.argv[1]
with open(path, encoding="utf-8", errors="replace") as stream:
    text = stream.read()

# ncnn Vulkan runtimes print physical devices as: [0 Device Name]. Every
# matrix invocation selects device index 0 with -g 0, so other indices do not
# affect this run's acceptance.
selected = re.search(r"\[\s*0\s+([^]\r\n]+?)\s*\]", text)
if not selected:
    print("GPU acceptance failed: no parseable Vulkan device name for selected index 0", file=sys.stderr)
    raise SystemExit(1)

name = selected.group(1)
software = re.compile(r"llvmpipe|lavapipe|softpipe|swiftshader", re.IGNORECASE)
if software.search(name):
    print(f"GPU acceptance failed: selected Vulkan device index 0 is software: {name}", file=sys.stderr)
    raise SystemExit(1)

print(f"GPU device index 0 accepted: {name}")
PY
