#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
F = (ROOT / "src/game/front.c").read_text(errors="replace")
M = (ROOT / "Makefile").read_text(errors="replace")

checks = []

def ck(name, cond):
    ok = bool(cond)
    checks.append((name, ok))
    print(("[PASS] " if ok else "[FAIL] ") + name)

def fn(text, sig):
    # Return the actual function definition, not a forward declaration.
    search = 0

    while True:
        s = text.find(sig, search)

        if s < 0:
            return ""

        after = s + len(sig)

        while after < len(text) and text[after].isspace():
            after += 1

        if after < len(text) and text[after] == "{":
            b = after
            d = 0

            for i in range(b, len(text)):
                if text[i] == "{":
                    d += 1
                elif text[i] == "}":
                    d -= 1

                    if d == 0:
                        return text[s:i+1]

            return ""

        # Prototype or otherwise not a definition. Search for the next match.
        search = after

preset = fn(F, "static void frontModApplyQuickSettingsPreset(void)")
checkcombo = fn(F, "static s32 frontModCheckQuickSettingsPresetCombo(void)")
modesel = fn(F, "void interface_menu06_modesel(void)")

ck("combo is exactly L+R+all four C buttons",
   "L_TRIG | R_TRIG | U_CBUTTONS | D_CBUTTONS | L_CBUTTONS | R_CBUTTONS" in F)

ck("held chord requires a fresh edge and cannot EEPROM-write every held frame",
   "joyGetButtons(PLAYER_1, combo) == combo" in checkcombo and
   "joyGetButtonsPressedThisFrame(PLAYER_1, combo) != 0" in checkcombo)

ck("combo hook exists exactly once and only in Bond-portrait MODE SELECT",
   F.count("frontModCheckQuickSettingsPresetCombo();") == 1 and
   "frontModCheckQuickSettingsPresetCombo();" in modesel)

mainblock = preset[preset.find("save->options ="):preset.find("g_ModGameplayOptions2 =")]
ck("main Options exact preset is encoded",
   all(x in mainblock for x in [
       "OPTION_SIGHTONSCREEN",
       "OPTION_DISPLAYAMMO",
       "OPTION_HEADROLL",
       "OPTION_CROSSHAIR",
       "CONTROLLER_CONFIG_SOLITARE"
   ]) and
   "save->music_vol = 0;" in preset and
   "save->sfx_vol = 255;" in preset and
   all(x not in mainblock for x in [
       "OPTION_AUTOAIM",
       "OPTION_AIMCONTROL",
       "OPTION_LOOKAHEAD",
       "OPTION_INVERTLOOK",
       "OPTION_SCREENWIDE",
       "OPTION_SCREENCINEMA",
       "OPTION_SCREENRATIO",
       "OPTION_SCREENRATIO2"
   ]))

opt2block = preset[preset.find("g_ModGameplayOptions2 ="):preset.find("save->mod_options2")]
ck("Special Options exact positive/negative semantics are encoded",
   all(x in opt2block for x in [
       "MODOPT2_ENDLESS_DEATHCAM",
       "MODOPT2_REALTIME_COLLAPSE",
       "MODOPT2_DISABLE_HITSTUN",
       "MODOPT2_DISABLE_KNOCKBACK",
       "MODOPT2_DISABLE_DAMAGE_SFX",
       "MODOPT2_DISABLE_NOISE_DITHER"
   ]) and
   "MODOPT2_DAMAGE_FLASH" not in opt2block and
   "g_ModEnemyBulletHolesEnabled = TRUE;" in preset)

ck("Patches exact preset is encoded",
   "MODOPT3_ENABLE_MICROOPT" in preset and
   "g_ModAntiAliasingEnabled = FALSE;" in preset and
   "g_ModTpCornerShootingFixEnabled = FALSE;" in preset)

