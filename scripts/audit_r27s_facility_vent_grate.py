#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
LM=(R/"src/game/levelmodifiers.c").read_text(errors="replace")
checks=[]
def ck(n,c):
    checks.append((n,bool(c))); print(("[PASS] " if c else "[FAIL] ")+n)
ck("R27S marker present","R27S_FACILITY_WALK_THROUGH_VENT_GRATE" in LM)
ck("Facility catalog is implemented","category == LEVELMOD_CATEGORY_SINGLE_PLAYER && index == 1" in LM)
ck("requested label is exact",'return "Walk Through Grate In Vents";' in LM)
ck("Facility modifier is reversible OFF/ON",'g_LevelModifierFacilityVentGrateActive ? "ON" : "OFF"' in LM)
ck("dump-derived STAN offsets are exact","FACILITY_VENT_GRATE_STAN_OFFSET_A 0x12627" in LM and "FACILITY_VENT_GRATE_STAN_OFFSET_B 0x12634" in LM)
ck("retail bytes are retained for OFF","FACILITY_VENT_GRATE_RETAIL_A      0x0d" in LM and "FACILITY_VENT_GRATE_RETAIL_B      0xfb" in LM)
ck("GameShark ON bytes are represented","FACILITY_VENT_GRATE_OPEN_A        0x0e" in LM and "FACILITY_VENT_GRATE_OPEN_B        0xff" in LM)
ck("no fixed GameShark RAM addresses are runtime writes","*(u8 *)0x801B3F87" not in LM and "*(u8 *)0x801B3F94" not in LM)
ck("live toggle rebuilds STAN metadata","stanRebuildRoomData();" in LM)
ck("frontend preload is independently toggleable","!g_LevelModifierFacilityVentGratePreload" in LM)
bad=[n for n,c in checks if not c]
print(); print(f"R27S FACILITY VENT GRATE AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(" - "+n)
    sys.exit(1)
