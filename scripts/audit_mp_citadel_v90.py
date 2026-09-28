#!/usr/bin/env python3
from pathlib import Path
import hashlib
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
checks = []

def check(desc, ok):
    checks.append((desc, bool(ok)))
    print(f"[{'PASS' if ok else 'FAIL'}] {desc}")

def text(rel):
    return (ROOT / rel).read_text(errors='replace')

def sha(rel):
    return hashlib.sha256((ROOT / rel).read_bytes()).hexdigest()

stan = text('assets/obseg/stan/Tbg_cat_all_p_stanZ.c')
stan_engine = text('src/game/stan.c')
bg = text('src/game/bg.c')
prop = text('src/game/prop.c')
front = text('src/game/front.c')
const = text('src/bondconstants.h')
oddh = text('assets/oddtextures.h')
oddc = text('assets/oddtextures.c')
images = text('assets/images.def')
filelist = text('assets/obseg/Makefile.filelist')
obseg = text('assets/obseg/ob_seg.s')
resids = text('assets/obseg/file_resource_id_enums.h')
restable = text('assets/obseg/file_resource_table.inc.c')
obsegh = text('assets/obseg/obseg.h')
makefile = text('Makefile')
language = text('src/game/language.c')
image_sync = text('scripts/make/sync_imagelist_with_def.py')
doc = text('docs/CITADEL_ZOINKITY.md')
readme = text('readme.md')

# Exact reconstructed assets.
check('Zoinkity Citadel MP setup payload is exact reconstructed binary',
      sha('assets/obseg/setup/Ump_setupcatZ.bin') == '2d2dbaed1337a55cdeefe78f550a4634d1b8da1888de888c2e761efa68bb1016')
check('Citadel runtime portrait is the V90 R5 vertically-corrected 68x44 I8 Zoinkity portrait',
      sha('assets/images/split/MP_CITADEL.bin') == 'c14f6860e1f5197b0f9e408091310ef864fd57f22d9cca21e5feca4923c9ba65')
check('retail final image 2697 is preserved byte-for-byte',
      sha('assets/images/split/2697.bin') == '06f82f9b6c78217705d40272a143836a9bf4b22c4944a53f663896152d77ce43')

# Setup header sanity: every nonzero top-level file offset remains in-bounds.
setup = (ROOT / 'assets/obseg/setup/Ump_setupcatZ.bin').read_bytes()
ptrs = struct.unpack('>10I', setup[:40])
check('Citadel setup is a self-contained valid-offset setup resource',
      len(setup) == 0x1a98 and all(p == 0 or p < len(setup) for p in ptrs))
check('Citadel setup retains Zoinkity provenance marker',
      b'GE Reclipping Project' in setup and b'c/o Zoinkity' in setup)

# Corrected STAN source: 377 real tiles + terminator and high-sided polygons.
real_tiles = len(re.findall(r'^StandTile tile_\d+ = \{', stan, re.M)) - 1
point_counts = [int(x) for x in re.findall(r'\n\s+(\d+),\n\s+0x[0-9a-fA-F]+, 0x[0-9a-fA-F]+, 0x[0-9a-fA-F]+,\n\s+\{', stan)]
check('Citadel uses Zoinkity hand-reclipped normal STAN source',
      'Original hand-reclipped collision data by Zoinkity.' in stan and real_tiles == 377)
check('Citadel reclip includes and preserves 12/13-point polygons',
      point_counts and max(point_counts) == 13 and 12 in point_counts)
check('generic STAN size table supports all 0-15 point-count encodings',
      '0x48,0x50,0x58,0x60' in stan_engine and '0x68,0x70,0x78,0x80' in stan_engine)

# Zoinkity scale.
check('Citadel level table uses Zoinkity corrected scale',
      re.search(r'LEVELID_CITADEL.*0\.57639205', bg) is not None)

# Setup pipeline registration is appended so existing MP setup names stay stable.
check('Citadel MP setup participates in normal resource build',
      re.search(r'mp_setupstatue \\\n\s*mp_setupcat\b', filelist) is not None and
      'obseg_file_Z setup, Ump_setupcatZ' in obseg)

check('appended mod resources are physically emitted in appended table order',
      re.search(r'bg_file_seg bg_mapmaker_all_p_seg, bg_mapmaker_all_p\nobseg_file_Z stan, Tbg_mapmaker_all_p_stanZ\nobseg_file_Z setup, UsetupmapmakerZ\n\.endif\nobseg_file_Z setup, Ump_setupcatZ\n\n\.global ob__ob_end_seg', obseg) is not None and
      obseg.count('Ump_setupcatZ') == 1)

check('Citadel setup has an appended resource ID without moving prior IDs',
      re.search(r'SETUPMAPMAKER,\s*#endif\s*/\* V90: appended after all existing resources so no prior ID moves\. \*/\s*MP_SETUPCAT,\s*OBENDSEG', resids, re.S) is not None)
check('Citadel setup is registered in runtime resource lookup',
      '{MP_SETUPCAT, "Ump_setupcatZ", &Ump_setupcatZ}' in restable and
      'extern u8 Ump_setupcatZ[];' in obsegh)
check('Citadel has a real language-bank mapping instead of the unknown-stage hang path',
      re.search(r'case\s+LEVELID_CITADEL:\s*/\*.*?\*/\s*return_id\s*=\s*LCAT;', language, re.S) is not None)

