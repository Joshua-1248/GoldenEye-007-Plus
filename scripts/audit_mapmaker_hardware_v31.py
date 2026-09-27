#!/usr/bin/env python3
from pathlib import Path
import sys
src = Path('src/game/mapmaker.c').read_text()
mk = Path('Makefile').read_text()
checks = [
 ('native draw radius', '#define MM_NATIVE_DRAW_RADIUS 4200.0f' in src),
 ('dynamic vertex reserve', '#define MM_RENDER_VTX_RESERVE 4096' in src),
 ('native render distance cull', 'dist2 > MM_NATIVE_DRAW_RADIUS * MM_NATIVE_DRAW_RADIUS' in src),
 ('native render behind-camera cull', 'forward < -400.0f' in src),
 ('native render hard buffer guard', 'dynGetFreeVtx() < MM_RENDER_VTX_RESERVE || dynGetFreeGfx(gdl) < MM_RENDER_GFX_RESERVE' in src),
 ('collision coarse-cell rejection', src.count('MM_COLLISION_COARSE_RADIUS') >= 5),
 ('advanced loop local buffer guard', 'loop->vertex_count * sizeof(Vtx) + 2048' in src),
 ('editor marker distance cull', 'MM_MARKER_DRAW_RADIUS * MM_MARKER_DRAW_RADIUS' in src),
 ('texture metadata reuse cache', 'textureid == g_MapMakerLastTextureId' in src),
 ('texture cache invalidated with private pool', 'g_MapMakerLastTextureId = MAPMAKER_TEXTURE_NONE;' in src),
 ('adaptive local grid radii', all(x in src for x in (
     '#define MM_GRID_NEAR_RADIUS  12',
     '#define MM_GRID_MID_RADIUS   32',
     '#define MM_GRID_FAR_RADIUS   64'))),
 ('adaptive grid line thinning', '(line & 7) != 0' in src and '(line & 3) != 0' in src),
 ('advanced overlay handle cap', 'shown < 12' in src),
 ('mandatory build prerequisite', 'mapmaker-hardware-v31-audit' in mk and 'prerequisites:' in mk),
]
failed=[]
for name,ok in checks:
    print(('[PASS] ' if ok else '[FAIL] ') + name)
    if not ok: failed.append(name)
if failed:
    print(f'MAP MAKER HARDWARE V31 AUDIT: FAIL ({len(checks)-len(failed)}/{len(checks)})')
    sys.exit(1)
print(f'MAP MAKER HARDWARE V31 AUDIT: PASS ({len(checks)}/{len(checks)})')
