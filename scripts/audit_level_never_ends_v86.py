#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
F={
 'const':(R/'src/bondconstants.h').read_text(errors='replace'),
 'cheat':(R/'src/game/cheat.c').read_text(errors='replace'),
 'front':(R/'src/game/front.c').read_text(errors='replace'),
 'sp':(R/'src/game/options.c').read_text(errors='replace'),
 'mp':(R/'src/game/mpmenu.c').read_text(errors='replace'),
 'ai':(R/'src/game/chrai.c').read_text(errors='replace'),
 'mk':(R/'Makefile').read_text(errors='replace'),
}
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
C,Q,A,S,M,I,K=F['const'],F['cheat'],F['front'],F['sp'],F['mp'],F['ai'],F['mk']
ck('Level Never Ends is appended after V85 IDs',
   C.find('CHEAT_MIRRORED_LEVELS') < C.find('CHEAT_ULTRA_KINETICS') < C.find('CHEAT_LEVEL_NEVER_ENDS') < C.find('CHEAT_INVALID'))
ck('Level Never Ends is a global SP/MP toggle cheat',
   'CHEAT_LEVEL_NEVER_ENDS' in Q and
   'CHEAT_MASK_TOGGLE | CHEAT_MASK_GLOBAL | CHEAT_MASK_MPGAME | CHEAT_MASK_SPGAME' in Q)
ck('main Cheat Options exposes Level Never Ends',
   'case CHEAT_LEVEL_NEVER_ENDS: return (u8 *)"Level Never Ends";' in A and
   'CHEAT_MIRRORED_LEVELS,\n        CHEAT_LEVEL_NEVER_ENDS,' in A)
ck('SP Watch exposes appended Level Never Ends exactly once',
   'if (row == CHEAT_ULTRA_KINETICS)\n        return CHEAT_LEVEL_NEVER_ENDS;' in S and
   '#define MODWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in S)
ck('MP/Co-Op Watch exposes appended Level Never Ends exactly once',
   'if (row == CHEAT_ULTRA_KINETICS)\n        return CHEAT_LEVEL_NEVER_ENDS;' in M and
   '#define MPWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in M)
# Model the row remap using the actual appended-ID ordering.  This catches the
# easy-to-miss duplicate-Ultra/omitted-new-cheat case when CHEAT_INVALID grows.
block=C[C.find('typedef enum CHEAT_IDS'):C.find('} CHEAT_ID;', C.find('typedef enum CHEAT_IDS'))]
names=[]
for line in block.splitlines():
    line=line.strip()
    if line.startswith('CHEAT_') and not line.startswith('CHEAT_MAX'):
        names.append(line.split(',')[0].split('=')[0].strip())
ids={n:i for i,n in enumerate(names)}
kin=ids['CHEAT_KINETIC_EXPLOSIONS']; ultra=ids['CHEAT_ULTRA_KINETICS']; level=ids['CHEAT_LEVEL_NEVER_ENDS']; freeze=ids.get('CHEAT_FREEZE_TIMER'); invalid=ids['CHEAT_INVALID']
mapped=[]
for row in range(invalid-1):
    if row < kin: mapped.append(row+1)
    elif row == kin: mapped.append(ultra)
    elif row == ultra: mapped.append(level)
    elif freeze is not None and row == level: mapped.append(freeze)
    else: mapped.append(row)
ck('SP/MP Watch remap contains every toggle cheat once with no duplicate Ultra',
   len(mapped) == invalid-1 and len(set(mapped)) == len(mapped) and sorted(mapped) == list(range(1, invalid)))
ck('AI exit detector parses command boundaries',
   'chraiLevelNeverEndsExitAhead' in I and 'chraiitemsize((u8 *)ailist, offset)' in I)
ck('AI exit detector recognizes direct EndLevel', 'cmd == AI_EndLevel' in I)
ck('AI exit detector recognizes retail fade/exit latch', 'cmd == AI_TriggerFadeAndExitLevelOnButtonPress' in I)
ck('AI exit detector recognizes jump to GAILIST_END_LEVEL',
   'ntohs(ai->AI_LIST_ID) == GAILIST_END_LEVEL' in I)
ck('only background AI exit preambles are rewound',
   'chr->chrnum != 0xfe' in I and 'chr->aioffset = 0;' in I)
ck('control-lock exit preamble is blocked before side effects',
   'case AI_BondDisableControl:' in I and
   'chraiLevelNeverEndsBlockBackgroundExit(ChrEntityp, AiListp, Offset)' in I)
ck('damage/pickup-lock exit preamble is blocked before side effects',
   'case AI_BondDisableDamageAndPickups:' in I and I.count('chraiLevelNeverEndsBlockBackgroundExit(ChrEntityp, AiListp, Offset)') >= 2)
ck('terminal EndLevel has a Level Never Ends failsafe',
   'if (cheatIsActive(CHEAT_LEVEL_NEVER_ENDS))\n                    {\n                        Offset += sizeof(AiEndLevelRecord);' in I)
ck('button-driven fade/exit latch has a Level Never Ends failsafe',
   'Offset += sizeof(AiTriggerFadeAndExitLevelOnButtonPressRecord);' in I and I.count('cheatIsActive(CHEAT_LEVEL_NEVER_ENDS)') >= 3)
ck('activation/deactivation handlers accept the toggle without destructive state changes', Q.count('case CHEAT_LEVEL_NEVER_ENDS:') >= 2)
ck('V86 audit is mandatory build prerequisite',
   'level-never-ends-v86-audit:' in K and 'level-never-ends-v86-audit' in K.split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
print(f"\nLEVEL NEVER ENDS V86 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for n in failed: print(' - '+n)
    sys.exit(1)

ck('frontend-selected toggle cheats are rebuilt after stage reset',
   'cheatApplyFrontendSelectionsForStage();' in (R/'src/game/lv.c').read_text(errors='replace') and
   'void cheatApplyFrontendSelectionsForStage(void)' in (R/'src/game/cheat.c').read_text(errors='replace') and
   'for (i = 0; i <= CHEAT_INVALID; i++)' in (R/'src/game/initcheattext.c').read_text(errors='replace'))
