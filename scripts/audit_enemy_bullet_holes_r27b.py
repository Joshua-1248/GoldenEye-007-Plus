#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def read(path):
    return (ROOT / path).read_text(errors='replace')

OPT = read('src/game/options.c')
OPTH = read('src/game/options.h')
ROOTOPTH = read('options.h')
FILE2 = read('src/game/file2.c')
FRONT = read('src/game/front.c')
MP = read('src/game/mpmenu.c')
CHR = read('src/game/chraction.c')
SPEC = read('src/game/spectrum.c')

fails = 0

def check(name, cond):
    global fails
    print(('[PASS] ' if cond else '[FAIL] ') + name)
    if not cond:
        fails += 1

check('Enemy Bullet Holes runtime default is Off',
      'u8 g_ModEnemyBulletHolesEnabled = FALSE;' in OPT)
check('Enemy Bullet Holes global is exported through both option headers',
      'extern u8 g_ModEnemyBulletHolesEnabled;' in OPTH and
      'extern u8 g_ModEnemyBulletHolesEnabled;' in ROOTOPTH)
check('Extended save record uses a previously free flag bit',
      '#define GE_SRAM_EXT_FLAG_ENEMY_BULLET_HOLES 0x20' in FILE2)
check('Extended save load restores the option and missing records reset it Off',
      'g_ModEnemyBulletHolesEnabled =\n        (record->flags & GE_SRAM_EXT_FLAG_ENEMY_BULLET_HOLES) != 0;' in FILE2 and
      'g_ModEnemyBulletHolesEnabled = FALSE;' in FILE2)
check('Extended save store and legacy mirror sync preserve the option',
      'if (g_ModEnemyBulletHolesEnabled)\n        record.flags |= GE_SRAM_EXT_FLAG_ENEMY_BULLET_HOLES;' in FILE2 and
      FILE2.count('GE_SRAM_EXT_FLAG_ENEMY_BULLET_HOLES') >= 5)
check('Main-menu Special Options exposes Enemy Bullet Holes', 'page2labels[9]' in FRONT and '29,52' in FRONT and 'g_ModEnemyBulletHolesEnabled ^= 1;' in FRONT)
check('SP Watch base Special Options exposes Enemy Bullet Holes before submenu links', '#define MODWATCH_OPTION_ROWS 14' in OPT and 'frontModGetOptionLabel(52)' in OPT and 'g_ModEnemyBulletHolesEnabled ^= 1;' in OPT and 'frontModGetOptionLabel(64)' in OPT)
check('MP Watch exposes and persists the new global toggle', 'if (mode == 2) return 15;' in MP and 'oldrows[11] = {0,1,2,3,4,6,7,8,9,11,17}' in MP and 'g_ModEnemyBulletHolesEnabled ^= 1;' in MP and 'mpwatchConfigStoreGlobals();' in MP)
check('Shared label table contains Enemy Bullet Holes',
      'index == 52' in SPEC and '0x456E656D' in SPEC and '0x65730000' in SPEC)
check('Enemy decal work is fully gated behind the option',
      CHR.count('g_ModEnemyBulletHolesEnabled') >= 2 and
      'chrpropFindNearestBgHitOnSegment(' in CHR and
      'enemybghitvalid' in CHR)
check('Enemy BG hit uses Bond-style all-room nearest surface resolver',
      CHR.find('chrlvStanLineDirIntersection(&sp240, &sp220, &sp258);') <
          CHR.find('enemybghitvalid = chrpropFindNearestBgHitOnSegment(') and
      'enemybghitvalid = chrpropFindNearestBgHitOnSegment(' in CHR)
check('Enemy surface hits use Bond wall-hit material/orientation path',
      'g_HitTypeSounds[' in CHR and
      'g_Textures[enemybghit.texturenum]' in CHR and
      'explosionCreateBulletImpact(' in CHR and
      '&enemybghit.hitpos' in CHR and '&enemybghit.normal' in CHR and
      'enemybgroom,' in CHR)
check('Enemy holes are not incorrectly tied to retail spark-distance suppression',
      CHR.find('if (g_ModEnemyBulletHolesEnabled') <
          CHR.find('if (sp22C != 0)') and
      'sp20C < 10000.0f' in CHR)
check('Enemy BG response includes Bond-equivalent material chips and exact spark',
      'explosionCreate(0, &enemybghit.hitpos, sp254,' in CHR and
      'material != 5 && material != 6' in CHR and
      'bullet_spark_create(&sparkpos, 1, 26.0f,' in CHR)
check('Enemy object/door response uses exact visual-only model hit path',
      'chrpropProbeObjectHitOnSegment(enemyhitprop' in CHR and
      'objCreateBulletImpactVisual(&visualshot, &visualhit);' in CHR and
      'enemyobjvisual = TRUE;' in CHR)
check('Resolved character/prop hits cannot stamp the background behind them',
      'sp230 == 0' in CHR and 'stanSavedColl_posData == NULL' in CHR)

if fails:
    print(f'\nR27B audit: {fails} failure(s)')
    sys.exit(1)
print('\nR27B audit: all checks passed')
