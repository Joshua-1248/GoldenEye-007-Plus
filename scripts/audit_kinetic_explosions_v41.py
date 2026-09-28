#!/usr/bin/env python3
from pathlib import Path
R=Path(__file__).resolve().parents[1]
files={
 'const':(R/'src/bondconstants.h').read_text(),
 'cheat':(R/'src/game/cheat.c').read_text(),
 'front':(R/'src/game/front.c').read_text(),
 'opt':(R/'src/game/options.c').read_text(),
 'chr':(R/'src/game/chraction.c').read_text(),
 'spec':(R/'src/game/spectrum.c').read_text(),
 'oh':(R/'src/game/options.h').read_text(),
 'roh':(R/'options.h').read_text(),
 'mk':(R/'Makefile').read_text(),
}
checks=[]
def ck(desc, ok):
    checks.append((desc,bool(ok)))
    print(('[PASS] ' if ok else '[FAIL] ')+desc)
ck('TP Crouched Cam Height default is 46 in both headers', '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in files['oh'] and '#define TP_CROUCH_CAM_HEIGHT_DEFAULT 46' in files['roh'])
ck('Complete Objectives replaces old in-game action label', '/* Complete Objectives */' in files['spec'] and 'All Objectives Complete' not in files['spec'])
ck('legacy Super Kinetics ID remains stable/appended', 'CHEAT_KINETIC_EXPLOSIONS' in files['const'] and files['const'].find('CHEAT_KINETIC_EXPLOSIONS') < files['const'].find('CHEAT_INVALID'))
ck('Super Kinetics remains a global SP/MP toggle cheat', 'CHEAT_KINETIC_EXPLOSIONS' in files['cheat'] and 'CHEAT_MASK_TOGGLE | CHEAT_MASK_GLOBAL | CHEAT_MASK_MPGAME | CHEAT_MASK_SPGAME' in files['cheat'])
ck('main Cheat menu exposes renamed Super Kinetics', 'case CHEAT_KINETIC_EXPLOSIONS: return (u8 *)"Super Kinetics";' in files['front'] and 'CHEAT_KINETIC_EXPLOSIONS,' in files['front'])
ck('in-game cheat enumeration still includes legacy kinetic ID',
   '#define MODWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in files['opt']
   and 'if (row < CHEAT_KINETIC_EXPLOSIONS)' in files['opt']
   and 'return (CHEAT_ID)(row + 1);' in files['opt'])
ck('Super Kinetics still uses x1.5 at canonical fallspeed path', 'cheatIsActive(CHEAT_KINETIC_EXPLOSIONS)' in files['chr'] and 'kineticmult = 1.5f;' in files['chr'] and 'norm *= kineticmult;' in files['chr'])
ck('kinetic stack retains proportional explicit upward lift', 'self->fallspeed.f[1] += (2.6666667f * kineticmult) * damage;' in files['chr'])
damage_fn = files['chr'][files['chr'].find('s32 chrlvExplosionDamage'):files['chr'].find('if (atan < subroty)', files['chr'].find('s32 chrlvExplosionDamage'))]
ck('normal explosion damage/range tables are not modified', 'g_ExplosionTypes' not in damage_fn)
ck('V41 audit is mandatory build prerequisite', 'kinetic-explosions-v41-audit' in files['mk'].split('prerequisites:',1)[1].split('\n',1)[0])
failed=[d for d,o in checks if not o]
print(f"\nKINETIC EXPLOSIONS V41 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for d in failed: print(' - '+d)
    raise SystemExit(1)
