#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
CC_HOST="${CC_HOST:-cc}"
TEST_SRC="$ROOT_DIR/scripts/tests/fastpath_equivalence.c"

# The supplied R20 community-source baseline contained this wrapper but did
# not contain the host-side test source it references.  Do not report a false
# PASS and do not fail with a cryptic compiler error.  Exit 77 is the
# conventional "skipped test" status.
if [[ ! -f "$TEST_SRC" ]]; then
    echo "SKIP: fastpath equivalence source is not present in the supplied R20 baseline:" >&2
    echo "  $TEST_SRC" >&2
    echo "The R21 ROM/source audits are independent of this unavailable host test." >&2
    exit 77
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
"$CC_HOST" -O2 -fno-fast-math -fno-strict-aliasing -fwrapv \
    "$TEST_SRC" -lm -o "$TMP/fastpath_equivalence"
"$TMP/fastpath_equivalence"
