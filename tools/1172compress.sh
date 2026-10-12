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

# Four independent Turtledeflate processes compress separate CDATA ranges.
# This branch is cdata-only; ordinary 1172 asset compression is never changed.
# There is no sequential whole-file Zopfli step on this fast path.
if [ "${MODE}" = "--cdata-zopfli" ] && [ "${GE_CDATA_TURTLE_WORKERS:-1}" != "1" ]; then
    if [ "${GE_CDATA_TURTLE:-}" != "1" ]; then
        echo "ERROR: parallel CDATA mode requires GE_CDATA_TURTLE=1" >&2
        exit 3
    fi
    case "${GE_CDATA_TURTLE_WORKERS}" in
        2|3|4|5|6|7|8) ;;
        *) echo "ERROR: GE_CDATA_TURTLE_WORKERS must be 1 through 8" >&2; exit 3 ;;
    esac
    # Preserve the eight-way Turtledeflate fast path. If its independent
    # chunks exceed the fixed cdata slot, search whole-stream DEFLATE layouts
    # with the same number of parallel workers instead of disabling checks.
    if ! python3 "${ROOT_DIR}/scripts/parallel_turtle_cdata.py" \
        "${INPUT_FILE}" "${TMP_FILE}" \
        --bin "${GE_CDATA_TURTLE_BIN:-${HOME}/Downloads/turtledeflate/build/turtledeflate_app}" \
        --workers "${GE_CDATA_TURTLE_WORKERS}" \
        --level 9; then
        echo "CDATA chunked Turtledeflate did not fit; trying parallel whole-stream candidates"
        python3 "${ROOT_DIR}/scripts/parallel_zopfli_cdata.py" \
            "${INPUT_FILE}" "${TMP_FILE}" \
            --workers "${GE_CDATA_TURTLE_WORKERS}"
    fi
elif [ "${MODE}" = "--cdata-zopfli" ]; then
    python3 - "${INPUT_FILE}" "${TMP_FILE}" <<'PYZ'
import pathlib
import sys
import zopfli.gzip
import os
import subprocess
import tempfile
import zlib
src = pathlib.Path(sys.argv[1]).read_bytes()
best = zopfli.gzip.compress(src, numiterations=80, blocksplitting=1,
                            blocksplittingmax=64, blocksplittinglast=0)

# The retail cdata slot has a fixed 72704-byte limit. Preserve the existing
# compressor output when it fits. Only retry when the final cdata is too large;
# ordinary 1172 resources NEVER enter this branch (Auto Shotgun safety).
limit = 72704
if len(best) - 16 > limit:
    for iterations, maxblocks, splitlast in ((80, 0, 0),
                                             (80, 64, 1),
                                             (120, 0, 0)):
        trial = zopfli.gzip.compress(src, numiterations=iterations,
                                    blocksplitting=1,
                                    blocksplittingmax=maxblocks,
                                    blocksplittinglast=splitlast)
        if len(trial) < len(best):
            best = trial
        if len(best) - 16 <= limit:
            break
    print("cdata compression: best %d / %d bytes" %
          (len(best) - 16, limit))

# The advanced N64 CDATA encoder is entirely opt-in: never touch ordinary
# compressed assets. This must recompress the *current* data_seg; a candidate
# from an older ROM build is invalid after source code changes.
output = b"\x11\x72" + best[10:-8]
if os.getenv("GE_CDATA_TURTLE") == "1":
    executable = pathlib.Path(os.getenv(
        "GE_CDATA_TURTLE_BIN",
        str(pathlib.Path.home() / "Downloads/turtledeflate/build/turtledeflate_app")))
    if not executable.is_file():
        sys.exit("ERROR: GE_CDATA_TURTLE=1 but turtledeflate_app is missing: " + str(executable))
    with tempfile.TemporaryDirectory(prefix="goldeneye_cdata_") as tmp:
        inputfile = pathlib.Path(tmp) / "cdata_input.bin"
        inputfile.write_bytes(src)
        try:
            trial = subprocess.run([str(executable), "--raw", "--9", str(inputfile)],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                   timeout=1200, check=True)
        except subprocess.TimeoutExpired:
            sys.exit("ERROR: Turtledeflate CDATA exceeded 20-minute timeout")
        except subprocess.CalledProcessError as exc:
            sys.exit("ERROR: Turtledeflate CDATA failed: " +
                     exc.stderr.decode(errors="replace")[-1000:])
        rawfile = pathlib.Path(str(inputfile) + ".deflate")
        if not rawfile.exists():
            sys.exit("ERROR: Turtledeflate produced no raw DEFLATE stream")
        candidate = b"\x11\x72" + rawfile.read_bytes()
        # Verify full DEFLATE consumption and exact byte-for-byte input.
        decoder = zlib.decompressobj(-15)
        unpacked = decoder.decompress(candidate[2:]) + decoder.flush()
        if not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or unpacked != src:
            sys.exit("ERROR: Turtledeflate output failed strict CDATA verification")
        if len(candidate) < len(output):
            output = candidate
            print("cdata: Turtledeflate won (%d bytes, saved %d vs Zopfli)" %
                  (len(output), len(best) - 16 - len(output)), flush=True)
        else:
            print("cdata: keeping Zopfli (%d vs Turtledeflate %d bytes)" %
                  (len(output), len(candidate)), flush=True)
pathlib.Path(sys.argv[2]).write_bytes(output)
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
