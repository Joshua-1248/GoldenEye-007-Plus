#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def text(path):
    return (ROOT / path).read_text(errors='replace')

files = {
    'const': text('src/bondconstants.h'),
    'cheat': text('src/game/cheat.c'),
    'front': text('src/game/front.c'),
    'opt': text('src/game/options.c'),
    'mp': text('src/game/mpmenu.c'),
    'mirror': text('src/game/mirroredlevels.c'),
    'mirrorh': text('src/game/mirroredlevels.h'),
    'bg': text('src/game/bg.c'),
    'stan': text('src/game/stan.c'),
    'stanh': text('src/game/stan.h'),
    'prop': text('src/game/prop.c'),
    'lv': text('src/game/lv.c'),
    'intro': text('src/game/bondview_r.c'),
    'make': text('Makefile'),
    'ldt': text('ld/game.text.ld.inc'),
    'ldd': text('ld/game.data.ld.inc'),
    'ldr': text('ld/game.rodata.ld.inc'),
    'ldb': text('ld/game.bss.ld.inc'),
    'bondview2': text('src/game/bondview2.c'),
    'propobj': text('src/game/propobj.c'),
    'mapmaker': text('src/game/mapmaker.c'),
    'boss': text('src/boss.c'),
}

checks = []
def ck(name, ok):
    checks.append((name, bool(ok)))
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

ck('Mirrored Levels cheat ID remains appended after Super Tank',
   'CHEAT_SUPER_TANK,\n    CHEAT_MIRRORED_LEVELS,' in files['const'])
ck('Mirrored Levels remains a global SP/MP toggle',
   'CHEAT_MIRRORED_LEVELS' in files['cheat'] and
   'CHEAT_MASK_TOGGLE | CHEAT_MASK_GLOBAL | CHEAT_MASK_MPGAME | CHEAT_MASK_SPGAME' in files['cheat'])
ck('main Cheats menu names Mirrored Levels',
   'case CHEAT_MIRRORED_LEVELS: return (u8 *)"Mirrored Levels";' in files['front'])
ck('SP Watch explicitly maps Mirrored Levels',
   'MODWATCH_MIRRORED_ROW' in files['opt'] and 'return CHEAT_MIRRORED_LEVELS;' in files['opt'])
ck('MP/Co-Op Watch explicitly maps Mirrored Levels',
   'MPWATCH_MIRRORED_ROW' in files['mp'] and 'return CHEAT_MIRRORED_LEVELS;' in files['mp'])
ck('toggle handlers queue mirror state',
   files['cheat'].count('mirrorLevelsSetEnabled(') >= 2)
ck('implementation is exact X sign reflection, no pivot',
   'roomdata->pos.x = -roomdata->pos.x;' in files['bg'] and
   'pad->pos.x = -pad->pos.x;' in files['mirror'] and
   'player->field_488' in files['mirror'] and
   'g_MirrorLevelsPivotX' not in files['mirror'] and
   'g_MirrorLevelsBgPivotX' not in files['bg'])
ck('BG vertices are mirrored at load/runtime',
   'vtx->v.ob[0] = -vtx->v.ob[0];' in files['bg'] and
   files['bg'].count('bgMirrorRoomVerticesRaw(roomID);') >= 1)
ck('BG triangle winding is repaired after reflection',
   'bgMirrorGdlWindingRaw' in files['bg'] and
   'bgMirrorRoomGdlRaw(roomID);' in files['bg'])
ck('portal coordinates and winding mirror together',
   'tmp[count - 1 - i]' in files['bg'] and
   '(&portal->point)[i].x = -src.x;' in files['bg'])
ck('setup pads mirror only after canonical STAN links are initialized',
   'mirrorLevelsApplySetupIfNeeded();' in files['prop'] and
   files['prop'].find('mirrorLevelsApplySetupIfNeeded();') > files['prop'].find('init_pathtable_something((struct PadRecord *) vol'))
ck('player authoritative collision position mirrors for P1-P4',
   'mirrorCollisionState(&player->field_488);' in files['mirror'] and
   'for (i = 0; i < getPlayerCount() && i < 4; i++)' in files['mirror'])
ck('runtime props/guards and persistent headings mirror',
   'prop->pos.x = -prop->pos.x;' in files['mirror'] and
   'chr->prevpos.x = -chr->prevpos.x;' in files['mirror'] and
   'vehicle->roty = -vehicle->roty;' in files['mirror'] and
   'tank->tank_orientation_angle = -tank->tank_orientation_angle;' in files['mirror'])
ck('guard model root mirrors with prop for real-time movement',
   'getsuboffset(chr->model, &modelpos);' in files['mirror'] and
   'modelpos.x = -modelpos.x;' in files['mirror'] and
   'setsuboffset(chr->model, &modelpos);' in files['mirror'])
ck('sliding door displacement direction mirrors on live toggle',
   'door->frac = -door->frac;' in files['mirror'])
ck('swinging door hinge basis and rotation preserve reflected handedness',
   'cross(Fu, Fl) is -F(u x l)' in files['propobj'] and
   'sp38.f[0] = -sp38.f[0];' in files['propobj'] and
   'if (mirrorLevelsIsEnabled())' in files['propobj'] and
   'angle = M_TAU_F - angle;' in files['propobj'])
ck('door model geometry mirrors asymmetry and cull side together',
   'matrix_column_1_scalar_multiply(-1.0f, rhs);' in files['propobj'] and
   'asymmetric details such as knobs/handles' in files['propobj'] and
   'mrData->cullmode = CULLMODE_FRONT;' in files['propobj'] and
   'mrData->cullmode = CULLMODE_BACK;' in files['propobj'])
