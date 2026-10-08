#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
L  = (R/"src/game/levelmodifiers.c").read_text(errors="replace")
H  = (R/"src/game/levelmodifiers.h").read_text(errors="replace")
B  = (R/"src/game/bondview2.c").read_text(errors="replace")
D  = (R/"src/game/debugmenu_handler.c").read_text(errors="replace")
DH = (R/"src/game/debugmenu_handler.h").read_text(errors="replace")
BS = (R/"src/boss.c").read_text(errors="replace")
O  = (R/"src/game/options.c").read_text(errors="replace")
M  = (R/"src/game/mpmenu.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks=[]
def ck(name, cond):
    checks.append((name, bool(cond)))
    print(("[PASS] " if cond else "[FAIL] ") + name)

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


ck("R27S R4 R1 marker present",
   "R27S_R4_R1_R1_MCM_HOLE_ROOM_TP_SOLO_MP" in L)

ck("Temple historical STAN links remain exact",
   all(x in L for x in [
       "0x032a, enabled ? 0x02c0 : 0x0000",
       "0x0b0a, enabled ? 0x0258 : 0x0000",
       "0x0b4a, enabled ? 0x0258 : 0x0000",
       "0x0b8a, enabled ? 0x0258 : 0x0000",
       "0x0be2, enabled ? 0x0258 : 0x0000"]))

ck("Complex historical STAN links remain exact",
   all(x in L for x in [
       "0x0452, enabled ? 0x0398 : 0x0000",
       "0x0492, enabled ? 0x0398 : 0x0000",
       "0x058a, enabled ? 0x0398 : 0x0000",
       "0x05b2, enabled ? 0x0398 : 0x0000",
       "0x05ca, enabled ? 0x0398 : 0x0000",
       "0x05ea, enabled ? 0x0398 : 0x0000",
       "0x0602, enabled ? 0x0398 : 0x0000",
       "0x1fc2, enabled ? 0x0470 : 0x0000",
       "0x1ffa, enabled ? 0x0470 : 0x0000",
       "0x2002, enabled ? 0x0470 : 0x0000"]))

ck("synthetic-room fallback is modifier scoped",
   "levelModifiersUseCurrentStanRoomForHoleTraversal" in L
   and "g_LevelModifierTempleFallActive" in L
   and "g_LevelModifierComplexFallActive" in L
   and "levelModifiersUseCurrentStanRoomForHoleTraversal" in H)

ROOM = audit_cfunc(B, "u8 bondviewGetCurrentPlayersRoom(void)")
roompos = ROOM.find("levelModifiersUseCurrentStanRoomForHoleTraversal()")
ck("synthetic hole mismatch outranks stale cameratile ownership",
   roompos >= 0
   and "return g_CurrentPlayer->field_488.current_tile_ptr->room;" in ROOM
   and "return g_CurrentPlayer->cameratile->room;" in ROOM
   and roompos < ROOM.find("return g_CurrentPlayer->cameratile->room;"))

ck("hole traversal uses current collision room only on mismatch",
   roompos >= 0
   and "g_CurrentPlayer->field_488.current_tile_ptr != NULL" in ROOM
   and "g_CurrentPlayer->field_488.current_tile_ptr->room" in ROOM
   and "g_CurrentPlayer->field_488.current_tile_ptr_for_portals->room" in ROOM
   and "return g_CurrentPlayer->field_488.current_tile_ptr->room;" in ROOM
   and "!=" in ROOM[roompos:ROOM.find("return g_CurrentPlayer->field_488.current_tile_ptr->room;")])

ck("retail portal-room fallback remains intact",
   "return g_CurrentPlayer->field_488.current_tile_ptr_for_portals->room;" in ROOM)

ck("solo Multiplayer Third Person auto-rearm accepts CAMERAMODE_MP",
   "R27S R4 R1: solo Multiplayer can remain in CAMERAMODE_MP" in B
   and "&& g_CameraMode != CAMERAMODE_MP))" in B)

ck("one-player normal MP Third Person uses selected MP character body",
   "R27S R4 R1 R1: one-player normal MP uses selected MP character" in B
   and "(getPlayerCount() == 1 && gamemode != GAMEMODE_MULTI)" in B
   and "get_player_mp_char_head(get_cur_playernum())" in B
   and "get_player_mp_char_body(get_cur_playernum())" in B)

ck("MCM still uses A activation and Start exit",
   "R27P: GE+ uses A to activate; Start is reserved for closing MCM." in D
   and "if (button_pressed & A_BUTTON)" in D
   and "if (button_pressed & START_BUTTON)" in D)

ck("MCM Start exit arms same-frame latch",
   "g_DebugMenuStartExitConsumedThisFrame = TRUE;" in D
   and "debugMenuStartExitConsumed" in DH)

ck("boss resets consumed input before MCM processing",
   "debugMenuBeginInputFrame();" in BS
   and BS.find("debugMenuBeginInputFrame();") < BS.find("debug_menu_processor("))

ck("SP gameplay rejects consumed MCM Start before fresh-edge pause handling",
   "R27S R4 R1 R1: MCM owns its closing Start edge" in B
   and "oldbuttons |= START_BUTTON;" in B)

ck("SP pause/watch rejects consumed MCM Start",
   "joyGetButtonsPressedThisFrame(PLAYER_1, START_BUTTON)" in O
   and "!debugMenuStartExitConsumed()" in O)

mpos = M.find("!debugMenuStartExitConsumed()")
ck("MP pause/watch rejects consumed MCM Start",
   mpos >= 0
   and M.find("g_CurrentPlayer->mpmenuon = TRUE;", mpos) >= 0)

prereq = next((x for x in MK.splitlines() if x.startswith("prerequisites:")), "")
ck("R27S R4 original focused audit remains mandatory",
   "r27s-r4-temple-complex-frigate-audit" in prereq)

ck("R27S R4 R1 audit is mandatory",
   "r27s-r4-r1-mcm-hole-room-tp-solo-mp-audit:" in MK
   and "r27s-r4-r1-mcm-hole-room-tp-solo-mp-audit" in prereq)

bad=[n for n,c in checks if not c]
print()
print("R27S R4 R1 R1 MCM / HOLE-ROOM / SOLO-MP-TP AUDIT: "
      + ("PASS" if not bad else "FAIL")
      + f" ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad:
        print(" - " + n)
    sys.exit(1)
