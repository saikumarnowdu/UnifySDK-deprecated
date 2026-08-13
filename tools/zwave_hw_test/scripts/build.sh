#!/usr/bin/env bash
# Build libzwave_python_harness.so for Python ctypes testing.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
VENDOR="${ROOT}/vendor"

if [[ ! -f "${VENDOR}/zwave_api/src/zwapi_init.c" ]]; then
  echo "Vendor missing — extracting zwapi from git history ..."
  "${ROOT}/scripts/extract_zwapi_vendor.sh"
fi

cmake -B "${ROOT}/build" -G Ninja "${ROOT}"
cmake --build "${ROOT}/build"
echo "Built: ${ROOT}/build/libzwave_python_harness.so"
