#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
BV = (ROOT / 'src/game/bondview2.c').read_text(errors='replace')
MK = (ROOT / 'Makefile').read_text(errors='replace')
checks = []

def ck(name, cond):
    checks.append((name, bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ') + name)

ck('Fly vertical movement uses applied_view Y',
   'field_488.applied_view.f[1]' in BV and '* g_CurrentPlayer->speedforwards * g_GlobalTimerDelta' in BV)
ck('Turbo doubles Fly vertical movement instead of adding yaw-only movement',
   '(get_debug_fast_bond_flag() ? 20.0f : 10.0f)' in BV)
ck('Primary Fly X/Z movement remains look-vector based',
   BV.count('field_488.applied_view.f[0] * g_CurrentPlayer->speedforwards') >= 2 and
   BV.count('field_488.applied_view.f[2] * g_CurrentPlayer->speedforwards') >= 2)
ck('Turbo path has an explicit Fly Mode branch',
   'if (get_debug_fast_bond_flag())' in BV and
   'if (g_CheatActivated[CHEAT_FLY_MODE] && g_PlayerIsInTank == 0)' in BV)
ck('Turbo Fly forward X uses applied_view rather than yaw theta',
   '(g_CurrentPlayer->field_488.applied_view.f[0] * g_CurrentPlayer->speedforwards) -' in BV)
ck('Turbo Fly forward Z uses applied_view rather than yaw theta',
   '(g_CurrentPlayer->field_488.applied_view.f[2] * g_CurrentPlayer->speedforwards) +' in BV)
ck('Fly strafing remains horizontal/yaw-relative',
   'g_CurrentPlayer->field_488.theta_transform.f[2] * g_CurrentPlayer->speedsideways' in BV and
   'g_CurrentPlayer->field_488.theta_transform.f[0] * g_CurrentPlayer->speedsideways' in BV)
ck('Non-Fly Turbo movement keeps the retail yaw-plane fast-Bond path',
   BV.count('g_CurrentPlayer->field_488.theta_transform.f[0] * g_CurrentPlayer->speedforwards') >= 1 and
   BV.count('g_CurrentPlayer->field_488.theta_transform.f[2] * g_CurrentPlayer->speedforwards') >= 1)
ck('Third Person Fly body clears chr-local vertical gravity velocity before chrTick',
   BV.find('chr->fallspeed.y = 0.0f;', BV.find('if (tp_use_player_y)')) < BV.find('tailret = chrTick(prop);'))
post = BV.find('tailret = chrTick(prop);')
ck('Third Person Fly body re-clears vertical gravity velocity after chrTick',
   BV.find('chr->fallspeed.y = 0.0f;', post) > post)
ck('Fly body Y lock is presentation-state gated',
   BV.count('g_CheatActivated[CHEAT_FLY_MODE]\n            && bondviewThirdPersonPresentationActive(index)') >= 2)
ck('PD-style ground/manground ownership remains intact',
   BV.count('chr->ground = ppointers[index]->stanHeight;') >= 2 and
   BV.count('chr->manground = ppointers[index]->field_70;') >= 2)
ck('V87 Freeze Timer audit remains mandatory',
   'freeze-timer-v87-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')), ''))
ck('V88 audit is a mandatory build prerequisite',
   'fly-turbo-tp-y-v88-audit:' in MK and
   'fly-turbo-tp-y-v88-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')), ''))

bad = [n for n, ok in checks if not ok]
print()
print(f"FLY TURBO / TP Y V88 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for name in bad:
        print(' - ' + name)
    sys.exit(1)
