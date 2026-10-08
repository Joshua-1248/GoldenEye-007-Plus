#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BV2 = (ROOT / 'src/game/bondview2.c').read_text()
BV = (ROOT / 'src/game/bondview.c').read_text()
GUN = (ROOT / 'src/game/gun.c').read_text()
GUNFIRE = (ROOT / 'src/game/gunfire.c').read_text()
LDS = (ROOT / 'ge007.ld').read_text()
MAKE = (ROOT / 'Makefile').read_text()

checks = []
def ck(name, cond):
    ok = bool(cond)
    checks.append(ok)
    print(('[PASS] ' if ok else '[FAIL] ') + name)

ck('one-human Multiplayer selects persistent upper body cache',
   'gamemode == GAMEMODE_MULTI || modThirdPersonActive(get_cur_playernum())' in BV2
   and 'usethirdpersonbodycache = TRUE;' in BV2)
ck('physical one-human MP cannot borrow first-person hand buffers',
   'useweaponbuffers = (getPlayerCount() == 1 && !usethirdpersonbodycache);' in BV2)
ck('nonphysical one-human MP also refuses hand-buffer body aliasing',
   '&& gamemode != GAMEMODE_MULTI\n            && !modThirdPersonActive(get_cur_playernum())' in BV2)
ck('persistent body cache remains separate upper-RDRAM reservation',
   '_thirdPersonBodyCacheStart' in LDS and '_thirdPersonBodyCacheEnd' in LDS
   and '_thirdPersonBodyCacheEnd = _thirdPersonBodyCacheStart + 0x2A000;' in LDS)
ck('cached player body uses cache instead of remove_item_in_hand branch',
   'weaponbuf0 = _thirdPersonBodyCacheStart;' in BV2
   and 'bodyheader = &g_ThirdPersonBodyHeader;' in BV2
   and 'else\n#endif\n#endif\n            {\n                remove_item_in_hand(GUNLEFT);' in BV2)
ck('one-human Multiplayer still receives normal starting weapon requests',
   'if (gamemode == GAMEMODE_MULTI)' in BV
   and 'currentPlayerEquipWeaponWrapper(GUNLEFT, starting_weapon[GUNLEFT]);' in BV
   and 'currentPlayerEquipWeaponWrapper(GUNRIGHT, starting_weapon[GUNRIGHT]);' in BV)
ck('one-human MP live CAMERAMODE_MP publishes first-person player camera state',
   'gamemode == GAMEMODE_MULTI\n        && getPlayerCount() == 1' in BV2
   and 'currentPlayerSetCameraMode(0);\n        MoveBond(' in BV2)
ck('interface first-person path updates and renders both hands',
   'gunUpdateAndFireBothHands();' in BV2
   and 'gunRenderFirstPersonGunModels(&gdl);' in BV2)
ck('first-person model loader remains enabled by unlocked hand state',
   'if ((g_CurrentPlayer->hand_invisible[hand] < 0) && (g_CurrentPlayer->lock_hand_model[hand] == 0))' in GUN)
ck('first-person renderer still rejects only nonmaterialized hands',
   'if (handptr->field_87F == 0)' in GUNFIRE)
ck('P6 bot crash hardening remains mandatory',
   'r30-p6-bot-crash-hardening-audit' in MAKE)
ck('P7 FPV audit is mandatory',
   'r30-p7-one-player-mp-fpv-audit' in MAKE
   and 'python3 scripts/audit_r30_p7_one_player_mp_fpv.py' in MAKE)

passed = sum(checks)
print(f'\nR30 P7 ONE-PLAYER MP FPV AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
sys.exit(0 if passed == len(checks) else 1)
