#!/usr/bin/env python3
"""Verify Max Head-On Mode's stable IDs, UI routes and bounded SFX behavior."""
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[1]
read = lambda p: (ROOT / p).read_text(errors='replace')
B = read('src/bondconstants.h')
C = read('src/game/cheat.c')
F = read('src/game/front.c')
O = read('src/game/options.c')
M = read('src/game/mpmenu.c')
V = read('src/game/bondview2.c')
checks = []
def check(label, condition):
    checks.append((label, bool(condition)))
    print(('PASS ' if condition else 'FAIL ') + label)

check('append-only cheat ID (retail and previous modded IDs preserved)',
      re.search(r'CHEAT_FREEZE_TIMER,\s*CHEAT_MAX_HEAD_ON_MODE,\s*#endif\s*CHEAT_INVALID', B) is not None)
check('global SP/MP toggle registration',
      re.search(r'CHEAT_MAX_HEAD_ON_MODE[^\n]*CHEAT_MASK_TOGGLE[^\n]*CHEAT_MASK_GLOBAL[^\n]*CHEAT_MASK_MPGAME[^\n]*CHEAT_MASK_SPGAME', C) is not None)
check('both activation and deactivation supported', C.count('case CHEAT_MAX_HEAD_ON_MODE:') == 2)
check('main cheat page has exactly one entry and public label',
      F.count('        CHEAT_MAX_HEAD_ON_MODE,') == 1 and
      'case CHEAT_MAX_HEAD_ON_MODE: return (u8 *)"Max Head-On";' in F and
      'case CHEAT_MAX_HEAD_ON_MODE:\n        case CHEAT_SUPER_TANK:' in F)
for name, text, rows in [('SP Watch', O, 'MODWATCH_TOGGLE_ROWS'), ('MP/Co-Op Watch', M, 'MPWATCH_TOGGLE_ROWS')]:
    check(name + ' maps the new final row',
          'if (row == CHEAT_FREEZE_TIMER)\n        return CHEAT_MAX_HEAD_ON_MODE;' in text and
          '#define ' + rows + ' (CHEAT_INVALID - 1)' in text)
check('gameplay is modded-cheat-only and uses live cheat state',
      '#ifdef GE_MODDED_CHEATS\n/* Max Head-On Mode' in V and
      V.count('cheatIsActive(CHEAT_MAX_HEAD_ON_MODE)') == 2)
check('original one-shot crush and yell retained',
      V.count('sp6C->chrflags |= CHRFLAG_01000000;') >= 1 and
      V.count('TANK_CRUSH_MAN_SFX') >= 2 and V.count('CRUSHED_YELL_SFX') >= 2)
check('repeats bounded and throttled',
      '#define MAX_HEADON_TICKS 60' in V and
      '#define MAX_HEADON_INTERVAL 2' in V and
      'elapsed >= MAX_HEADON_TICKS' in V and
      'MAX_HEADON_SLOTS 8' in V)
check('guard collision/ACT_DIE gated and positional sound preserved',
      'if (sp6C->actiontype == ACT_DIE)' in V and
      'maxHeadOnRepeat(sp6C, &prop->pos);' in V and
      'maxHeadOnBegin(sp6C);' in V)
check('Friendly Fire reads uppercase ON/OFF in frontend multiplayer Settings',
      'g_ModMpFriendlyFireEnabled ? "ON" : "OFF"' in F)
check('Friendly Fire reads uppercase ON/OFF in in-game multiplayer Settings',
      'g_ModMpFriendlyFireEnabled ? "ON" : "OFF"' in O)
check('Freeze Timer audit accepts append-only new ID',
      'CHEAT_MAX_HEAD_ON_MODE' in read('scripts/audit_freeze_timer_v87.py'))
# Existing V86 audit skips identifiers beginning with CHEAT_MAX; independently
# parse the complete enum so Max Head-On Mode cannot be silently skipped.
block = B.split('typedef enum CHEAT_IDS', 1)[1].split('} CHEAT_ID;', 1)[0]
ids = []
for line in block.splitlines():
    line = line.strip()
    if line.startswith('CHEAT_') and not line.startswith('CHEAT_MAX ='):
        ids.append(line.split(',')[0].split('=')[0].strip())
idx = {name: i for i, name in enumerate(ids)}
kin = idx['CHEAT_KINETIC_EXPLOSIONS']
ultra = idx['CHEAT_ULTRA_KINETICS']
level = idx['CHEAT_LEVEL_NEVER_ENDS']
freeze = idx['CHEAT_FREEZE_TIMER']
headon = idx['CHEAT_MAX_HEAD_ON_MODE']
invalid = idx['CHEAT_INVALID']
selected = []
for row in range(invalid - 1):
    if row < kin: selected.append(row + 1)
    elif row == kin: selected.append(ultra)
    elif row == ultra: selected.append(level)
    elif row == level: selected.append(freeze)
    elif row == freeze: selected.append(headon)
    else: selected.append(row)
check('Watch mapping is a full permutation (no hidden or duplicate cheats)',
      len(selected) == invalid - 1 and
      sorted(selected) == list(range(1, invalid)))
failed = [label for label, passed in checks if not passed]
print('MAX HEAD-ON MODE R19 AUDIT:', 'PASS' if not failed else 'FAIL',
      f'({len(checks)-len(failed)}/{len(checks)})')
sys.exit(1 if failed else 0)
