#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CHR = (ROOT / "src/game/chrprop.c").read_text()
BG = (ROOT / "src/game/bg.c").read_text()
LD = (ROOT / "ge007.ld").read_text()
MK = (ROOT / "Makefile").read_text()

checks = []

def ck(name, ok):
    ok = bool(ok)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def function_body(text, signature):
    start = text.find(signature)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    i = brace
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:i]
        i += 1
    return ""

update = function_body(CHR, "void chraiUpdateOnscreenPropCount(void)")
ptick = function_body(CHR, "void propsTickPlayer(void)")
build = function_body(BG, "static void bgBuildPortalFastCache(void)")
lookup = function_body(BG, "static s32 bgFastGetDrawnRoomIndex(s32 room)")
bbox = function_body(BG, "s32 bgGet2dBboxByRoomId(s32 room_id, struct bbox2d *result)")
portal = function_body(BG, "s32 sub_GAME_7F0B39BC(int curroom,int unk1, bbox2d * screensize, s32 next)")

ck("stale prop-room cache is no longer built before propsTickPlayer",
   "chrpropsBuildRenderRoomCache();" not in update)

ck("prop-room cache is rebuilt after weaponTickPlayer cleanup phase",
   "weaponTickPlayer(prop)" in ptick and
   "chrpropsBuildRenderRoomCache();" in ptick and
   ptick.find("chrpropsBuildRenderRoomCache();") > ptick.find("weaponTickPlayer(prop)"))

ck("late cache rebuild remains runtime Micro-Optimizations gated",
   "modMicroOptimizationsEnabled()" in ptick and
   "#if defined(GE_PHYSICAL_FASTPATHS) && defined(GE_MODDED_CHEATS)" in ptick)

ck("render-room cache still uses Expansion Pak scratch",
   "_gameFastPropRoomCacheStart" in CHR and
   "_gameFastPropRoomCacheStart = _gameFastBgCacheEnd;" in LD and
   "_gameFastPropRoomCacheEnd = _gameFastBssStart + 0x2400;" in LD)

ck("drawn-room index sidecar is explicitly initialized",
   "g_BgFastRoomDrawIndex[room] = -1;" in build)

ck("drawn-room cache hit remains range and identity validated",
   "(u32)index < (u32)g_BgNumberOfRoomsDrawn" in lookup and
   "dword_CODE_bss_8007FFA0[index].roomid == room" in lookup)

ck("bbox lookup retains retail linear fallback",
   "for (i=0; i<g_BgNumberOfRoomsDrawn; i++)" in bbox and
   "room_id == dword_CODE_bss_8007FFA0[i].roomid" in bbox)

ck("portal consolidation retains retail linear fallback",
   "g_BgNumberOfRoomsDrawn" in portal and
   "curroom == dword_CODE_bss_8007FFA0[i].roomid" in portal)

ck("original R27G2 audit remains mandatory",
   "r27g2-render-visibility-cache-audit" in MK)

prereq = next((line for line in MK.splitlines() if line.startswith("prerequisites:")), "")
ck("R27G2 R2 audit is mandatory",
   "r27g2-r2-stale-weapon-cache-audit:" in MK and
   "r27g2-r2-stale-weapon-cache-audit" in prereq)

failed = [name for name, ok in checks if not ok]

if failed:
    print(f"\nR27G2 R2 STALE-WEAPON CACHE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"\nR27G2 R2 STALE-WEAPON CACHE AUDIT: PASS ({len(checks)}/{len(checks)})")