ck("Third-Person exact screenshot values are encoded",
   all(x in preset for x in [
       "g_ModStayInTpOnDeathDefault = TRUE;",
       "MODOPT3_TP_CROUCH_CAM",
       "MODOPT3_DIRECTIONAL_SHOULDER",
       "g_ModTpSightTranslucencyEnabled = TRUE;",
       "310 - TP_CAM_DISTANCE_DEFAULT",
       "-12 - TP_CAM_HEIGHT_DEFAULT",
       "-24 - TP_CAM_HORIZONTAL_DEFAULT",
       "24 - TP_CAM_DOWN_FRAME_DEFAULT",
       "46 - TP_CROUCH_CAM_HEIGHT_DEFAULT",
       "g_ModThirdPersonCrosshairRange = 500;",
       "g_ModTpWorldSpaceCrosshairEnabled = FALSE;"
   ]))

commit = fn(F, "static void frontModCommitQuickSettingsPresetIfDirty(void)")

ck("preset prepares camera mirror but defers persistent writes",
   "fileStoreThirdPersonCameraSettings(save);" in preset and
   "g_ModQuickSettingsPresetDirty = TRUE;" in preset and
   "fileWriteSave(save);" not in preset and
   "fileStoreExtendedSettings(save);" not in preset and
   "fileLoadSettingsForFolder(selected_folder_num);" not in preset)

ck("quick preset commits legacy plus extension state only from deferred helper",
   "if (!g_ModQuickSettingsPresetDirty)" in commit and
   "fileWriteSave(save);" in commit and
   "fileStoreExtendedSettings(save);" in commit and
   "g_ModQuickSettingsPresetDirty = FALSE;" in commit)

ck("MODE SELECT exit paths flush the deferred quick preset",
   modesel.count("frontModCommitQuickSettingsPresetIfDirty();") >= 2 and
   "Any navigation out of MODE SELECT commits" in modesel)

ck("preset applies menu audio immediately",
   "frontModApplyMusicVolume(save);" in preset and
   "frontModApplySfxVolume(save);" in preset)

ck("all four MP control styles are 1.2 Solitaire",
   "controlstyle_player[i] = CONTROLLER_CONFIG_SOLITARE;" in preset)

ck("all MP players use Reverse/AutoAimOff/Hold/SightOn/LookAheadOff/AmmoOn/HeadRollOn",
   "g_MpPlayerOptions[i] =" in preset and
   "MP_PLAYEROPT_SIGHT" in preset and
   "MP_PLAYEROPT_AMMO" in preset and
   "MP_PLAYEROPT_HEADROLL" in preset and
   "MP_PLAYEROPT_AUTOAIM" not in preset and
   "MP_PLAYEROPT_AIMCONTROL" not in preset and
   "MP_PLAYEROPT_LOOKAHEAD" not in preset)

ck("MP P1 damage sound Off while P2/P3/P4 are On",
   "if (i != PLAYER_1)" in preset and
   "g_MpPlayerOptions[i] |= MP_PLAYEROPT_DAMAGE_SFX;" in preset)

ck("MP Crosshair On and Third Person Off for all four",
   "g_MpPlayerCrosshair[i] = TRUE;" in preset and
   "g_PlayerThirdPerson[i] = FALSE;" in preset)

ck("MP No Radar Off, viewport lock None, Kill Count Message Off",
   "g_CheatActivated[CHEAT_NO_RADAR_MP] = FALSE;" in preset and
   "g_MpViewportLock = 0;" in preset and
   "g_MpKillCountMessageEnabled = FALSE;" in preset)

prereq = next((line for line in M.splitlines() if line.startswith("prerequisites:")), "")
ck("R27H audit is a mandatory build prerequisite",
   "r27h-quick-settings-preset-audit:" in M and
   "r27h-quick-settings-preset-audit" in prereq)

failed = [name for name, ok in checks if not ok]
if failed:
    print(f"\nR27H QUICK SETTINGS PRESET AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})")
    for name in failed:
        print(" - " + name)
    sys.exit(1)

print(f"\nR27H QUICK SETTINGS PRESET AUDIT: PASS ({len(checks)}/{len(checks)})")
