# Current Development Status

Current public-source refresh: **R22-derived GoldenEye 007 Plus, internal checkpoint V90 — 2026-09-28**.

Recent integrated work includes the dedicated Citadel multiplayer-stage restoration, additive Citadel portrait/resource handling, Zoinkity's final-engine-compatible Citadel STAN/clipping data, Krijy/Zoinkity attribution, and a generic Perfect Dark-inspired room-aware/cylinder-aware STAN ground-support resolver. Runtime testing confirms the latter fixes the Citadel upper-floor fall-through problem without a Citadel-specific movement branch.

The current tested build remains:

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
