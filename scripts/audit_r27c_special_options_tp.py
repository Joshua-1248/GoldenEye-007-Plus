#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
FR=(R/'src/game/front.c').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
OH=(R/'src/game/options.h').read_text(errors='replace')
MP=(R/'src/game/mpmenu.c').read_text(errors='replace')
F2=(R/'src/game/file2.c').read_text(errors='replace')
BV=(R/'src/game/bondview2.c').read_text(errors='replace')
CP=(R/'src/game/chrprop.c').read_text(errors='replace')
GF=(R/'src/game/gunfire.c').read_text(errors='replace')
CA=(R/'src/game/chraction.c').read_text(errors='replace')
PO=(R/'src/game/propobj.c').read_text(errors='replace')
BC=(R/'src/bondconstants.h').read_text(errors='replace')
SPEC=(R/'src/game/spectrum.c').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c):
    checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)

ck('Patches remains frontend-accessible and Third-Person Options is nested under Enhancements',
   'MENU_LEVEL_MODIFIERS,' in BC and 'MENU_PATCHES,' in BC and 'MENU_THIRD_PERSON_OPTIONS,' in BC)
ck('main Special Options draws Patches directly below Level Modifiers',
   FR.find('frontModGetOptionLabel(64)') < FR.find('frontModGetOptionLabel(53)') < FR.find('frontModGetOptionLabel(54)'))
ck("Main Patches keeps TP Corner/Silo X/AR33 while moved enhancement rows remain in Watch Patches",
   "frontModGetOptionLabel(i == 0 ? 55 : i == 1 ? 80 : 81)" in FR
   and all(x in FR for x in [
       "frontModGetOptionLabel(46)",
       "frontModGetOptionLabel(31)",
       "frontModGetOptionLabel(50)",
       "frontModGetOptionLabel(70)",
       "frontModGetOptionLabel(52)",
       "frontModGetOptionLabel(73)",
   ])
   and "frontModGetOptionLabel(80)" in O
   and "frontModGetOptionLabel(81)" in O
   and "row == 4 ? 70 : row == 5 ? 80 : 81" in MP)
ck('Third-Person Options contains Third Person first plus all prior controls',
   all(x in SPEC for x in ['Stay In TP On Death','TP Crouch Cam','Directional Shoulder','TP Sight Translucency','TP Cam Distance','TP Cam Height','TP Cam Horizontal','TP Cam Down Frame','TP Crouched Cam Height','TP Crosshair Range','TP World-Space Crosshair']) and
   'labelindex = i == 0 ? 32 : i <= 3 ? 46 + i : i == 4 ? 51 : 51 + i' in FR and
   'g_PlayerThirdPerson[0] ? "On" : "Off"' in FR and
   '#define MODWATCH_TP_ROWS 11' in O and 'if (mode == 6) return 11;' in MP)
ck('TP Corner Shooting Fix and TP World-Space Crosshair default Off',
   'g_ModTpCornerShootingFixEnabled = FALSE;' in O and 'g_ModTpWorldSpaceCrosshairEnabled = FALSE;' in O)
ck('new TP patch toggles persist independently in extension reserved bits',
   'GE_SRAM_EXT_RESERVED_TP_CORNER_FIX 0x02' in F2 and
   'GE_SRAM_EXT_RESERVED_TP_WORLD_CROSSHAIR 0x04' in F2 and
   'g_ModTpCornerShootingFixEnabled ? GE_SRAM_EXT_RESERVED_TP_CORNER_FIX' in F2 and
   'g_ModTpWorldSpaceCrosshairEnabled ? GE_SRAM_EXT_RESERVED_TP_WORLD_CROSSHAIR' in F2)
ck('TP Cam Distance authored default is 310 with v3-to-v4 migration',
   '#define TP_CAM_DISTANCE_DEFAULT 310' in OH and '#define GE_SRAM_EXT_VERSION         4u' in F2 and
   's32 actual = 300 + oldencoded * 5;' in F2)
ck('HUD crosshair is default TP presentation and world sight remains optional',
   '&& g_ModTpWorldSpaceCrosshairEnabled' in GF and
   '|| !g_ModTpWorldSpaceCrosshairEnabled' in GF and
   'void gunRenderThirdPersonWorldSight' in GF and
   'bondviewThirdPersonPresentationActive(get_cur_playernum())\n                && g_ModTpWorldSpaceCrosshairEnabled' in BV and
   'gunDrawSight(&gdl);' in BV)
ck('TP Corner Shooting Fix gates existing weapon-side cover validation',
   'chrpropThirdPersonResolveReticle(hand, &tpReticle,' in CP and
   'g_ModTpCornerShootingFixEnabled);' in CP and
   'if (g_PlayerIsInTank == 0 && g_ModTpCornerShootingFixEnabled)' in CP)
ck('TP Sight Translucency eases with stable opaque/translucent pass alpha',
   'g_TpSightBodyAlpha' in BV and
   'bondviewGetThirdPersonLocalBodyAlpha(s32 withalpha)' in BV and
   'if (withalpha == 0)' in BV and
   'value = (s32)g_TpSightBodyAlpha[player] + step' in BV and
   'value > targetalpha ? targetalpha : value' in BV and
   'g_TpSightBodyAlpha[player] += step' not in BV and 'g_TpSightBodyAlpha[player] - step' in BV)
ck('Moonraker TP gameplay ray stays on exact HUD crosshair axis',
   'tpLaserStableMuzzleRay = TRUE;' in CP and
   'shotdata.dir = exactworlddir;' in CP and
   'tpBeamVisualOriginValid = gunGetThirdPersonMuzzleOrigin' in CP and
   'gunSetThirdPersonResolvedBeam(hand, &tpBeamVisualOrigin, &tpbeamend);' in CP)
ck('Watch Laser TP beam is created after the authoritative firing tick',
   'item_id == ITEM_WATCHLASER && modThirdPersonActive(get_cur_playernum())' in CP and
   'gunCreateBeamForHand(hand);' in CP and
   'gunSetThirdPersonResolvedBeam(hand, &shotdata.gunpos, &tpbeamend);' in CP)
ck('TP camera sees object scenery and uses one-sided collision hysteresis',
   'chrpropThirdPersonBoundsSegmentHit' in CP and
   'g_ModThirdPersonCameraCollisionFrac' in BV and
   'Never smooth into solid geometry.' in BV and
   '0.10f * g_GlobalTimerDelta' in BV)
ck('enemy object/door impact parity is visual-only',
   'chrpropProbeObjectHitOnSegment' in CP and 'objCreateBulletImpactVisual' in PO and
   'objCreateBulletImpactVisual(&visualshot, &visualhit);' in CA)
visual=PO[PO.find('void objCreateBulletImpactVisual'):PO.find('void objHit(', PO.find('void objCreateBulletImpactVisual'))]
ck('visual-only enemy object helper does not apply damage or penetration continuation',
   visual and 'chrobjMaybeDetonateObjectIfFlags' not in visual and 'objApplyDamage' not in visual and
   'gunSetTracerTarget' not in visual and 'objDrop' not in visual)
ck('R27C audit is mandatory build prerequisite',
   'r27c-special-options-tp-audit:' in MK and
   'r27c-special-options-tp-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print(); print(f"R27C SPECIAL OPTIONS/TP AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
