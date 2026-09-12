#!/bin/bash
# This file will delete all extracted *.bin files from a previous ./extract_baserom.sh (provided the csv has not changed!!!)
if [ -z "$1" ]; then
    DOALL="1"
    echo "Processing Everything"
fi

true="1"
if [ "$DOALL" == "1" ] || [ $1 == 'files' ]; then
    while IFS=, read -r offset size name compressed extract
    do
        echo "removing $name"
        rm -f $name
    done < filelist.u.csv

    while IFS=, read -r offset size name compressed extract
    do
        echo "removing $name"
        rm -f $name
    done < filediff.j.csv

    while IFS=, read -r offset size name compressed extract
    do
        echo "removing $name"
        rm -f $name
    done < filediff.e.csv
fi

if [ "$DOALL" == "1" ] || [ $1 == 'images' ]; then
    # Generate imagelist if it doesn't exist
    if [ ! -f build/u/imagelist.csv ]; then
        mkdir -p build/u
        python3 scripts/make/generate_imagelist.py build/u/imagelist.csv
    fi
    while IFS=, read -r offset size name compressed extract
    do
        echo "removing $name"
        rm -f $name
    done < build/u/imagelist.csv
fi
