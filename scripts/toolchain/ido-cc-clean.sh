#!/usr/bin/env bash
set -o pipefail

if [ "$#" -lt 1 ]; then
    echo "ido-cc-clean.sh: missing compiler command" >&2
    exit 2
fi

compiler="$1"
shift

tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

"$compiler" "$@" 2>"$tmp"
status=$?

# IDO 5.3's assembler warns when the compiler emits IEEE-754 FLT_MAX using
# its canonical decimal spelling. The value is valid single precision and
# assembles to 0x7f7fffff (or 0xff7fffff for the negated value). Suppress only
# the known retail-source occurrences; leave every other diagnostic visible.
source_arg=" $* "
while IFS= read -r line; do
    suppress=0
    case "$line" in
        "as1: Warning: src/game/stan.c, line 336: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/stanintersection.c, line 54: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/stanintersection.c, line 66: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/bg.c, line 5360: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/bg.c, line 5361: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/bg.c, line 5791: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: src/game/bgfog.c, line 443: number outside range for single precision floating point values") suppress=1 ;;
        "as1: Warning: , line 0: number outside range for single precision floating point values")
            case "$source_arg" in
                *" src/game/bgfog.c "*) suppress=1 ;;
            esac
            ;;
        "cc: Warning: -mips3 should not be used for ucode 32-bit compiles")
            case "$source_arg" in
                *" src/libultra/libc/ll.c "*|*" src/libultra/libc/llcvt.c "*) suppress=1 ;;
            esac
            ;;
    esac

    if [ "$suppress" -eq 0 ]; then
        printf '%s\n' "$line" >&2
    fi
done <"$tmp"

exit "$status"
