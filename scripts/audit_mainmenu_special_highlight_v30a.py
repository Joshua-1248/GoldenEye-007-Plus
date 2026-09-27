#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
FRONT = ROOT / 'src/game/front.c'
MAKE = ROOT / 'Makefile'

def fail(msg):
    print('MAIN MENU SPECIAL HIGHLIGHT V30A AUDIT: FAIL:', msg, file=sys.stderr)
    raise SystemExit(1)

def require(cond, msg):
    if not cond:
        fail(msg)

s = FRONT.read_text(errors='replace')
m = MAKE.read_text(errors='replace')
checks = [
    ('special row pitch is explicitly 16', '#define MOD_SPECIAL_OPTIONS_ROW_HEIGHT 16' in s),
    ('special render uses shared row pitch', 'g_ModOptionsPage ? MOD_SPECIAL_OPTIONS_ROW_HEIGHT : MOD_OPTIONS_ROW_HEIGHT' in s),
    ('cursor hit-test uses same special row pitch', '(g_ModOptionsPage ? MOD_SPECIAL_OPTIONS_ROW_HEIGHT : MOD_OPTIONS_ROW_HEIGHT)' in s),
    ('old unconditional 18px cursor division is gone', 'cursor_v_pos - MOD_OPTIONS_FIRST_ROW_Y) / MOD_OPTIONS_ROW_HEIGHT' not in s),
    ('audit is mandatory build prerequisite', 'mainmenu-special-highlight-v30a-audit' in m and 'audit_mainmenu_special_highlight_v30a.py' in m),
]
for name, ok in checks:
    print(('[PASS] ' if ok else '[FAIL] ') + name)
    if not ok:
        fail(name)
print(f'MAIN MENU SPECIAL HIGHLIGHT V30A AUDIT: PASS ({len(checks)}/{len(checks)})')
