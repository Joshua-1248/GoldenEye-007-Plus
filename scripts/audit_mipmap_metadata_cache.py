#!/usr/bin/env python3
"""Guard GoldenEye Plus' persistent weapon-prewarm mipmap metadata isolation."""
from pathlib import Path
import re, sys

root = Path(__file__).resolve().parents[1]
image_h = (root / 'src/game/image.h').read_text(errors='ignore')
image_c = (root / 'src/game/image.c').read_text(errors='ignore')
tex_c = (root / 'src/game/tex.c').read_text(errors='ignore')
gun_c = (root / 'src/game/gun.c').read_text(errors='ignore')
makefile = (root / 'Makefile').read_text(errors='ignore')
prewarm_s = (root / 'src/game/gun_prewarm_ids.s').read_text(errors='ignore')

m = re.search(r'#define\s+PERSISTENT_TEX_LOD_CACHE_CAPACITY\s+(\d+)', image_h)
if not m:
    print('FAIL: PERSISTENT_TEX_LOD_CACHE_CAPACITY is missing')
    sys.exit(1)
capacity = int(m.group(1))

prewarm = []
for line in prewarm_s.splitlines():
    if '.half' in line:
        prewarm.extend(int(x) for x in re.findall(r'\d+', line.split('.half', 1)[1]))
prewarm_unique = set(prewarm)

explicit = set()
for path in (root / 'assets').rglob('*.c'):
    text = path.read_text(errors='ignore')
    for line in text.splitlines():
        if ('TEXTURETYPE_MIPMAP' in line or 'TEXTURETYPE_LOD' in line or
                'TEXTURETYPE_DETAIL' in line):
            explicit.update(int(x) for x in re.findall(r'IMAGE_(\d+)', line))

persistent_explicit = prewarm_unique & explicit
checks = [
    ('prewarm list remains 605 references', len(prewarm) == 605),
    ('persistent LOD capacity covers every explicit-LOD prewarm texture', capacity >= len(persistent_explicit)),
    ('persistent metadata table exists', 'g_PersistentTexCacheItems[PERSISTENT_TEX_LOD_CACHE_CAPACITY]' in image_c),
    ('persistent metadata count exists', 'g_PersistentTexCacheCount' in image_c),
    ('prewarm resets persistent metadata', 'texResetPersistentLodCache();' in gun_c),
    ('prewarm enables persistent metadata mode', 'texSetPersistentLodCacheMode(TRUE);' in gun_c),
    ('prewarm disables persistent metadata mode', gun_c.count('texSetPersistentLodCacheMode(FALSE);') >= 2),
    ('width lookup checks persistent metadata', 'g_PersistentTexCacheItems[i].widths[lod - 1]' in tex_c),
    ('height lookup checks persistent metadata', 'g_PersistentTexCacheItems[i].heights[lod - 1]' in tex_c),
    ('retail stage cache remains 150 entries', 'g_TexCacheItems[150]' in image_c),
    ('persistent table does not wrap to zero', 'lodcache == g_TexCacheItems && *lodcachecount >= lodcachecapacity' in image_c),
    ('audit is mandatory build prerequisite', 'mipmap-metadata-audit' in makefile and 'prerequisites: autoshot-resource-audit coop-mission-item-audit playtester-bugfix-audit mipmap-metadata-audit' in makefile),
]

failed = 0
for name, ok in checks:
    print(f"[{'OK' if ok else 'FAIL'}] {name}")
    if not ok:
        failed += 1

print(f'prewarm: {len(prewarm)} refs / {len(prewarm_unique)} unique / {len(persistent_explicit)} explicit-LOD unique')
print(f'persistent metadata capacity: {capacity}')
if failed:
    print(f'MIPMAP METADATA AUDIT: FAIL ({failed}/{len(checks)} failed)')
    sys.exit(1)
print(f'MIPMAP METADATA AUDIT: PASS ({len(checks)}/{len(checks)})')
