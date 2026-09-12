#!/usr/bin/env bash
set -euo pipefail

EXPECTED_SHA1="abe01e4aeb033b6c0836819f549c791b26cfde83"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

if [[ ! -f Makefile ]]; then
  echo "ERROR: Run this script from the GoldenEye 007 Plus source root." >&2
  exit 2
fi

if [[ ! -f baserom.u.z64 ]]; then
  echo "ERROR: baserom.u.z64 is missing." >&2
  echo "Place your own unmodified NTSC-U GoldenEye 007 ROM here as baserom.u.z64." >&2
  exit 2
fi

actual_sha1="$(sha1sum baserom.u.z64 | awk '{print $1}')"
if [[ "$actual_sha1" != "$EXPECTED_SHA1" ]]; then
  echo "ERROR: Unsupported baserom.u.z64 SHA-1:" >&2
  echo "  got:      $actual_sha1" >&2
  echo "  expected: $EXPECTED_SHA1" >&2
  exit 2
fi

echo "Base ROM hash OK."

# These model directories contain GoldenEye 007 Plus customizations. Preserve
# them across forced retail-asset extraction so extraction cannot silently
# replace their generated companion files with retail versions.
assettmp="$(mktemp -d)"
custom_asset_dirs=(
  assets/obseg/prop/chrsilverwppk
  assets/obseg/prop/chrgoldwppk
  assets/obseg/prop/sev_door3
  assets/obseg/prop/sev_door3_wind
  assets/obseg/prop/sev_door4_wind
  assets/obseg/prop/sev_door_v1
)
restore_custom_assets() {
  for d in "${custom_asset_dirs[@]}"; do
    if [[ -d "$assettmp/$d" ]]; then
      rm -rf "$d"
      mkdir -p "$(dirname "$d")"
      cp -a "$assettmp/$d" "$d"
    fi
  done
}
cleanup_assets() {
  restore_custom_assets
  rm -rf "$assettmp"
}
trap cleanup_assets EXIT

for d in "${custom_asset_dirs[@]}"; do
  if [[ -d "$d" ]]; then
    mkdir -p "$assettmp/$(dirname "$d")"
    cp -a "$d" "$assettmp/$d"
  fi
done

echo "Extracting required US assets from local baserom.u.z64..."
make force_extract_u
restore_custom_assets

echo "Building GoldenEye 007 Plus..."
make \
  VERSION=US \
  PHYSICAL_CODE=YES \
  PHYSICAL_FASTPATHS=YES \
  MODDED_CHEATS=YES \
  OPT_USE_LLVM=YES \
  COMPARE=0 \
  -j"$JOBS" all

ROM="build/u-phys-opt-mod/ge007.u-phys-opt-mod.z64"
if [[ ! -f "$ROM" ]]; then
  echo "ERROR: Build completed without expected ROM output: $ROM" >&2
  exit 1
fi

cp -f "$ROM" GoldenEye_007_Plus.z64

echo
echo "Build complete: GoldenEye_007_Plus.z64"
sha1sum GoldenEye_007_Plus.z64
sha256sum GoldenEye_007_Plus.z64
