#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
s = (ROOT / "src/game/chrprop.c").read_text()

checks = []
def ck(name, ok):
    ok = bool(ok)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

ck("render-room cache exists",
   "static s16 g_ModOnScreenPropRenderRoom[ONSCREEN_PROP_LIST_LEN];" in s)

ck("cache rebuild is Micro-Optimizations gated",
   re.search(r"if\s*\(modMicroOptimizationsEnabled\(\)\)\s*\{\s*chrpropsBuildRenderRoomCache\(\);",
             s, re.S))

helper = re.search(
    r"static void chrpropsBuildRenderRoomCache\(void\)\s*\{(.*?)\n\}",
    s, re.S)
h = helper.group(1) if helper else ""

ck("cache uses retail room-id source",
   "chraiGetPropRoomIds(prop, roomids);" in h)

ck("cache keeps first-rendered-room semantics",
   re.search(r"for\s*\(rp\s*=\s*roomids;.*?getROOMID_isRendered\(\*rp\).*?"
             r"renderroom\s*=\s*\(s16\)\*rp;\s*break;", h, re.S))

ck("cache preserves TP camera-room exception",
   "chrpropsLocalThirdPersonActive(prop)" in h and
   "renderroom = (s16)g_BgCurrentRoom;" in h)

rp = re.search(
    r"Gfx \*chrpropsRenderPass\(Gfx \*gdl, s32 roomid, s32 renderpass\)\s*\{(.*?)\n\}",
    s, re.S)
r = rp.group(1) if rp else ""

ck("all render phases retain original chrpropRender calls",
   r.count("chrpropRender(gdl, prop, 0)") >= 2 and
   r.count("chrpropRender(gdl, prop, 1)") >= 1)

ck("both membership sites use cached O(1) compare when enabled",
   r.count("g_ModOnScreenPropRenderRoom[(s32)(pp - g_OnScreenPropList)] == roomid") == 2)

ck("Micro-Optimizations Off retains original fallback",
   r.count("chraiGetPropRoomIds(prop, sp48);") == 2 and
   r.count("getROOMID_isRendered(*rp)") == 2)

ck("prop traversal direction is unchanged",
   "for (pp = g_LastOnScreenProp; --pp >= g_OnScreenPropList; )" in r and
   "for (pp = g_OnScreenPropList; pp < g_LastOnScreenProp; pp++)" in r)

ck("render-pass flag rules remain present",
   "PROPFLAG_00000020 | PROPFLAG_RENDERPOSTBG" in r and
   "prop->flags & PROPFLAG_00000020" in r)

failed = [n for n, ok in checks if not ok]
if failed:
    print(f"\nR27G1 RENDER PROP-ROOM CACHE AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for n in failed:
        print(" - " + n)
    sys.exit(1)

print(f"\nR27G1 RENDER PROP-ROOM CACHE AUDIT: PASS ({len(checks)}/{len(checks)})")
