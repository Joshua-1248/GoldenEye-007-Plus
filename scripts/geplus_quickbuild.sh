#!/usr/bin/env bash
set -euo pipefail
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
exec make -j"$JOBS" VERSION=US PHYSICAL_CODE=YES PHYSICAL_FASTPATHS=YES MODDED_CHEATS=YES MAP_MAKER=YES OPT_USE_LLVM=YES COMPARE=0 all