ck('door interaction/front-side math corrects reflected handedness',
   files['propobj'].count('normal.f[0] = -normal.f[0];') >= 2 and
   files['propobj'].count('angle2 = M_TAU_F - angle2;') >= 2)
ck('live door collision hull is rebuilt after reflection',
   '#include "game/propobj.h"' in files['mirror'] and
   'doorUpdateBbox(door);' in files['mirror'])
ck('live ordinary object collision hulls rebuild after reflection',
   'else if (prop->type == PROP_TYPE_OBJ)' in files['mirror'] and
   'chrobjCollisionRelated(obj);' in files['mirror'])
ck('runtime mirror skips recycled/free prop slots before dereferencing type unions',
   'freemask[(MAX_PROPS + 31) / 32]' in files['mirror'] and
   'freeprop = g_FreeProps;' in files['mirror'] and
   'freeprop = freeprop->prev;' in files['mirror'] and
   'if (freemask[i >> 5] & (1u << (i & 31)))' in files['mirror'])
ck('intro/cutscene cameras mirror in world space',
   'camera->pos.x = -camera->pos.x;' in files['mirror'] and
   '-((struct SetupIntroCamera*)intro_record)->unk04.fval' in files['intro'])
ck('STAN bytes are never rewritten by Mirrored Levels',
   'stanMirrorLevelsToggle' not in files['stan'] and
   'tile->points[i].x =' not in files['stan'] and
   'oldpoints[' not in files['stan'])
ck('STAN collision reads mirrored X logically',
   'stanMirrorPointX' in files['stan'] and
   'stanMirrorLevelsSetEnabled' in files['stan'] and
   'stanMirrorSignedDistance' in files['stan'])
ck('STAN room bounds rebuild in mirrored coordinates',
   's32 pointx = stanMirrorPointX(&tile->points[i]);' in files['stan'])
ck('stage load sets logical STAN mirror before stanLoadFile',
   files['bg'].find('stanMirrorLevelsSetEnabled(levelid != LEVELID_TITLE') < files['bg'].find('stanLoadFile((struct StanPrefixRecord *) gptr_stan);'))
ck('title/front-end stage is never mirrored during mission exit',
   files['bg'].count('levelid != LEVELID_TITLE') >= 2 and
   's32 mirrorenabled = levelid != LEVELID_TITLE' in files['bg'])
ck('Map Maker Native Test mirrors authored X without rewriting editor data',
   'mapmakerNativeLocalXToWorld' in files['mapmaker'] and
   'mapmakerNativeWorldXToLocal' in files['mapmaker'] and
   'mapmakerNativeMirrorRotation' in files['mapmaker'] and
   'mapmakerNativeEffectiveOriginX()' in files['mapmaker'])
ck('Map Maker player start and movement use mirrored runtime coordinate mapping',
   'mirrorLevelsIsEnabled()' in files['mapmaker'] and
   'localoldx = mapmakerNativeWorldXToLocal(oldx);' in files['mapmaker'] and
   '*newx = mapmakerNativeLocalXToWorld(localx);' in files['mapmaker'] and
   'localx = mapmakerNativeWorldXToLocal(g_CurrentPlayer->field_488.collision_position.x);' in files['mapmaker'])
ck('live toggles drain in-flight graphics before applying',
   's32 mirrorLevelsHasPending(void)' in files['mirror'] and
   's32 mirrorLevelsHasPending(void);' in files['mirrorh'] and
   '(!mirrorLevelsHasPending() || pendingGfx == 0)' in files['boss'] and
   'if (pendingGfx == 0)' in files['boss'] and
   'mirrorLevelsApplyPending();' in files['boss'] and
   'mirrorLevelsApplyPending();' not in files['lv'])
ck('stage unload canonicalizes mirrored world before cleanup',
   'mirrorLevelsPrepareStageUnload();' in files['lv'] and
   files['lv'].find('mirrorLevelsPrepareStageUnload();') < files['lv'].find('cheatDisableAllCheats();') and
   'void mirrorLevelsPrepareStageUnload(void)' in files['mirror'] and
   'bgMirrorLevelsToggle(FALSE);' in files['mirror'] and
   'stanMirrorLevelsSetEnabled(FALSE);' in files['mirror'])
ck('normal left/right controls are untouched',
   'mirrorLevelsIsEnabled' not in files['bondview2'])
ck('mirror module imports declarations it uses',
   '#include "game/player.h"' in files['mirror'] and
   '#include "game/loadobjectmodel.h"' in files['mirror'] and
   '#include "game/stan.h"' in files['mirror'])
ck('mirror module is linked in all game sections',
   all('src/game/mirroredlevels.o' in files[k] for k in ('ldt','ldd','ldr','ldb')))
ck('V49 audit is mandatory build prerequisite',
   'mirrored-levels-v49-audit:' in files['make'] and
   'eeprom16-backend-v46-audit mirrored-levels-v49-audit' in files['make'])

all_game_sources = '\n'.join(path.read_text(errors='ignore') for path in (ROOT / 'src/game').glob('*.c'))
ck('no stale V48 cull-transform hook remains',
   'mirrorLevelsTransformCullMode' not in all_game_sources and
   'mirrorLevelsTransformCullMode' not in files['mirrorh'])

failed = [name for name, ok in checks if not ok]
print(f"\nMIRRORED LEVELS V49 AUDIT: {'PASS' if not failed else 'FAIL'} ({len(checks)-len(failed)}/{len(checks)})")
if failed:
    for name in failed:
        print(' -', name)
    sys.exit(1)
