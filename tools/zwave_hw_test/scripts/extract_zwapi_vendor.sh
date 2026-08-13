#!/usr/bin/env bash
# Extract zwapi + zwave_definitions from Unify SDK history (ver_1.6.0) for the
# hardware test harness. Run from repository root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
VENDOR="${ROOT}/tools/zwave_hw_test/vendor"
REF="${ZWAPI_VENDOR_GIT_REF:-fb360ad38^}"

cd "$ROOT"

if ! git rev-parse --verify "$REF" >/dev/null 2>&1; then
  echo "Git ref not found: $REF" >&2
  exit 1
fi

echo "Extracting zwapi vendor from $REF ..."

rm -rf "$VENDOR"
mkdir -p "$VENDOR"

git archive "$REF" \
  applications/zpc/components/zwave_api \
  applications/zpc/components/zwave/zwave_definitions \
  applications/zpc/components/zwave/zwave_controller/include/zwave_controller_endian.h \
  | tar -x -C "$VENDOR"

# Normalize layout under vendor/
if [[ -d "$VENDOR/applications/zpc/components/zwave_api" ]]; then
  mv "$VENDOR/applications/zpc/components/zwave_api" "$VENDOR/zwave_api"
  mv "$VENDOR/applications/zpc/components/zwave/zwave_definitions" "$VENDOR/zwave_definitions"
  mv "$VENDOR/applications/zpc/components/zwave/zwave_controller/include/zwave_controller_endian.h" \
    "$VENDOR/zwave_controller_endian.h"
  rm -rf "$VENDOR/applications"
fi

# Optional: apply Serial API performance patch (zwapi portions only).
PATCH="${ROOT}/tools/zpc_serial_api_perf/0001-zwave-zwapi-serial-api-performance.patch"
if [[ "${APPLY_ZWAPI_PERF_PATCH:-1}" == "1" ]] && [[ -f "$PATCH" ]]; then
  echo "Applying zwapi performance patch ..."
  (
    cd "$VENDOR"
    sed 's|applications/zpc/components/zwave_api/|zwave_api/|g' "$PATCH" \
      | patch -p1 --forward 2>/dev/null || true
  )
fi

echo "Vendor tree ready at $VENDOR"
