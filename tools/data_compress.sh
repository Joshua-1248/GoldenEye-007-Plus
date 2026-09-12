#!/bin/bash
MAPFILE="./build/"$2"/ge007."$2".map"
#this script is a hacky mess that can most definately be improved
#fixme as I will fail if vaddr of data gets moved!!!
# Support both GNU ld's traditional map format and the LLD fallback used by
# the physical/optimized build modes.  The old grep|cut code silently parsed
# LLD maps as zero, causing the entire ROM to be treated as csegment data.
DATASEG_START_HEX=$(sed -nE 's/.*__csegtempPos[[:space:]]*=[[:space:]]*0x([0-9A-Fa-f]+).*/\1/p' "${MAPFILE}" | head -n1)
if [ -z "${DATASEG_START_HEX}" ]; then
    DATASEG_START_HEX=$(grep ${MAPFILE} -e '__csegtempPos =' | cut -d "x" -f3)
fi
DATASEG_START=$((16#${DATASEG_START_HEX}))

# LLD map rows are: VMA LMA SIZE ALIGN NAME.  Prefer that explicit .csegment
# row, then fall back to the legacy GNU-map parser.
DATASEG_LEN_HEX=$(awk '$5 == ".csegment" { print $3; exit }' "${MAPFILE}")
if [ -z "${DATASEG_LEN_HEX}" ]; then
    DATASEG_LEN_HEX=$(grep ${MAPFILE} -e 'load address 0x0000000000c00000' | sed -nE 's/.*load address 0x0000000000c00000[^0-9A-Fa-f]+0x([0-9A-Fa-f]+).*/\1/p' | head -n1)
fi
DATASEG_LEN=$((16#${DATASEG_LEN_HEX}))

#build/rebuild aaa_rip
[ ! -x tools/aaa_rip/aaa_rip ] && make -C tools/aaa_rip

echo "patching $1"
echo "extract data segment"
[ -x tools/aaa_rip/aaa_rip ] && tools/aaa_rip/aaa_rip $1 build/$2/data_seg ${DATASEG_START} ${DATASEG_LEN} || dd skip=${DATASEG_START} count=${DATASEG_LEN} if=$1 of=build/$2/data_seg bs=1

echo "truncate $1 to 0x$(printf "%x\n" ${DATASEG_START})"
cat $1 | head --bytes=${DATASEG_START} > $1.tmp

echo "compress data segment"
tools/1172compress.sh build/$2/data_seg build/$2/data_seg.rz


echo "inject data segment"
RZSIZE=$(stat -c%s "build/$2/data_seg.rz")
echo "size=${RZSIZE}"

#fixme as I will fail if position of c_data gets moved!!!
CDATA_POS_HEX=$(awk '$NF == "c_data_array" { print $1; exit }' "${MAPFILE}")
if [ -z "${CDATA_POS_HEX}" ]; then
    CDATA_POS_HEX=$(grep ${MAPFILE} -e 'c_data_array' | cut -d "x" -f 2 | cut -d " " -f 1)
fi
CDATA_POS=$((16#${CDATA_POS_HEX}))
#CDATA_MAX_SIZE=$(printf "%d\n" 0x$(grep  '${MAPFILE}' -e '.c_data         0x0000000000021990' | cut -d "x" -f 2 ))
#CDATA_POS=137616
CDATA_MAX_SIZE=72704

echo "maxsize=${CDATA_MAX_SIZE}"

# The compressed csegment lives in a fixed retail ROM slot immediately before
# the inflate payload.  Do not silently overwrite the following region if a
# mod grows past that slot; fail loudly so the source can be trimmed safely.
if [ "${RZSIZE}" -gt "${CDATA_MAX_SIZE}" ]; then
    echo "ERROR: compressed data segment is ${RZSIZE} bytes; fixed slot is only ${CDATA_MAX_SIZE} bytes" >&2
    rm -f "$1.tmp"
    exit 1
fi

[ -x tools/aaa_rip/aaa_rip ] && tools/aaa_rip/aaa_rip build/$2/data_seg.rz $1.tmp 0 0 ${CDATA_POS} || dd if=build/$2/data_seg.rz of=$1.tmp obs=1 seek=${CDATA_POS} conv=notrunc

mv $1.tmp $1

