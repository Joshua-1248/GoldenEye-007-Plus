#!/usr/bin/env python3
"""Static source preflight for the GoldenEye GE_PHYSICAL_CODE branch.

This does not replace scripts/audit_physical_code.py's linked-ELF audit.  It is a
fast guard for hand-written literals that should be compiled out in physical
mode, and it works before extracted assets/toolchain files are present.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Iterable, List, Tuple

ROOT = Path(__file__).resolve().parent.parent

LEGACY_EXACT = {
    0x70000000,
    0x70100000,
    0x70200000,
    0x7F000000,
    0x7F100000,
}

# Narrow runtime executable ranges.  Keeping these narrow avoids unrelated
# 0x7... colors, floats, masks, and table data.
LEGACY_RANGES = (
    (0x70000450, 0x70020C00, "retail resident code"),
    (0x70200000, 0x70210000, "retail inflate code"),
    (0x7F000000, 0x7F100000, "retail game code"),
)

HEX_RE = re.compile(r"0x[0-9A-Fa-f]{8}\b")
PP_RE = re.compile(r"^\s*#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)$")
ASM_PP_RE = re.compile(r"^\s*\.(ifdef|ifndef|else|endif)\b(.*)$")
LUI_RE = re.compile(r"\blui\s+\$[A-Za-z0-9_]+\s*,\s*(0x[0-9A-Fa-f]{1,4})\b")


def strip_block_comments(text: str) -> str:
    def repl(match: re.Match[str]) -> str:
        s = match.group(0)
        return "".join("\n" if c == "\n" else " " for c in s)
    return re.sub(r"/\*.*?\*/", repl, text, flags=re.S)


def physical_condition(kind: str, expr: str):
    expr = expr.strip()
    if kind == "ifdef" and expr == "GE_PHYSICAL_CODE":
        return True
    if kind == "ifndef" and expr == "GE_PHYSICAL_CODE":
        return False
    if kind in ("if", "elif"):
        compact = re.sub(r"\s+", "", expr)
        if compact in ("defined(GE_PHYSICAL_CODE)", "definedGE_PHYSICAL_CODE"):
            return True
        if compact in ("!defined(GE_PHYSICAL_CODE)", "!definedGE_PHYSICAL_CODE"):
            return False
    return None


def active_lines(path: Path) -> Iterable[Tuple[int, str]]:
    text = strip_block_comments(path.read_text(errors="ignore"))
    is_asm = path.suffix.lower() in (".s", ".inc")

    # Frames are [parent_active, mode, cond, branch_active].  Unknown conditions
    # deliberately keep both branches eligible; this is a conservative scan.
    frames: List[List[object]] = []
    active = True

    for lineno, raw in enumerate(text.splitlines(), 1):
        m = (ASM_PP_RE.match(raw) if is_asm else PP_RE.match(raw))
        if m:
            kind, expr = m.group(1), m.group(2)
            if kind in ("ifdef", "ifndef", "if"):
                cond = physical_condition(kind, expr)
                parent = active
                branch = True if cond is None else cond
                frames.append([parent, cond, branch])
                active = bool(parent and branch)
            elif kind == "elif" and frames:
                parent, old_cond, _old_branch = frames[-1]
                cond = physical_condition(kind, expr)
                if old_cond is None and cond is None:
                    branch = True
                elif old_cond is None:
                    # Unknown earlier branch may or may not have run; include this
                    # branch as potentially active for conservative scanning.
                    branch = True
                else:
                    branch = False if bool(old_cond) else (True if cond is None else bool(cond))
                frames[-1] = [parent, cond if old_cond is not None else None, branch]
                active = bool(parent and branch)
            elif kind == "else" and frames:
                parent, cond, branch = frames[-1]
                if cond is None:
                    new_branch = True
                else:
                    new_branch = not bool(branch)
                frames[-1][2] = new_branch
                active = bool(parent and new_branch)
            elif kind == "endif" and frames:
                parent, _cond, _branch = frames.pop()
                active = bool(parent)
            continue

        if not active:
            continue

        line = raw
        if is_asm:
            line = line.split("#", 1)[0]
        else:
            line = line.split("//", 1)[0]
        yield lineno, line


def classify(value: int):
    if value in LEGACY_EXACT:
        return "legacy virtual base/range constant"
    for lo, hi, label in LEGACY_RANGES:
        if lo <= value < hi:
            return label
    return None


def main() -> int:
    candidates: List[Path] = [ROOT / "ge007.ld"]
    for base in (ROOT / "src", ROOT / "ld"):
        if not base.exists():
            continue
        for p in base.rglob("*"):
            if not p.is_file() or p.suffix.lower() not in {".c", ".h", ".s", ".inc", ".ld"}:
                continue
            if ".pre_" in p.name or p.name.endswith(".fullpageropt") or p.name.endswith(".pre_tlbopt"):
                continue
            candidates.append(p)

    failures = []
    checked_literals = 0
    for path in candidates:
        for lineno, line in active_lines(path):
            for m in HEX_RE.finditer(line):
                checked_literals += 1
                value = int(m.group(0), 16)
                why = classify(value)
                if why:
                    failures.append((path, lineno, value, why, line.strip()))

            # Hand-written MIPS often materializes a segment base as a 16-bit
            # LUI immediate (for example `lui $a1, 0x7000`) rather than spelling
            # the full 32-bit address.  Check active assembly branches for that
            # form as well.
            if path.suffix.lower() in (".s", ".inc"):
                for m in LUI_RE.finditer(line):
                    checked_literals += 1
                    value = (int(m.group(1), 16) << 16) & 0xFFFFFFFF
                    why = classify(value)
                    if why:
                        failures.append((path, lineno, value, why + " via LUI", line.strip()))

    print("GoldenEye GE_PHYSICAL_CODE source preflight")
    print("=" * 41)
    print(f"scanned {len(candidates)} runtime/linker source files; examined {checked_literals} 32-bit hex literals")
    if failures:
        for path, lineno, value, why, line in failures:
            rel = path.relative_to(ROOT)
            print(f"[FAIL] {rel}:{lineno}: 0x{value:08X} ({why}) :: {line}")
        print(f"RESULT: FAIL ({len(failures)} suspicious physical-mode literal(s))")
        return 1

    print("[ OK ] no live physical-mode source literals point into the retired 0x700/0x702/0x7F execution ranges")
    print("RESULT: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
