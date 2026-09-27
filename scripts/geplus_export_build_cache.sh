#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-geplus-u-phys-opt-mod-build-cache.tar.zst}"
if [ ! -d build/u-phys-opt-mod ]; then
  echo "build/u-phys-opt-mod does not exist; run scripts/geplus_quickbuild.sh first" >&2
  exit 1
fi
# Preserve build timestamps: GNU make uses them to rebuild changed/newer source files only.
tar --zstd -cf "$OUT" build/u-phys-opt-mod
printf 'Wrote %s\n' "$OUT"
