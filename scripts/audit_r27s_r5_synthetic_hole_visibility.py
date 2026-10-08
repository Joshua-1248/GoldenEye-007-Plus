#!/usr/bin/env python3
from pathlib import Path
import sys

R=Path(__file__).resolve().parents[1]
BG=(R/"src/game/bg.c").read_text(errors="replace")
LM=(R/"src/game/levelmodifiers.c").read_text(errors="replace")
BV=(R/"src/game/bondview2.c").read_text(errors="replace")
MK=(R/"Makefile").read_text(errors="replace")

C=[]
def ck(n,x):
    C.append((n,bool(x)))
    print(("[PASS] " if x else "[FAIL] ")+n)

def audit_cfunc(text, signature):
    start = text.find(signature)
    if start < 0:
        return ""
    brace = text.find("{", start)
    if brace < 0:
        return ""
    depth = 0
    state = "code"
    quote = ""
    i = brace
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "line":
            if c == "\n":
                state = "code"
        elif state == "block":
            if c == "*" and n == "/":
                state = "code"
                i += 1
        elif state == "string":
            if c == "\\":
                i += 1
            elif c == quote:
                state = "code"
        else:
            if c == "/" and n == "/":
                state = "line"
                i += 1
            elif c == "/" and n == "*":
                state = "block"
                i += 1
            elif c in ("'", '"'):
                state = "string"
                quote = c
            elif c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return text[start:i + 1]
        i += 1
    return ""


start=BG.find("void bgDetermineVisibleRooms(void)")
end=BG.find("/**\n * Address 0x7F0B8D78.",start)
vis=BG[start:end] if start>=0 and end>=0 else ""

seed=vis.find("R27S R6: seed the normal room")
drain=max(
    vis.find("while (bgProcessNextQueuedPortal(&sp44)",seed),
    vis.find("while (bgProcessNextQueuedPortal() != 0)",seed),
)
nbr=vis.find("temp_v1 = g_BgPortals[var_s0].connectedRoom1;",seed)

ck("Temple/Complex synthetic-hole policy remains modifier scoped",
   "levelModifiersUseCurrentStanRoomForHoleTraversal" in LM
   and "g_LevelModifierTempleFallActive" in LM
   and "g_LevelModifierComplexFallActive" in LM)

ROOM_VIS = audit_cfunc(BV, "u8 bondviewGetCurrentPlayersRoom(void)")

ck("synthetic collision-room ownership fallback remains",
   "levelModifiersUseCurrentStanRoomForHoleTraversal()" in ROOM_VIS
   and "g_CurrentPlayer->field_488.current_tile_ptr != NULL" in ROOM_VIS
   and "return g_CurrentPlayer->field_488.current_tile_ptr->room;" in ROOM_VIS
   and "return g_CurrentPlayer->cameratile->room;" in ROOM_VIS
   and ROOM_VIS.find("levelModifiersUseCurrentStanRoomForHoleTraversal()")
       < ROOM_VIS.find("return g_CurrentPlayer->cameratile->room;")
   and "g_CurrentPlayer->field_488.current_tile_ptr_for_portals->room" in ROOM_VIS)

ck("R6 has reusable GE-native room portal root helper",
   "R27S R6: PD-style secondary portal root." in BG
   and "static void bgQueueRootRoomPortals" in BG)

ck("root helper supports physical fast adjacency cache",
   "bgFastRoomFirstPortalNode(room)" in BG
   and "g_BgFastPortalNext[node]" in BG)

ck("root helper retains generic authored-portal fallback",
   "room == g_BgPortals[portalnum].connectedRoom1" in BG
   and "room == g_BgPortals[portalnum].connectedRoom2" in BG)

ck("normal mod camera room uses root helper",
   "bgQueueRootRoomPortals(g_BgCurrentRoom, screenbounds);" in vis)

ck("secondary root is only created for actual room mismatch",
   "current_tile_ptr_for_portals->room" in vis
   and "!= g_BgCurrentRoom" in vis)

ck("secondary root room itself is onscreen",
   "sub_GAME_7F0B39BC(" in vis[seed:drain]
   and "temp_v1" in vis[seed:drain])

ck("secondary room authored portals are recursively seeded",
   "bgQueueRootRoomPortals(temp_v1, screenbounds);" in vis)

ck("both roots enter the queue before the single GE portal drain",
   seed>=0 and drain>seed
   and vis.find("bgQueueRootRoomPortals(temp_v1, screenbounds);",seed)<drain)

ck("normal GE neighbour marking still occurs after traversal",
   drain>=0 and nbr>drain)

ck("old late room-only R5 seed is removed",
   "R27S R5 R2:" not in vis)

ck("normal GE recursive portal processor remains authoritative",
   "bgProcessNextQueuedPortal" in vis
   and "bgQueuePortalTraversal" in BG)

ck("R6 does not rewrite BG portal topology",
   "connectedRoom1 =" not in vis[seed:drain]
   and "connectedRoom2 =" not in vis[seed:drain])

ck("R6 does not rewrite STAN",
   "stanRebuildRoomData" not in vis[seed:drain])

ck("non-modded original root traversal remains source-preserved",
   "#else\n#ifdef GE_PHYSICAL_FASTPATHS" in vis)

ck("R7 synthetic room bypasses GE authored room correction",
   "R27S R7: bondview deliberately selected collision/STAN ownership" in BG
   and BG.find("R27S R7: bondview deliberately selected collision/STAN ownership")
       < BG.find("for (depth = 0, maxdepth = 11;")
   and "bgDetermineVisibleRooms();\n        return;" in BG)

pr=next((x for x in MK.splitlines() if x.startswith("prerequisites:")),"")
ck("R27S R4 R1 focused audit remains mandatory",
   "r27s-r4-r1-mcm-hole-room-tp-solo-mp-audit" in pr)

ck("R27S visibility audit remains mandatory",
   "r27s-r5-synthetic-hole-visibility-audit" in pr
   and "r27s-r5-synthetic-hole-visibility-audit:" in MK)

bad=[n for n,v in C if not v]
print()
print("R27S R6 PD-STYLE SECONDARY PORTAL ROOT AUDIT: "
      + ("PASS" if not bad else "FAIL")
      + f" ({len(C)-len(bad)}/{len(C)})")
if bad:
    for n in bad:
        print(" - "+n)
    sys.exit(1)
