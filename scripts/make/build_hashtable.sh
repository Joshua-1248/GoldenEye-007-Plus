#!/bin/bash

COUNTRY_CODE=""
OUTFILE=""
ARG_VERSION=""

usage() {
    echo "$0 usage:"
    echo ""
    echo "$0 -v u [-o results_file]"
    echo ""
    echo "    -o        output filename. Optional. Defaults to full_hashtable_{version}.csv"
    echo "    -v        version. Supported options are: US,u, JP,j, EU,e"
    echo ""
    exit 0
}

[ $# -eq 0 ] && usage

while getopts "o:hv:" arg; do
    case $arg in
        v)
            case "${OPTARG,,}" in
                us | u)
                    COUNTRY_CODE="u"
                    ;;
                jp | j)
                    COUNTRY_CODE="j"
                    ;;
                eu | e)
                    COUNTRY_CODE="e"
                    ;;
                *)
                    echo "Unsupported version: ${OPTARG}"
                    usage
                    ;;
            esac

            ARG_VERSION="${OPTARG}"
            ;;
        o)
            OUTFILE="${OPTARG}"
            ;;
        h | *)
            usage
            ;;
    esac
done

if [ -z "${COUNTRY_CODE}" ]; then
    echo "No valid version provided."
    usage
fi

if ! command -v "mips-linux-gnu-objcopy" &> /dev/null
then
    echo "command mips-linux-gnu-objcopy not found"
    exit 1
fi

    if ! command -v "md5sum" &> /dev/null
then
    echo "command md5sum not found"
    exit 1
fi

if [ -z "${OUTFILE}" ] ; then
    OUTFILE="full_hashtable_${ARG_VERSION}.csv"
fi

TMP=$(mktemp /tmp/ge_test_files.XXXXXX) || { echo "Failed to create temp file"; exit 1; }
trap 'rm -f "${TMP}"' EXIT

rm -f "${OUTFILE}"
touch "${OUTFILE}"

SECTIONS=( ".text" ".code" ".bss" ".data" ".rodata" )

# Search roots: build country tree and repository root assets folder
SEARCH_ROOTS=( "build/${COUNTRY_CODE}" )

for ROOT in "${SEARCH_ROOTS[@]}"
do
    if [ -d "${ROOT}" ]; then
        # find all object files under this root, but skip build/${COUNTRY_CODE}/assets/images/*
        find "${ROOT}" -type f -name '*.o' ! -path "*/assets/images/*" -print0 | while IFS= read -r -d '' FILE
        do
            echo "adding ${FILE}"
            for SEC in "${SECTIONS[@]}"
            do
                # Extract section to TMP; suppress objcopy stderr (section may be missing)
                mips-linux-gnu-objcopy -j "${SEC}" -O binary "${FILE}" "${TMP}" 2>/dev/null || true

                # If TMP has content, compute md5 and append to OUTFILE
                if [ -s "${TMP}" ]; then
                    MD5=$(md5sum -b "${TMP}" | cut -c -32)
                    printf "%s,%s,%s\n" "${MD5}" "${SEC}" "${FILE}" >> "${OUTFILE}"
                fi

                # Clear TMP for next section
                : > "${TMP}"
            done
        done
    fi
done

# TMP is removed by trap on exit
exit 0
