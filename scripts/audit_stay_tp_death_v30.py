#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = []

def check(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

opt = (ROOT/'src/game/options.c').read_text(errors='replace')
oph = (ROOT/'src/game/options.h').read_text(errors='replace')
file2 = (ROOT/'src/game/file2.c').read_text(errors='replace')
file2h = (ROOT/'src/game/file2.h').read_text(errors='replace')
front = (ROOT/'src/game/front.c').read_text(errors='replace')
mpm = (ROOT/'src/game/mpmenu.c').read_text(errors='replace')
bv = (ROOT/'src/game/bondview2.c').read_text(errors='replace')
spec = (ROOT/'src/game/spectrum.c').read_text(errors='replace')
mk = (ROOT/'Makefile').read_text(errors='replace')

check('per-player Stay In TP On Death state exists', 'g_PlayerStayInTpOnDeath[MAX_PLAYER_COUNT]' in opt and 'extern u8 g_PlayerStayInTpOnDeath[MAX_PLAYER_COUNT]' in oph)
check('save-backed default exists', 'g_ModStayInTpOnDeathDefault' in opt and 'g_ModStayInTpOnDeathDefault' in file2)
check('V29 camera pack reuses bit 26 for Stay In TP without growing EEPROM', 'MOD_CAMERA_STAY_TP_DEATH_BIT' in file2h and 'MOD_CAMERA_PACK_SIGNATURE3' in file2h and 'sizeof(save_data)' not in file2h)
check('old V29 signature remains migration-compatible', '((packed >> 27) & 0x07) != MOD_CAMERA_PACK_SIGNATURE3' in file2)
check('folder load initializes every player from saved default', 'g_PlayerStayInTpOnDeath[i] = g_ModStayInTpOnDeathDefault' in file2)
check('SP Watch exposes and toggles Stay In TP On Death in Third-Person Options', 'frontModGetOptionLabel(47)' in opt and 'g_PlayerStayInTpOnDeath[player] ^= 1' in opt and 'MODWATCH_MODE_TP_OPTIONS' in opt)
check('SP Watch persists Stay In TP default', 'g_ModStayInTpOnDeathDefault = g_PlayerStayInTpOnDeath[player]' in opt and 'fileStoreThirdPersonCameraSettings(save);' in opt)
check('MP Watch has independent per-player toggle', 'row == 10) g_PlayerStayInTpOnDeath[player] ^= 1' in mpm and 'row == 10) return g_PlayerStayInTpOnDeath[player] != 0' in mpm)
check('main-menu Third-Person Options exposes save-backed setting', 'frontModGetOptionLabel(54)' in front and 'frontModGetOptionLabel(labelindex)' in front and 'g_ModStayInTpOnDeathDefault ^= 1' in front)
check('death no longer suppresses TP when player opted in', 'p->bonddead && !modStayInTpOnDeath(player)' in bv)
check('scripted/cinematic death camera guard is still preserved', 'g_CameraMode != CAMERAMODE_NONE' in bv and 'g_CameraMode != CAMERAMODE_FP' in bv)
check('label text is exact', 'Stay In TP On Death' in spec)
check('V30 audit is mandatory build prerequisite', 'stay-tp-death-v30-audit' in mk and 'scripts/audit_stay_tp_death_v30.py' in mk)

passed = sum(v for _,v in checks)
print(f'\nSTAY IN TP ON DEATH V30 AUDIT: {"PASS" if passed == len(checks) else "FAIL"} ({passed}/{len(checks)})')
if passed != len(checks):
    sys.exit(1)
