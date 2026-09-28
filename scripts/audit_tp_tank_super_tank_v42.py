#!/usr/bin/env python3
from pathlib import Path
R = Path(__file__).resolve().parents[1]
files = {
    'const': (R/'src/bondconstants.h').read_text(),
    'cheat': (R/'src/game/cheat.c').read_text(),
    'front': (R/'src/game/front.c').read_text(),
    'bond': (R/'src/game/bondview2.c').read_text(),
    'chrprop': (R/'src/game/chrprop.c').read_text(),
    'opt': (R/'src/game/options.c').read_text(),
    'mk': (R/'Makefile').read_text(),
}
checks=[]
def ck(desc, ok):
    checks.append((desc,bool(ok)))
    print(('[PASS] ' if ok else '[FAIL] ') + desc)

const=files['const']; cheat=files['cheat']; front=files['front']; bond=files['bond']; chrprop=files['chrprop']
ck('Super Tank ID remains after legacy Super Kinetics and before Mirrored Levels',
   const.find('CHEAT_KINETIC_EXPLOSIONS') < const.find('CHEAT_SUPER_TANK') < const.find('CHEAT_MIRRORED_LEVELS') < const.find('CHEAT_INVALID'))
ck('Super Tank is a global SP/MP toggle cheat',
   'CHEAT_SUPER_TANK' in cheat and 'CHEAT_MASK_TOGGLE | CHEAT_MASK_GLOBAL | CHEAT_MASK_MPGAME | CHEAT_MASK_SPGAME' in cheat)
ck('main Cheats menu names and exposes Super Tank',
   'case CHEAT_SUPER_TANK: return (u8 *)"Super Tank";' in front and 'CHEAT_SUPER_TANK,' in front)
ck('in-game cheat enumeration remains dynamic through CHEAT_INVALID',
   '#define MODWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in files['opt']
   and 'if (row < MODWATCH_TOGGLE_ROWS)' in files['opt'])
ck('Third Person presentation is no longer disabled merely by tank occupancy',
   'g_PlayerIsInTank != 0)\n    {\n        return FALSE;' not in bond[bond.find('s32 bondviewThirdPersonPresentationActive'):bond.find('s32 bondviewThirdPersonReticleOcclusionPassActive')])
ck('tank Third Person has dedicated farther vehicle camera',
   'tankcamera = (g_PlayerIsInTank != 0 && g_PlayerTankProp != NULL);' in bond and 'boom = 384.0f;' in bond and 'anchor.f[1] = g_CurrentPlayer->field_70 + 105.0f;' in bond)
ck('tank Third Person camera is centered laterally',
   'if (tankcamera)\n    {\n        horizontaloffset = 0.0f;' in bond)
ck('tank hand-held TP shots use exact visible muzzle origin',
   'g_PlayerIsInTank != 0' in chrprop and 'shotdata.weapon != ITEM_TANKSHELLS' in chrprop and 'gunGetThirdPersonMuzzleOrigin(hand, &shotdata.gunpos)' in chrprop)
ck('tank shell keeps its dedicated vehicle shot path', 'shotdata.weapon != ITEM_TANKSHELLS' in chrprop)
ck('Super Tank defaults to retail multiplier when disabled', 'supertankmult = 1.0f;' in bond)
ck('Super Tank raises forward/reverse top speed by 2.5x', 'TANK_MAX_SPEED * supertankmult' in bond and 'supertankmult = 2.5f;' in bond)
ck('Super Tank raises hull turn response by 2.5x', '0.3f * supertankmult' in bond)
ck('V42 audit is mandatory build prerequisite', 'tp-tank-super-tank-v42-audit' in files['mk'].split('prerequisites:',1)[1].split('\n',1)[0])
failed=[d for d,o in checks if not o]
print(f"\nTP TANK/SUPER TANK V42 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for d in failed: print(' - '+d)
    raise SystemExit(1)