# Runtime intro/pad handoff.
block = prop[prop.find('g_CitadelMpPadTemplates'):prop.find('static PadRecord g_CitadelMpPads')]
template_rows = re.findall(r'^\s*\{ \{', block, re.M)
check('Citadel runtime handoff preserves all 48 Zoinkity pad slots', len(template_rows) == 48)
check('Citadel runtime handoff is regular-multiplayer-only',
      'stageId != LEVELID_CITADEL' in prop and
      'gamemode != GAMEMODE_MULTI' in prop and
      'get_scenario() == SCENARIO_COOP' in prop)
check('Citadel runtime pads resolve symbolic STAN names rather than hard-coded RDRAM pointers',
      'stanMatchTileName(g_CitadelMpPadTemplates[i].stanName)' in prop and
      '0x8008E360' not in prop and '0x80091AB0' not in prop and '0x80091B50' not in prop)
check('Citadel runtime handoff installs Zoinkity intro and pads before mirrored setup handling',
      prop.find('citadelApplyMultiplayerSetupOverrides(stageId);') < prop.find('mirrorLevelsApplySetupIfNeeded();'))

# Intro exact 40-word shape and requested spawn pads.
intro_match = re.search(r'static s32 g_CitadelMpIntro\[\] = \{(.*?)\n\};', prop, re.S)
intro_words = re.findall(r'0x[0-9A-Fa-f]{8}', intro_match.group(1) if intro_match else '')
check('Citadel intro preserves exact 40-word Zoinkity runtime sequence',
      len(intro_words) == 40 and intro_words[:8] == [
          '0x00000000','0x0000000B','0x00000000','0x00000000',
          '0x0000000C','0x00000000','0x00000000','0x0000000D'])

# Every named runtime pad target must exist in the corrected STAN.
def decode_name(id24):
    hi = (id24 >> 8) & 0xffff
    low = id24 & 0xff
    typ = 'P' if hi & 0x8000 else 'p'
    num = hi & 0x7fff
    letter = chr(ord('a') + ((low >> 3) & 0x1f))
    digit = low & 7
    return f'{typ}{num}{letter}{digit if digit else ""}'
stan_names = {decode_name(int(x, 16)) for x in re.findall(r'^\s+0x([0-9A-Fa-f]{6}), 0x[0-9A-Fa-f]{2},', stan, re.M)}
pad_names = {x for x in re.findall(r'\}, "([pP][^"]+)" \},', block)}
check('every Citadel MP runtime pad STAN name exists in corrected reclip', pad_names and pad_names <= stan_names)

# Frontend and portrait.
check('Citadel has a dedicated MP stage enum without shifting prior Plus stages',
      re.search(r'MP_STAGE_STATUE,\s*MP_STAGE_CRADLE,\s*MP_STAGE_CITADEL,', const, re.S) is not None)
check('Citadel is selectable in MP with its own title, portrait and level ID',
      'TITLE_STR_174_CITADEL' in front and 'IMG_MP_CITADEL, LEVELID_CITADEL, -1, 1, 4' in front)
check('Citadel portrait is appended to the MP portrait enum/table',
      re.search(r'IMG_MP_RANDOM,\s*IMG_MP_CITADEL', oddh, re.S) is not None and
      'IMAGE_MP_CITADEL' in oddc)
check('Citadel portrait uses the standard MP 68x44 I8 render path',
      '{IMAGE_MP_CITADEL, 0x44, 0x2C, 0, G_IM_FMT_I, G_IM_SIZ_8b' in oddc)
check('MP level-select renderer uses each portrait native texture dimensions',
      front.count('simage->width, simage->height, 0, 0, 1') >= 3)
image_rows = [line for line in images.splitlines() if line.strip().startswith('IMAGE(')]
check('Citadel has a dedicated appended image ID without replacing retail image 2697',
      len(image_rows) == 2699 and
      image_rows[-2].strip() == 'IMAGE(2697, 0x53D, HIT_DEFAULT, HIT_DEFAULT, 0, 0, 0, 0)' and
      image_rows[-1].strip() == 'IMAGE(MP_CITADEL, 0xBB4, HIT_DEFAULT, HIT_DEFAULT, 0, 0, 0, 0)')
check('image build treats image2698.bin as the retail padding sentinel, not a retail texture',
      "tail_name == 'image2698.bin'" in image_sync and
      'return old_entries[:-1], tail[\'offset\']' in image_sync and
      'real retail images' in image_sync)
check('incremental image-list generation tracks the sync script itself',
      '$(BUILD_DIR)/imagelist.csv: imagelist.u.csv assets/images.def scripts/make/sync_imagelist_with_def.py' in makefile)
check('incremental builds rebuild g_Textures when images.def changes',
      '$(BUILD_DIR)/src/game/image.o: assets/images.def' in makefile)

check('Citadel restoration provenance document exists',
      'Original Citadel reclip/restoration: **Zoinkity**.' in doc)
check('project README visibly credits Krijy and Zoinkity for Citadel',
      '**Krijy**' in readme and '**Zoinkity**' in readme and
      'docs/CITADEL_ZOINKITY.md' in readme)
check('V89 and V90 audits are mandatory prerequisites',
      'level-modifiers-v89-audit' in makefile and 'mp-citadel-v90-audit' in makefile and
      re.search(r'^prerequisites:.*mp-citadel-v90-audit', makefile, re.M) is not None)

passed = sum(ok for _, ok in checks)
print(f"\nMP CITADEL V90 AUDIT: {'PASS' if passed == len(checks) else 'FAIL'} ({passed}/{len(checks)})")
sys.exit(0 if passed == len(checks) else 1)
