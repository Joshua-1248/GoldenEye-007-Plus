#!/bin/bash

# $1: directory containing .bin files, one per each image, in "natural" sort order (image8, image9, image10, ...)
# $2: output directory for combined .bin

SPLIT_DIR=$1
COMBINE_DIR=$2

if [ -z ${SPLIT_DIR} ]; then echo "$0: missing argument: SPLIT_DIR"; exit 1; fi
if [ -z ${COMBINE_DIR} ]; then echo "$0: missing argument: COMBINE_DIR"; exit 1; fi

echo "combining split image .bin files into single .bin"

pushd .

echo "cd ${SPLIT_DIR}"
cd "${SPLIT_DIR}"

echo "rm -f combined.bin"
rm -f combined.bin

echo "combining ..."
ls -1dv image* | xargs -I {} cat {} >> combined.bin

popd

mkdir -p "${COMBINE_DIR}"

echo "rm -f ${COMBINE_DIR}/combined.bin"
rm -f "${COMBINE_DIR}/combined.bin"

echo "mv ${SPLIT_DIR}/combined.bin ${COMBINE_DIR}/"
mv "${SPLIT_DIR}/combined.bin" "${COMBINE_DIR}/"
