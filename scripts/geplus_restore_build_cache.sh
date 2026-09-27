#!/usr/bin/env bash
set -euo pipefail
CACHE="${1:?usage: $0 <geplus-u-phys-opt-mod-build-cache.tar.zst>}"
tar --zstd -xf "$CACHE"
printf 'Restored build/u-phys-opt-mod. Run scripts/geplus_quickbuild.sh; changed/new files will rebuild normally.\n'
