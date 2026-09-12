#!/bin/bash

# Combine image .bin files in the correct order using imagelist
# $1: imagelist CSV file (with paths in 3rd column)
# $2: output directory for combined .bin

IMAGELIST=$1
COMBINE_DIR=$2

if [ -z "${IMAGELIST}" ]; then echo "$0: missing argument: IMAGELIST"; exit 1; fi
if [ -z "${COMBINE_DIR}" ]; then echo "$0: missing argument: COMBINE_DIR"; exit 1; fi

if [ ! -f "${IMAGELIST}" ]; then
    echo "$0: imagelist file not found: ${IMAGELIST}"
    exit 1
fi

echo "Combining image .bin files into single .bin using ${IMAGELIST}"

mkdir -p "${COMBINE_DIR}"
COMBINED_BIN="${COMBINE_DIR}/combined.bin"

echo "rm -f ${COMBINED_BIN}"
rm -f "${COMBINED_BIN}"

echo "Combining images in order..."
# Read imagelist and concatenate files in order
while IFS=, read -r offset size filepath remainder; do
    if [ -f "${filepath}" ]; then
        cat "${filepath}" >> "${COMBINED_BIN}"
    else
        echo "Warning: ${filepath} not found, skipping"
    fi
done < "${IMAGELIST}"

COMBINED_SIZE=$(stat -c%s "${COMBINED_BIN}" 2>/dev/null || stat -f%z "${COMBINED_BIN}" 2>/dev/null)
echo "Combined ${COMBINED_SIZE} bytes into ${COMBINED_BIN}"

# Pad to next 0x10 (16-byte) multiple
PADDING=$((16 - (COMBINED_SIZE % 16)))
if [ ${PADDING} -ne 16 ]; then
    echo "Padding ${PADDING} bytes to align to 0x10 boundary"
    dd if=/dev/zero bs=1 count=${PADDING} >> "${COMBINED_BIN}" 2>/dev/null
    PADDED_SIZE=$(stat -c%s "${COMBINED_BIN}" 2>/dev/null || stat -f%z "${COMBINED_BIN}" 2>/dev/null)
    echo "Final size: ${PADDED_SIZE} bytes (0x$(printf '%X' ${PADDED_SIZE}))"
fi
