#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
O  = (R/"src/game/options.c").read_text(errors="replace")
OH = (R/"src/game/options.h").read_text(errors="replace")
ROH = (R/"options.h").read_text(errors="replace")
F2 = (R/"src/game/file2.c").read_text(errors="replace")
FR = (R/"src/game/front.c").read_text(errors="replace")
SP = (R/"src/game/spectrum.c").read_text(errors="replace")
BV = (R/"src/game/bondview2.c").read_text(errors="replace")
GF = (R/"src/game/gunfire.c").read_text(errors="replace")
GU = (R/"src/game/gun.c").read_text(errors="replace")
PL = (R/"src/game/player.c").read_text(errors="replace")
MP = (R/"src/game/mpmenu.c").read_text(errors="replace")
MK = (R/"Makefile").read_text(errors="replace")

checks=[]
def ck(name, cond):
    cond=bool(cond)
    checks.append((name,cond))
    print(("[PASS] " if cond else "[FAIL] ")+name)

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


ck("Melee Quick-Swap defaults Off and is exported through both options headers",
   "u8 g_ModMeleeQuickSwapEnabled = FALSE;" in O
   and all(x in OH for x in [
       "extern u8 g_ModMeleeQuickSwapEnabled;",
       "void modMeleeQuickSwapSetEnabled(s32 enabled);",
       "void modMeleeQuickSwapResetPlayerSelection(s32 player);",
       "void modMeleeQuickSwapLockPlayerSelection(s32 player);",
       "s32 modMeleeQuickSwapPlayerSelectionLocked(s32 player);",
   ])
   and all(x in ROH for x in [
       "extern u8 g_ModMeleeQuickSwapEnabled;",
       "void modMeleeQuickSwapSetEnabled(s32 enabled);",
       "void modMeleeQuickSwapResetPlayerSelection(s32 player);",
       "void modMeleeQuickSwapLockPlayerSelection(s32 player);",
       "s32 modMeleeQuickSwapPlayerSelectionLocked(s32 player);",
   ]))

ck("manual style uses retail per-player selector rather than growing struct player",
   "cur_item_weapon_getname" in BV
   and "cur_item_weapon_getname" in GF
   and "g_ModMeleeQuickSwapLockedPlayers" in O
   and "modMeleeQuickSwapResetPlayerSelection(player_num);" in PL)

ck("B must already be held before a fresh physical Z press",
   "R27T: B-held-first then fresh-Z" in BV
   and "(buttons & B_BUTTON)" in BV
   and "(oldbuttons & B_BUTTON)" in BV
   and "((buttons & ~oldbuttons) & Z_TRIG)" in BV)

ck("shortcut is restricted to GoldenEye's Fist/melee slot",
   "getCurrentPlayerWeaponId(GUNRIGHT) == ITEM_FIST" in BV)

ck("Sniper Butt selection requires the Sniper Rifle to be owned",
   "bondinvItemAvailable(ITEM_SNIPERRIFLE)" in BV)

ck("shortcut consumes fire/action/cycle/detonation side effects",
   all(x in BV for x in [
       "moveData.triggerOn = 0;",
       "moveData.btap = 0;",
       "moveData.weaponBackOffset = 0;",
       "moveData.weaponForwardOffset = 0;",
       "moveData.detonating = 0;",
   ]))

ck("B->Z waits for melee idle, then uses retail lower/swap/raise with the correct model",
   "R27T R5: B->Z only queues the alternate melee presentation" in BV
   and "weapon_action_state == GUN_ANIM_STATE_IDLE" in BV
   and "weapon_action_state = GUN_ANIM_STATE_SWITCH_LOWER;" in BV
   and "R27T R8 R1: compact form of the same selector rules." in GF
   and "g_CurrentPlayer->hand_invisible[hand] = -1;" in GF
   and "g_CurrentPlayer->field_2A44[hand] = ITEM_FIST;" in GF
   and "modMeleeQuickSwapLockPlayerSelection(get_cur_playernum());" in GF)

ck("manual choice and queued swap are isolated separately per player",
   "modMeleeQuickSwapQueuePlayerSelection(get_cur_playernum());" in BV
   and "void modMeleeQuickSwapQueuePlayerSelection(s32 player)" in O
   and "R27T R7: queue only." in O
   and "(1u << (player + MAX_PLAYER_COUNT))" in O
   and "modMeleeQuickSwapPlayerSelectionPending(get_cur_playernum())" in GF
   and "modMeleeQuickSwapLockPlayerSelection(get_cur_playernum());" in GF
   and "g_ModMeleeQuickSwapLockedPlayers |= 1u << player;" in O
   and "~(1u << (player + MAX_PLAYER_COUNT))" in O)

ck("weapon cycling preserves explicit choice but retains retail automatic behavior before first toggle",
   "R27T: preserve an explicitly selected melee style" in GF
   and "modMeleeQuickSwapPlayerSelectionLocked(get_cur_playernum())" in GF
   and GF.count("bondinvItemAvailable(ITEM_SNIPERRIFLE)") >= 2)

