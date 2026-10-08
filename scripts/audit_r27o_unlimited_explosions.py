#!/usr/bin/env python3
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]
FR = (R/'src/game/front.c').read_text(errors='replace')
O = (R/'src/game/options.c').read_text(errors='replace')
OH = (R/'src/game/options.h').read_text(errors='replace')
MP = (R/'src/game/mpmenu.c').read_text(errors='replace')
F2 = (R/'src/game/file2.c').read_text(errors='replace')
SPEC = (R/'src/game/spectrum.c').read_text(errors='replace')
EX = (R/'src/game/explosion.c').read_text(errors='replace')
EXH = (R/'src/game/explosion.h').read_text(errors='replace')
INIT = (R/'src/game/initexplosioncasing.c').read_text(errors='replace')
CLEAN = (R/'src/game/cleanexplosions.c').read_text(errors='replace')
MK = (R/'Makefile').read_text(errors='replace')

checks=[]
def ck(name, cond):
    ok=bool(cond); checks.append((name,ok)); print(('[PASS] ' if ok else '[FAIL] ')+name)

def between(src,start,end):
    a=src.find(start)
    if a<0:return ''
    b=src.find(end,a+len(start))
    return src[a:] if b<0 else src[a:b]

front_patch=between(FR,'static void frontPatchesChange','static void frontThirdPersonOptionsChange')
front_ui=between(FR,'void interface_menu_patches','Gfx *constructor_menu_patches')
front_draw=between(FR,'Gfx *constructor_menu_patches','void init_menu_third_person_options')
sp_patch=between(O,'static void modWatchTogglePatchOption','static void modWatchToggleThirdPersonOption')
mp_patch=between(MP,'static void mpwatchConfigTogglePatch','static s32 mpwatchConfigPatchValue')
mp_value=between(MP,'static s32 mpwatchConfigPatchValue','static void mpwatchConfigToggleTpOption')
create=between(EX,'explosionCreate(','void setSixExplosionAndSmokeEntries')

ck('runtime option exists and defaults Off',
   'u8 g_ModUnlimitedExplosionsEnabled = FALSE;' in O and
   'extern u8 g_ModUnlimitedExplosionsEnabled;' in OH)
ck('packed label is appended after R27D labels',
   'index == 70' in SPEC and 'Unlimited Explosions' in SPEC and
   all(x in SPEC for x in ['index == 66','index == 67','index == 68','index == 69']))
ck("main Enhancements owns Unlimited Explosions after frontend reorganization",
   'frontModGetOptionLabel(70)' in FR
   and 'g_ModUnlimitedExplosionsEnabled ^= 1;' in FR
   and 'g_ModOptionsDirty = TRUE;' in FR)
ck("SP Watch Patches has seven rows and preserves Unlimited Explosions",
   "#define MODWATCH_PATCH_ROWS 7" in O
   and "g_ModUnlimitedExplosionsEnabled ^= 1;" in sp_patch
   and "g_ModWatchSettingsDirty = TRUE;" in sp_patch
   and "frontModGetOptionLabel(70)" in O
   and "frontModGetOptionLabel(80)" in O
   and "frontModGetOptionLabel(81)" in O)
ck("MP/Co-op Watch Patches has seven rows and preserves Unlimited Explosions",
   "if (mode == 5) return 7;" in MP
   and "g_ModUnlimitedExplosionsEnabled ^= 1;" in mp_patch
   and "mpcfg_globals_dirty = TRUE;" in mp_patch
   and "mpwatchConfigStoreGlobals();" not in mp_patch
   and "g_ModUnlimitedExplosionsEnabled != 0" in mp_value
   and "row == 4 ? 70 : row == 5 ? 80 : 81" in MP)
ck('option uses independent extension reserved bit 0x08',
   '#define GE_SRAM_EXT_RESERVED_UNLIMITED_EXPLOSIONS 0x08' in F2)
ck('old/no-extension folders default Unlimited Explosions Off',
   'g_ModUnlimitedExplosionsEnabled = FALSE;' in F2)
ck('extension load/store round-trips Unlimited Explosions',
   'record->reserved & GE_SRAM_EXT_RESERVED_UNLIMITED_EXPLOSIONS' in F2 and
   'g_ModUnlimitedExplosionsEnabled ? GE_SRAM_EXT_RESERVED_UNLIMITED_EXPLOSIONS' in F2)
ck('legacy mirror sync preserves complete reserved byte',
   'record.reserved = current->reserved;' in F2 or
   'record.reserved = g_GeSramExtBank.folders[folder].reserved;' in F2)
ck('R27H quick preset keeps new patch Off',
   'frontModApplyQuickSettingsPreset' not in FR or 'g_ModUnlimitedExplosionsEnabled = FALSE;' in FR)
ck('retail/non-modded capacity remains six',
   '#define EXPLOSION_BUFFER_LEN_RETAIL 6' in EXH and
   '#define EXPLOSION_BUFFER_LEN EXPLOSION_BUFFER_LEN_RETAIL' in EXH)
ck('Plus storage pool is 64 slots',
   '#define EXPLOSION_BUFFER_LEN 64' in EXH and
   'EXPLOSION_BUFFER_LEN * sizeof(struct Explosion)' in INIT)
ck('creation limit is six Off / full pool On',
   'g_ModUnlimitedExplosionsEnabled' in create and
   '? EXPLOSION_BUFFER_LEN' in create and
   ': EXPLOSION_BUFFER_LEN_RETAIL;' in create and
   'var_v0 < explosion_limit' in create)
ck('screen-shake countdown remains retail six, not pool capacity',
   'g_NumExplosionEntries = 6;' in EX and 'g_NumSmokeEntries = 6;' in EX and
   'g_NumExplosionEntries = EXPLOSION_BUFFER_LEN' not in EX)
ck('lifecycle and cleanup scan full allocated pool',
   'for (i = 0; i < EXPLOSION_BUFFER_LEN; i++)' in EX and
   'i<EXPLOSION_BUFFER_LEN' in CLEAN.replace(' ',''))
ck('R27E deferred persistence remains intact',
   'frontModCommitDeferredSubmenuSettings' in FR and
   'static void modWatchCommitDeferredSettings(void)' in O and
   'static u8 mpcfg_globals_dirty;' in MP)

prereq=next((x for x in MK.splitlines() if x.startswith('prerequisites:')),'')
ck('R27O audit is mandatory prerequisite',
   'r27o-unlimited-explosions-audit:' in MK and 'r27o-unlimited-explosions-audit' in prereq)

bad=[n for n,ok in checks if not ok]
print()
if bad:
    print(f"R27O UNLIMITED EXPLOSIONS AUDIT: FAIL ({len(checks)-len(bad)}/{len(checks)})")
    for n in bad: print(' - '+n)
    sys.exit(1)
print(f"R27O UNLIMITED EXPLOSIONS AUDIT: PASS ({len(checks)}/{len(checks)})")
