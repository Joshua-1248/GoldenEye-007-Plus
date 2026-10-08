#!/bin/bash
set -euo pipefail

usage()
{
    echo "Rare 1172 compression script" >&2
    echo "Usage: $0 input output [--cdata-zopfli]" >&2
    echo "  default: historical bundled gzip/raw-DEFLATE (required for ordinary resources)" >&2
    echo "  --cdata-zopfli: reserved for final cdata/data_seg only" >&2
    exit 1
}

if [ -z "${1:-}" ] || [ -z "${2:-}" ]; then
    usage
fi

INPUT_FILE="$1"
OUTPUT_FILE="$2"
MODE="${3:-ordinary}"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
BUNDLED_GZIP="${ROOT_DIR}/tools/gzipsrc/gzip"

case "${MODE}" in
    ordinary)
        ;;
    --cdata-zopfli)
        ;;
    *)
        echo "ERROR: unknown 1172 compression mode '${MODE}'" >&2
        usage
        ;;
esac

if [ ! -f "${INPUT_FILE}" ]; then
    echo "ERROR: can not read input file: ${INPUT_FILE}" >&2
    exit 2
fi

# Ordinary GoldenEye resources MUST use the exact historical bundled gzip path.
# This is a build invariant, not an optional tuning switch: using Zopfli here
# has previously produced a runtime-crashing Auto Shotgun resource stream.
# Intentionally ignore GE_ZOPFLI_1172 and GZ from the environment so a generic
# build command cannot silently alter the resource format again.
if [ "${MODE}" = "ordinary" ]; then
    if [ ! -x "${BUNDLED_GZIP}" ]; then
        echo "ERROR: required historical gzip is missing or not executable:" >&2
        echo "       ${BUNDLED_GZIP}" >&2
        echo "Refusing to fall back to system gzip because ordinary 0x1172 resources" >&2
        echo "must remain byte-stable (Auto Shotgun regression guard)." >&2
        exit 3
    fi
else
    if ! python3 -c 'import zopfli.gzip' >/dev/null 2>&1; then
        echo "ERROR: Python zopfli is required for final cdata compression." >&2
        echo "Install it with: python3 -m pip install zopfli" >&2
        exit 3
    fi
fi

# Build into a sibling temporary file, then atomically replace the target.
OUTDIR=$(dirname "${OUTPUT_FILE}")
OUTBASE=$(basename "${OUTPUT_FILE}")
mkdir -p "${OUTDIR}"
TMP_FILE=$(mktemp "${OUTDIR}/.${OUTBASE}.tmp.XXXXXX") || exit 2
cleanup() { rm -f "${TMP_FILE}"; }
trap cleanup EXIT HUP INT TERM

if [ "${MODE}" = "--cdata-zopfli" ]; then
    python3 - "${INPUT_FILE}" "${TMP_FILE}" <<'PYZ'
import pathlib
import sys
import zopfli.gzip
src = pathlib.Path(sys.argv[1]).read_bytes()
gz = zopfli.gzip.compress(src, numiterations=80, blocksplitting=1, blocksplittingmax=64, blocksplittinglast=0)
pathlib.Path(sys.argv[2]).write_bytes(b"\x11\x72" + gz[10:-8])
PYZ
else
    printf '\x11\x72' > "${TMP_FILE}"
    "${BUNDLED_GZIP}" --no-name --best < "${INPUT_FILE}" | tail --bytes=+11 | head --bytes=-8 >> "${TMP_FILE}"
fi

if [ ! -s "${TMP_FILE}" ] || [ "$(wc -c < "${TMP_FILE}")" -le 2 ]; then
    echo "ERROR: 1172 compression produced a truncated output for ${INPUT_FILE}" >&2
    exit 4
fi

mv -f "${TMP_FILE}" "${OUTPUT_FILE}"
trap - EXIT HUP INT TERM
exit 0