ck("lost Sniper Rifle safely falls back to Fist",
   "bondinvItemAvailable(ITEM_SNIPERRIFLE) == 0" in GF
   and "cur_item_weapon_getname = ITEM_FIST;" in GF)

ck("packed label 73 is Melee Quick-Swap and 52 remains Enemy Bullet Holes",
   "index == 73" in SP
   and "0x4D656C65" in SP
   and "0x65205175" in SP
   and "0x69636B2D" in SP
   and "0x53776170" in SP
   and "index == 52" in SP
   and "Enemy Bullet Holes" in SP)

ck("main-menu Melee Quick-Swap is owned by Enhancements after submenu compaction",
   "page2labels[10] = {12,13,14,15,30,16,8,29,74,76}" in FR
   and "g_ModOptionsPage ? 10 : 11" in FR
   and "g_ModOptionsPage ? 9 : 10" in FR
   and 'frontModGetOptionLabel(73)' in FR
   and 'g_ModMeleeQuickSwapEnabled ? "On" : "Off"' in FR
   and "MENU_EXPERIMENTAL_OPTIONS" in FR
   and "MENU_ENHANCEMENTS_OPTIONS" in FR)

ck("SP Watch keeps direct Melee row and all submenu hierarchy",
   "#define MODWATCH_OPTION_ROWS 18" in O
   and "frontModGetOptionLabel(73)" in O
   and "MODWATCH_MODE_PATCHES" in O
   and "MODWATCH_MODE_TP_OPTIONS" in O
   and "MODWATCH_MODE_DEBUG" in O
   and "MODWATCH_MODE_EXPERIMENTAL" in O
   and "MODWATCH_MODE_ENHANCEMENTS" in O
   and "row == 14" in O
   and "row == 15" in O
   and "row == 16" in O)

ck("SP Watch descriptions explain Melee Quick-Swap and damage/death options",
   all(x in O for x in [
       "Allows switching melee weapon type from Slappers/Sniper Rifle Buttstock. Activate with holding B, then Z.",
       "When killed, the head animation matches the speed of the players' death animation.",
       "When disabled, allows the player to shoot even while taking damage.",
       "When disabled, turns off the white flashing that occurs when taking damage.",
       "When disabled, turns off the sound that plays whenever the player takes damage.",
       "modWatchSpecialOptionDescription(selected)",
       "textWrap(0xd0, description, wrappedDescription",
   ]))

ck("SP Watch root reserves a seventh row slot for option descriptions",
   "visibleRows = mode == MODWATCH_MODE_SPECIAL && !deathsubmenu ? 6 : MODWATCH_VISIBLE_ROWS;" in O
   and "row < top + visibleRows" in O
   and "draw_watch_mod_text(gdl,0x40,0xab,wrappedDescription" in O)

ck("MP Watch keeps direct Melee row and all submenu hierarchy",
   "if (mode == 2) return 19;" in MP
   and "frontModGetOptionLabel(73)" in MP
   and "mpcfg_mode[player]=7" in MP
   and "mpcfg_mode[player]=8" in MP
   and "mpcfg_mode[player]=9" in MP
   and "mpcfg_row[player]=15" in MP
   and "mpcfg_row[player]=16" in MP
   and "mpcfg_row[player]=17" in MP)

ck("MP Melee Quick-Swap respects R27E deferred-write policy",
   "mpcfg_globals_dirty = TRUE;" in MP
   and "modMeleeQuickSwapSetEnabled(!g_ModMeleeQuickSwapEnabled);" in MP
   and "modMeleeQuickSwapSetEnabled(!g_ModMeleeQuickSwapEnabled);\n        mpwatchConfigStoreGlobals();" not in MP)

ck("save journal uses genuinely free reserved bit 0x40 without record growth",
   "#define GE_SRAM_EXT_RESERVED_MELEE_QUICK_SWAP 0x40" in F2
   and "#define GE_SRAM_EXT_FLAG_ENEMY_BULLET_HOLES 0x20" in F2
   and "GE_SRAM_EXT_FLAG_MELEE_QUICK_SWAP" not in F2
   and "#define GE_SRAM_EXT_RECORD_SIZE     8u" in F2
   and "sizeof(GeSramExtFolderRecord) == GE_SRAM_EXT_RECORD_SIZE" in F2)

ck("extension load/store round-trips Melee Quick-Swap",
   "record->reserved & GE_SRAM_EXT_RESERVED_MELEE_QUICK_SWAP" in F2
   and "g_ModMeleeQuickSwapEnabled ? GE_SRAM_EXT_RESERVED_MELEE_QUICK_SWAP" in F2)

ck("old/no-extension folders default Melee Quick-Swap Off",
   "modMeleeQuickSwapSetEnabled(FALSE);" in F2
   and "modMeleeQuickSwapSetEnabled(FALSE);" in FR)

ROOM_R27T = audit_cfunc(BV, "u8 bondviewGetCurrentPlayersRoom(void)")

