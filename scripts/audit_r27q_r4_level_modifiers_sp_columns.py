#!/usr/bin/env python3
from pathlib import Path
import re
import sys

R = Path(__file__).resolve().parents[1]
F = (R/"src/game/front.c").read_text(errors="replace")
LM = (R/"src/game/levelmodifiers.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks = []
def ck(name, cond):
    cond = bool(cond)
    checks.append((name, cond))
    print(("[PASS] " if cond else "[FAIL] ") + name)

m = re.search(r'static const char \*g_LevelModifierSpNames\[\]\s*=\s*\{(.*?)\n\};', LM, re.S)
names = re.findall(r'"([^"]+)"', m.group(1)) if m else []

ck("campaign catalog still contains all 20 levels in order",
   names == ["Dam","Facility","Runway","Surface 1","Bunker 1","Silo",
             "Frigate","Surface 2","Bunker 2","Statue","Archives","Streets",
             "Depot","Train","Jungle","Control","Caverns","Cradle","Aztec","Egyptian"])
ck("two-column campaign layout marker is present",
   "R27Q_R4_LEVEL_MODIFIERS_SP_TWO_COLUMNS" in F)
ck("left and right campaign column centers are defined",
   "LEVELMOD_FRONT_LEFT_CENTER_X" in F and "LEVELMOD_FRONT_RIGHT_CENTER_X" in F)
ck("first ten and second ten entries map to separate columns",
   "index < LEVELMOD_FRONT_VISIBLE_ROWS" in F and
   ": LEVELMOD_FRONT_RIGHT_CENTER_X" in F)
ck("names are centered by measured text width",
   "x = center - (w >> 1);" in F and
   "x = LEVELMOD_FRONT_SINGLE_CENTER_X - (w >> 1);" in F)
ck("crosshair hit-testing distinguishes left and right columns",
   "cursor_h_pos < (f32)LEVELMOD_FRONT_MID_X" in F and
   "cursor_h_pos >= (f32)LEVELMOD_FRONT_MID_X" in F)
ck("Left/Right moves by ten entries",
   "g_LevelModifiersLevelChoice -= LEVELMOD_FRONT_VISIBLE_ROWS;" in F and
   "g_LevelModifiersLevelChoice + LEVELMOD_FRONT_VISIBLE_ROWS" in F)
ck("campaign page stays unscrolled so all 20 are visible",
   "g_LevelModifiersListTop = 0;" in
   F[F.find("void interface_menu_level_modifiers_levels"):
     F.find("Gfx *constructor_menu_level_modifiers_levels")])
ck("Previous tab and implemented-level gating remain",
   "frontAddPreviousTabText(DL)" in
   F[F.find("Gfx *constructor_menu_level_modifiers_levels"):
     F.find("void init_menu_level_modifiers_detail")] and
   "levelModifiersLevelImplemented" in
   F[F.find("void interface_menu_level_modifiers_levels"):
     F.find("Gfx *constructor_menu_level_modifiers_levels")])

prereq = next((x for x in MK.splitlines() if x.startswith("prerequisites:")), "")
ck("R27Q R4 audit is mandatory",
   "r27q-r4-level-modifier-columns-audit:" in MK and
   "r27q-r4-level-modifier-columns-audit" in prereq)

bad = [n for n, ok in checks if not ok]
print()
print(f"R27Q R4 LEVEL MODIFIER TWO-COLUMN AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad:
        print(" - " + n)
    sys.exit(1)
