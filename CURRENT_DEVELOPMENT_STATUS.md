# GoldenEye 007 Plus — Current Development Status

**Snapshot:** 2026-09-27  
**Internal checkpoint:** V83  
**Public baseline:** R22-derived development tree

## Recent confirmed fixes / changes

- Surface II no longer crashes in single-player Third Person.
- Surface II no longer crashes in 2P Co-Op with both players in Third Person.
- Surface II no longer crashes in 3P/4P Co-Op.
- Third Person/Co-Op end-cutscene body ownership was reworked so chained POSEND camera cuts preserve the canonical cinematic Bond body.
- Dam Third Person end-cutscene presentation was corrected without changing the authored Dam camera script.
- TP Sight Translucency is independently save-backed and no longer collides with TP camera option bits.
- TP camera defaults/tuning include a 300 default distance, 600 maximum distance and 500 default crosshair range.
- Extended save migration/legacy-sync bugs affecting TP camera tuner values were repaired.
- Co-Op objective inventory checks now use team authority for relevant Collect/Deposit criteria so completion is not accidentally current-player-local.
- The in-game All Objectives Complete action refreshes shared Co-Op objective state/HUD immediately.

## Current development / runtime-test areas

- Mirrored Levels live-toggle safety has received scheduler, stage-unload and free-prop fixes; large explored stages such as Caverns remain important stress cases.
- Third Person Moonraker Laser penetration uses a dedicated stable muzzle ray so penetrated surfaces should not visually deflect the outgoing beam; continue testing glass/doors/crates.
- Map Maker remains developmental and includes ongoing Basic/Advanced editing, dedicated native-stage and rendering/storage work.

## Standard build

```sh
make VERSION=US \
    PHYSICAL_CODE=YES \
    PHYSICAL_FASTPATHS=YES \
    MODDED_CHEATS=YES \
    MAP_MAKER=YES \
    OPT_USE_LLVM=YES \
    COMPARE=0 \
    -j2
```
