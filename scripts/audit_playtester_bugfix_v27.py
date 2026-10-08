#!/usr/bin/env python3
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
checks = []

def read(rel):
    return (root / rel).read_text(errors='replace')

def check(name, cond):
    checks.append((name, bool(cond)))

explosion = read('src/game/explosion.c')
player = read('src/game/player.c')
player_h = read('src/game/player.h')
bond = read('src/game/bondview2.c')
chract = read('src/game/chraction.c')

check('Explosion guard damage selects recorded explosion owner',
      'ownerplayer = (s32)temp_s2->player' in explosion and
      'set_cur_player(ownerplayer);' in explosion and
      'chrlvExplosionDamage(temp_s0->chr' in explosion and
      'set_cur_player(savedplayer);' in explosion)

check('Co-Op tank respawn reset helper exists',
      'void playerCoopTankResetCurrentPlayerForRespawn(void)' in player and
      'playerResetCoopTankContext(player_num);' in player and
      'g_PlayerTankProp = NULL;' in player and
      'g_ExplodeTankOnDeathFlag = FALSE;' in player)
check('Tank respawn reset is declared and called before life init',
      'playerCoopTankResetCurrentPlayerForRespawn' in player_h and
      bond.find('playerCoopTankResetCurrentPlayerForRespawn();') < bond.find('init_player_BONDdata();'))

check('TP camera always performs 3D BG and object-scenery traces',
      'bondviewThirdPersonFindBackgroundHitFraction(&anchor, &desired, &frac)' in bond and
      'chrpropThirdPersonBoundsSegmentHit(cameraprop' in bond and
      'Streets and a few other stages use visually solid object scenery' in bond)
check('TP body cache checks body resource size before load',
      'bodybytes <= 0 || bodybytes > size0' in bond)
check('TP body cache checks head resource size and model/anim bounds',
      'headbytes > size0 - cursor' in bond and
      'cursor + 0xfb > size0' in bond and
      'nextcursor > size0' in bond)

# Cadence-sensitive attack/attack-roll region should retain the V17 retail helpers.
start = chract.find('void chrlvAttackrollAnimationRelated7F02E3B8(ChrRecord *self)\n{')
end = chract.find('void chrlvTickThrowGrenade', start)
region = chract[start:end] if start >= 0 and end > start else ''
check('Guard cadence region does not use direct anim-speed micro-opt setter',
      region and 'CHRLV_SET_ANIM_SPEED_IMMEDIATE' not in region)
check('Guard attack-roll cadence uses retail animation/frame helpers',
      'objecthandlerGetModelAnim(model)' in region and
      'modelGetAnimFrame(temp_a0)' in region and
      'chrlvGetGuard007SpeedRating(self, 0.5f, 0.8f)' in region)
check('Signed single-shot AutomaticFiringRate correction remains',
      '#define CHRLV_GET_AUTOMATIC_FIRING_RATE(item) (modMicroOptimizationsEnabled() ? (s8)get_ptr_item_statistics(item)->AutomaticFiringRate' in chract)

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
print(f"\nResult: {len(checks)-len(failed)}/{len(checks)} checks passed")
if failed:
    sys.exit(1)
