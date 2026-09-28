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
 'chr':(R/'src/game/chraction.c').read_text(errors='replace'),
 'mk':(R/'Makefile').read_text(errors='replace'),
}
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
C=F['const']; Q=F['cheat']; A=F['front']; S=F['sp']; M=F['mp']; H=F['chr']
ck('existing mod cheat IDs stay stable and Ultra is appended',
   C.find('CHEAT_KINETIC_EXPLOSIONS') < C.find('CHEAT_SUPER_TANK') < C.find('CHEAT_MIRRORED_LEVELS') < C.find('CHEAT_ULTRA_KINETICS') < C.find('CHEAT_INVALID'))
ck('legacy Kinetic public name is now Super Kinetics',
   'case CHEAT_KINETIC_EXPLOSIONS: return (u8 *)"Super Kinetics";' in A and '"Kinetic Explosions"' not in A)
ck('Ultra Kinetics has public main-menu label',
   'case CHEAT_ULTRA_KINETICS: return (u8 *)"Ultra Kinetics";' in A)
ck('Ultra Kinetics is a global SP/MP toggle cheat',
   'CHEAT_ULTRA_KINETICS' in Q and 'CHEAT_MASK_TOGGLE | CHEAT_MASK_GLOBAL | CHEAT_MASK_MPGAME | CHEAT_MASK_SPGAME' in Q)
ck('main Cheat Options page exposes both kinetics together',
   'CHEAT_KINETIC_EXPLOSIONS,\n        CHEAT_ULTRA_KINETICS,\n        CHEAT_SUPER_TANK,' in A)
ck('SP Watch includes Ultra next to Super Kinetics',
   '#define MODWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in S and
   'if (row == CHEAT_KINETIC_EXPLOSIONS)\n        return CHEAT_ULTRA_KINETICS;' in S and
   'if (row < MODWATCH_TOGGLE_ROWS)\n        return (CHEAT_ID)row;' in S)
ck('MP/Co-Op Watch includes Ultra next to Super Kinetics',
   '#define MPWATCH_TOGGLE_ROWS (CHEAT_INVALID - 1)' in M and
   'if (row == CHEAT_KINETIC_EXPLOSIONS)\n        return CHEAT_ULTRA_KINETICS;' in M and
   'if (row < MPWATCH_TOGGLE_ROWS)\n        return (CHEAT_ID)row;' in M)
ck('Super Kinetics alone is exact x1.5',
   'if (cheatIsActive(CHEAT_KINETIC_EXPLOSIONS))\n            kineticmult = 1.5f;' in H)
ck('Ultra Kinetics alone is exact x3.5 and stacked pair is x5.0',
   'if (cheatIsActive(CHEAT_ULTRA_KINETICS))\n            kineticmult = kineticmult > 1.0f ? 5.0f : 3.5f;' in H)
ck('kinetic multiplier affects only death impulse, not explosion damage/range',
   'norm *= kineticmult;' in H and 'g_ExplosionTypes' not in H[H.find('s32 chrlvExplosionDamage'):H.find('if (atan < subroty)',H.find('s32 chrlvExplosionDamage'))])
ck('V41 upward launch helper scales with selected kinetic strength',
   'self->fallspeed.f[1] += (2.6666667f * kineticmult) * damage;' in H)
ck('both cheat activation/deactivation handlers include Ultra', Q.count('case CHEAT_ULTRA_KINETICS:') >= 2)
ck('V85 audit is mandatory build prerequisite',
   'super-ultra-kinetics-v85-audit:' in F['mk'] and
   'super-ultra-kinetics-v85-audit' in F['mk'].split('prerequisites:',1)[1].split('\n',1)[0])
failed=[n for n,c in checks if not c]
print(f"\nSUPER/ULTRA KINETICS V85 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for n in failed: print(' - '+n)
    sys.exit(1)