ck("R27S hole-room and solo-MP Third Person repairs remain present",
   "levelModifiersUseCurrentStanRoomForHoleTraversal()" in ROOM_R27T
   and "return g_CurrentPlayer->field_488.current_tile_ptr->room;" in ROOM_R27T
   and "return g_CurrentPlayer->cameratile->room;" in ROOM_R27T
   and ROOM_R27T.find("levelModifiersUseCurrentStanRoomForHoleTraversal()")
       < ROOM_R27T.find("return g_CurrentPlayer->cameratile->room;")
   and "gamemode == GAMEMODE_MULTI" in BV
   and "getPlayerCount() == 1" in BV
   and "g_CameraMode == CAMERAMODE_MP" in BV
   and "bondviewThirdPersonPresentationActive" in BV)

ck("one-player Multiplayer TP toggle keeps live simulation and leaves camera choice to presentation state",
   "R27T R4: one-player Multiplayer CAMERAMODE_MP is an underlying" in BV
   and "gamemode == GAMEMODE_MULTI" in BV
   and "getPlayerCount() == 1" in BV
   and "bondviewThirdPersonPresentationActive" in BV)

ck("active Fist/Sniper-Butt attacks are never interrupted by the quick-swap request",
   "weapon_action_state == GUN_ANIM_STATE_IDLE" in BV
   and "modMeleeQuickSwapQueuePlayerSelection(get_cur_playernum());" in BV
   and all(x in GF for x in [
       "GUN_ANIM_STATE_PUNCH1_STRIKE",
       "GUN_ANIM_STATE_PUNCH1_RECOVER",
       "GUN_ANIM_STATE_PUNCH2_STRIKE",
       "GUN_ANIM_STATE_PUNCH2_RECOVER",
   ]))

ck("first melee quick-swap uses the resident model as source of truth",
   "r27tPreviousMeleePresentation =" in GF
   and "g_CurrentPlayer->hand_item[hand]" in GF
   and "r27tPreviousMeleePresentation == ITEM_SNIPERRIFLE" in GF
   and "r27tPreviousMeleePresentation != ITEM_FIST" in GF
   and "R27T R8 R1: compact form of the same selector rules." in GF)

ck("R6 first-swap temporary obeys C89 declaration ordering",
   GF.count("enum ITEM_IDS r27tPreviousMeleePresentation;") == 3
   and "ITEM_IDS r27tPreviousMeleePresentation =" not in GF
   and "r27tPreviousMeleePresentation =\n                    g_CurrentPlayer->hand_item[hand];" in GF)

ck("first All-Guns Sniper Butt manual toggle goes to Fist",
   "R27T R8 R1: compact form of the same selector rules." in GF
   and "else if (bondinvGetAllGunsFlag()" in GF
   and "g_CurrentPlayer->cur_item_weapon_getname == ITEM_SNIPERRIFLE" in GF
   and "g_CurrentPlayer->cur_item_weapon_getname = ITEM_FIST;" in GF
   and "modMeleeQuickSwapLockPlayerSelection(get_cur_playernum());" in GF)

ck("Fist/Sniper Butt model reload bypasses same-slot suppression",
   "R27T R8 R1: compact form of the same selector rules." in GF
   and "g_CurrentPlayer->hand_invisible[hand] = -1;" in GF
   and "g_CurrentPlayer->field_2A44[hand] = ITEM_FIST;" in GF
   and "gunReloadMeleeQuickSwapPresentation" not in GF
   and "gunReloadMeleeQuickSwapPresentation" not in GU)

ck("post-first melee toggles use selector authority while ITEM_FIST remains shared slot",
   "R27T R8 R1: compact form of the same selector rules." in GF
   and "modMeleeQuickSwapPlayerSelectionLocked(get_cur_playernum())" in GF
   and "g_CurrentPlayer->cur_item_weapon_getname == ITEM_SNIPERRIFLE" in GF
   and "g_CurrentPlayer->cur_item_weapon_getname = ITEM_FIST;" in GF
   and "g_CurrentPlayer->cur_item_weapon_getname = ITEM_SNIPERRIFLE;" in GF)

ck("R8 R1 commit helper clears pending without a second runtime call",
   "g_ModMeleeQuickSwapLockedPlayers |= 1u << player;" in O
   and "~(1u << (player + MAX_PLAYER_COUNT))" in O
   and "modMeleeQuickSwapClearPlayerSelectionPending(get_cur_playernum());" not in GF)

pr=next((x for x in MK.splitlines() if x.startswith("prerequisites:")),"")
ck("R27T audit is mandatory",
   "r27t-melee-quick-swap-audit:" in MK
   and "r27t-melee-quick-swap-audit" in pr)

bad=[n for n,c in checks if not c]
print()
print("R27T CURRENT-BASELINE MELEE QUICK-SWAP AUDIT: "
      + ("PASS" if not bad else "FAIL")
      + f" ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad:
        print(" - "+n)
    sys.exit(1)
