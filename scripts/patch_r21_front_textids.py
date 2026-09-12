#!/usr/bin/env python3
"""R21 fixed-slot frontend text-ID hotfix.

The legacy getStringID(TEXTBANK, TEXTSLOT) macro does not parenthesize
TEXTSLOT.  R21 originally passed a ternary expression as TEXTSLOT in two new
frontend rows (No Radar On/Off and Head Roll On/Off).  C operator precedence
therefore made the compiler emit a low slot ID (0x73/0x74) without LTITLE's
0x9c00 bank bits, and langGet dereferenced language bank 0.

A normal C-source rewrite perturbs the tightly packed/compressed C segment by
more than the retail 72,704-byte slot allows.  Until code space is reclaimed,
patch the exact compiler sequences after objcopy.  The replacements keep all
code addresses and instruction counts unchanged.  They:
  * test the real flag instead of the always-nonzero (0x9c00 + flag),
  * load full LTITLE IDs 0x9c73 / 0x9c74, and
  * restore the Player Options row Y anchor before drawing the value column,
  * stop weapon cycling after a second pass through a prop-only inventory ring, and
  * make unexpected/Co-Op score-page scenarios return no score instead of spinning forever, and
  * initialize 3/4P NTSC-U weapon-pickup text before appending the short item name.

The last fix is needed because GoldenEye's localized option labels include a
trailing newline.  textRender advances the caller-owned Y variable on that
newline, so reusing the same Y pointer for the value drew it one line lower.
The size-neutral instruction rewrite keeps the sign-extended viGetX result in
$s1 directly and uses the now-free delay slot to restore the row Y from $s4.

Fail loudly if any frozen R21 signature is not found exactly once; this
prevents silently patching a later, incompatible frontend layout.
"""

from pathlib import Path
import sys

PATCHES = (
    (
        "Multiplayer Settings No Radar On/Off",
        bytes.fromhex("01014821 11200004 240b0074 240a0073"),
        bytes.fromhex("01004825 11200004 340b9c74 340a9c73"),
    ),
    (
        "Player Options Head Roll On/Off",
        bytes.fromhex("01816821 11a00003 24050074 10000001 24050073"),
        bytes.fromhex("01806825 11a00003 34059c74 10000001 34059c73"),
    ),
    (
        "Player Options label/value row alignment",
        bytes.fromhex("00028c00 00116403 0c00108f 01808825"),
        bytes.fromhex("00028c00 00118c03 0c00108f afb40088"),
    ),
    (
        "Inventory forward prop-only ring wrap guard",
        bytes.fromhex("16190005 00000000 56400006 8c6811ec 2411ffff 2413ffff"),
        bytes.fromhex("16190005 2a210000 54200006 8c6811ec 2411ffff 2413ffff"),
    ),
    (
        "Inventory backward prop-only ring wrap guard",
        bytes.fromhex("16190005 00000000 56400006 8c6811ec 241103e8 241303e8"),
        bytes.fromhex("16190005 2a210021 50200006 8c6811ec 241103e8 241303e8"),
    ),
    (
        "Co-Op pause invalid-score-scenario guard",
        bytes.fromhex("10000003 00001025 1000ffff 00000000 8fbf0014"),
        bytes.fromhex("10000003 00001025 10000001 00001025 8fbf0014"),
    ),
    (
        "3/4P weapon pickup string initialization",
        bytes.fromhex("11c0000b afa00024 3c058005"),
        bytes.fromhex("11c0000b a2000000 3c058005"),
    ),
)


def find_all(buf: bytes, needle: bytes):
    out = []
    start = 0
    while True:
        pos = buf.find(needle, start)
        if pos < 0:
            return out
        out.append(pos)
        start = pos + 1


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} <uncompressed-rom-bin>", file=sys.stderr)
        return 2

    path = Path(sys.argv[1])
    data = bytearray(path.read_bytes())

    for label, old, new in PATCHES:
        hits = find_all(data, old)
        if label == "Player Options label/value row alignment" and len(hits) > 1:
            # Later frontend additions can compile an identical instruction
            # sequence elsewhere.  The frozen Player Options site is the
            # first occurrence in the original R21 frontend region.
            hits = [min(hits)]
        if len(hits) == 0:
            newhits = find_all(data, new)
            if len(newhits) == 1:
                print(f"R21 frontend fixed-slot patch: {label} already present @ ROM 0x{newhits[0]:06X}")
                continue
            if label in ("Multiplayer Settings No Radar On/Off", "Player Options Head Roll On/Off", "Player Options label/value row alignment"):
                print(f"R21 frontend fixed-slot patch: {label} not needed in this source layout")
                continue
        if len(hits) != 1:
            print(f"ERROR: {label}: expected exactly one R21 signature, found {len(hits)}", file=sys.stderr)
            return 1
        pos = hits[0]
        data[pos:pos + len(old)] = new
        print(f"R21 frontend fixed-slot patch: {label} @ ROM 0x{pos:06X}")

    path.write_bytes(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
