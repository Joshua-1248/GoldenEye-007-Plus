#!/usr/bin/env python3
from pathlib import Path
import re, sys
R=Path(__file__).resolve().parents[1]
B=(R/'src/bondconstants.h').read_text(errors='replace')
C=(R/'src/game/cheat.c').read_text(errors='replace')
F=(R/'src/game/front.c').read_text(errors='replace')
O=(R/'src/game/options.c').read_text(errors='replace')
M=(R/'src/game/mpmenu.c').read_text(errors='replace')
P=(R/'src/game/propobj.c').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(name, cond):
    checks.append((name,bool(cond)))
    print(('[PASS] ' if cond else '[FAIL] ')+name)

ck('Freeze Timer ID is appended after Level Never Ends',
   re.search(r'CHEAT_LEVEL_NEVER_ENDS,\s*CHEAT_FREEZE_TIMER,\s*#endif', B) is not None)
ck('Freeze Timer is a global SP/MP toggle cheat',
   'CHEAT_FREEZE_TIMER' in C and
   re.search(r'CHEAT_FREEZE_TIMER[^\n]*CHEAT_MASK_TOGGLE[^\n]*CHEAT_MASK_GLOBAL[^\n]*CHEAT_MASK_MPGAME[^\n]*CHEAT_MASK_SPGAME', C) is not None)
ck('main Cheat Options classifies Freeze Timer as hidden/modded entry',
   re.search(r'case CHEAT_FREEZE_TIMER:\s*case CHEAT_SUPER_TANK:', F) is not None)
ck('main Cheat Options labels it Freeze Timer',
   'case CHEAT_FREEZE_TIMER: return (u8 *)"Freeze Timer";' in F)
ck('main Cheat Options page includes Freeze Timer exactly once',
   F.count('        CHEAT_FREEZE_TIMER,') == 1)
ck('SP Watch maps appended Freeze Timer directly after Level Never Ends',
   'if (row == CHEAT_LEVEL_NEVER_ENDS)\n        return CHEAT_FREEZE_TIMER;' in O)
ck('MP/Co-op Watch maps appended Freeze Timer directly after Level Never Ends',
   'if (row == CHEAT_LEVEL_NEVER_ENDS)\n        return CHEAT_FREEZE_TIMER;' in M)
ck('runtime cheat enable/disable handlers accept Freeze Timer',
   C.count('        case CHEAT_FREEZE_TIMER:') >= 2)
ck('countdown owner includes runtime cheat API',
   '#include "cheat.h"' in P)
ck('HUD mission countdown decrement is gated by Freeze Timer',
   '&& !cheatIsActive(CHEAT_FREEZE_TIMER)' in P and
   'clock_time = clock_time - g_GlobalTimerDelta;' in P)
ck('Freeze Timer does not replace clock_enable or countdown set/get behavior',
   'void countdownTimerSetRunning(bool enable)' in P and
   'clock_enable = enable;' in P and
   'void countdownTimerSetValue(f32 time)' in P and
   'clock_time = time;' in P)
ck('V87 audit is a mandatory prerequisite',
   'freeze-timer-v87-audit:' in MK and
   'freeze-timer-v87-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))

bad=[n for n,c in checks if not c]
print()
print(f"FREEZE TIMER V87 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)

ck('frontend-selected toggle cheats are rebuilt after stage reset',
   'cheatApplyFrontendSelectionsForStage();' in (R/'src/game/lv.c').read_text(errors='replace') and
   'void cheatApplyFrontendSelectionsForStage(void)' in (R/'src/game/cheat.c').read_text(errors='replace') and
   'for (i = 0; i <= CHEAT_INVALID; i++)' in (R/'src/game/initcheattext.c').read_text(errors='replace'))
