#!/usr/bin/env python3
from pathlib import Path
import re
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

ck("physical fast BSS keeps original 0x2400 total reservation",
   "_gameFastPropRoomCacheEnd = _gameFastBssStart + 0x2400;" in LD and
   "_gameFastBssEnd = _gameFastPropRoomCacheEnd;" in LD)
ck("final 0x200 fast-BSS window is reserved for prop render rooms",
   "_gameFastBgCacheEnd = _gameFastBssStart + 0x2200;" in LD and
   "_gameFastPropRoomCacheStart = _gameFastBgCacheEnd;" in LD and
   "== 0x200" in LD)
ck("R27G1 lower-BSS s16 cache is gone",
   "static s16 g_ModOnScreenPropRenderRoom" not in CHR)
ck("prop render-room cache uses linker-reserved u8 scratch",
   "extern u8 _gameFastPropRoomCacheStart[];" in CHR and
   "#define g_ModOnScreenPropRenderRoom ((u8 *)_gameFastPropRoomCacheStart)" in CHR and
   "ONSCREEN_PROP_LIST_LEN <= 0x200" in CHR)
ck("cache rebuild remains runtime Micro-Optimizations gated",
   re.search(r"defined\(GE_PHYSICAL_FASTPATHS\).*defined\(GE_MODDED_CHEATS\).*?"
             r"modMicroOptimizationsEnabled\(\).*?chrpropsBuildRenderRoomCache\(\);",
             CHR, re.S))
helper = re.search(r"static u8 chrpropsResolveRenderRoomFast\(PropRecord \*prop\)\s*\{(.*?)\n\}", CHR, re.S)
h = helper.group(1) if helper else ""
ck("fast resolver preserves no-STAN and viewer-STAN semantics",
   "prop->stan == NULL" in h and
   "prop->type == PROP_TYPE_VIEWER && prop->obj == NULL" in h and
   "prop->stan->room" in h)
ck("fast resolver walks authored prop rooms directly in original order",
   "for (i = 0; prop->rooms[i] != 0xff; i++)" in h and
   "g_BgRoomInfo[room].room_rendered" in h)
ck("TP local-body camera-room exception remains exact",
   "chrpropsLocalThirdPersonActive(prop)" in h and
   "return (u8)g_BgCurrentRoom;" in h)
rp = re.search(r"Gfx \*chrpropsRenderPass\(Gfx \*gdl, s32 roomid, s32 renderpass\)\s*\{(.*?)\n\}", CHR, re.S)
r = rp.group(1) if rp else ""
ck("both render membership sites use one cached byte compare when enabled",
   r.count("g_ModOnScreenPropRenderRoom[(s32)(pp - g_OnScreenPropList)] == (u8)roomid") == 2)
ck("Micro-Optimizations Off retains retail room-list fallback",
   r.count("chraiGetPropRoomIds(prop, sp48);") == 2 and
   r.count("getROOMID_isRendered(*rp)") == 2)
ck("render traversal and pass rules are unchanged",
   "for (pp = g_LastOnScreenProp; --pp >= g_OnScreenPropList; )" in r and
   "for (pp = g_OnScreenPropList; pp < g_LastOnScreenProp; pp++)" in r and
   "PROPFLAG_00000020 | PROPFLAG_RENDERPOSTBG" in r)
ck("bg fast cache owns one s16 drawn-room index per room",
   "s16 roomDrawIndex[MAXROOMCOUNT];" in BG and
   "#define g_BgFastRoomDrawIndex" in BG and
   "sizeof(struct BgFastPortalCache) <= 0x2200" in BG)
find = re.search(r"static s32 bgFastGetDrawnRoomIndex\(s32 room\)\s*\{(.*?)\n\}", BG, re.S)
f = find.group(1) if find else ""
ck("drawn-room index is self-validating instead of requiring frame clears",
   "(u32)index < (u32)g_BgNumberOfRoomsDrawn" in f and
   "dword_CODE_bss_8007FFA0[index].roomid == room" in f)
sub_start = BG.find("s32 sub_GAME_7F0B39BC(int curroom,int unk1, bbox2d * screensize, s32 next)")
sub_end = BG.find("\n\n\n/*\n* Unused function", sub_start)
s = BG[sub_start:sub_end] if sub_start >= 0 and sub_end > sub_start else ""
ck("portal visibility function range was resolved completely",
   bool(s) and "return 0;" in s and "#if defined(VERSION_EU)" in s)
ck("portal visibility consolidation starts from cached existing entry",
   "i = bgFastGetDrawnRoomIndex(curroom);" in s and
   "for (; i < g_BgNumberOfRoomsDrawn; i++)" in s)
ck("portal visibility retains linear first-match fallback and EU saturation guard",
   "if (i < 0)" in s and "i = 0;" in s and
   "if (i >= 0x78)" in s and
   "(u32)i < (u32)g_BgNumberOfRoomsDrawn" in s)
bbox = re.search(r"s32 bgGet2dBboxByRoomId\(.*?\)\s*\{(.*?)\n\}", BG, re.S)
b = bbox.group(1) if bbox else ""
ck("bbox lookup is O(1) on a valid cache hit",
   "i = bgFastGetDrawnRoomIndex(room_id);" in b and
   "if (i >= 0)" in b)
ck("bbox lookup retains exact retail linear fallback",
   "for (i=0; i<g_BgNumberOfRoomsDrawn; i++)" in b and
   "room_id == dword_CODE_bss_8007FFA0[i].roomid" in b)
ck("R27G2 audit is a mandatory build prerequisite",
   "r27g2-render-visibility-cache-audit:" in MK and
   "r27g2-render-visibility-cache-audit" in re.search(r"^prerequisites:.*$", MK, re.M).group(0))

failed = [name for name, ok in checks if not ok]
if failed:
    print(f"\nR27G2 RENDER/VISIBILITY CACHE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"\nR27G2 RENDER/VISIBILITY CACHE AUDIT: PASS ({len(checks)}/{len(checks)})")
