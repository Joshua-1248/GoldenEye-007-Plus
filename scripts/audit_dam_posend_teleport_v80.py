#!/usr/bin/env python3
from pathlib import Path
import sys
R=Path(__file__).resolve().parents[1]
C=(R/'src/game/chrai.c').read_text(errors='replace')
MK=(R/'Makefile').read_text(errors='replace')
checks=[]
def ck(n,c): checks.append((n,bool(c))); print(('[PASS] ' if c else '[FAIL] ')+n)
ck('cinematic player teleport sync is POSEND-only', 'if (g_CameraMode == CAMERAMODE_POSEND)' in C)
ck('teleport delta comes from old player collision position', 'teleportDelta.x = pos.x - g_CurrentPlayer->field_488.collision_position.x;' in C and 'teleportDelta.y = pos.y - g_CurrentPlayer->field_488.collision_position.y;' in C and 'teleportDelta.z = pos.z - g_CurrentPlayer->field_488.collision_position.z;' in C)
ck('camera/look-at cache follows cinematic teleport', 'g_CurrentPlayer->field_3C4 += teleportDelta.x;' in C and 'g_CurrentPlayer->field_3C8 += teleportDelta.y;' in C and 'g_CurrentPlayer->field_3CC += teleportDelta.z;' in C)
ck('body position caches follow cinematic teleport', 'g_CurrentPlayer->field_488.pos.x += teleportDelta.x;' in C and 'g_CurrentPlayer->field_488.pos3.x += teleportDelta.x;' in C)
ck('portal tile follows cinematic teleport', 'g_CurrentPlayer->field_488.current_tile_ptr_for_portals = stan;' in C)
ck('look-at smoother is reseeded after discontinuity', 'g_CurrentPlayer->field_3B8.x = g_CurrentPlayer->field_3C4 / 0.100000024f;' in C)
ck('previous player position follows cinematic teleport', 'g_CurrentPlayer->bondprevpos.x += teleportDelta.x;' in C)
ck('V80 audit is mandatory', 'dam-posend-teleport-v80-audit:' in MK and 'dam-posend-teleport-v80-audit' in next((x for x in MK.splitlines() if x.startswith('prerequisites:')),''))
bad=[n for n,c in checks if not c]
print()
print(f"DAM POSEND TELEPORT V80 AUDIT: {'PASS' if not bad else 'FAIL'} ({len(checks)-len(bad)}/{len(checks)})")
if bad:
    for n in bad: print(' - '+n)
    sys.exit(1)
