# GoldenEye 007 Plus

GoldenEye 007 Plus is a community modification of **GoldenEye 007 for Nintendo 64**, built on the fully decompiled GoldenEye source and focused on performance, stability, expanded multiplayer/co-op support, Third Person gameplay, quality-of-life features, experimental in-game map creation, and preservation-minded enhancements.

This repository contains **source code and project material only**. It is not a substitute for the original game. A legally obtained retail ROM is required where the upstream build/extraction process needs original game data.

## Current development snapshot

This source refresh represents the current **R22-derived GoldenEye 007 Plus development tree through internal checkpoint V90 (2026-09-28)**. It supersedes the older public GitHub source snapshot from 2026-09-27.

Major work present in this tree includes:

- performance and load-time optimization passes;
- expanded 1–4 player campaign Co-Op architecture;
- expanded multiplayer maps, menus, character selection and dual-wield behavior;
- save-backed gameplay/Special Options;
- Optional Third Person camera with adjustable camera and crosshair tuning;
- Third Person cutscene handoff fixes, including the Surface II and Dam end cinematics;
- TP Sight Translucency while manually aiming;
- Third Person projectile/beam/tracer alignment work;
- Moonraker Laser Third Person straight-ray penetration work;
- shared campaign Co-Op mission-item and objective-state handling;
- Co-Op mission report/statistics routing;
- Mirrored Levels support and live-toggle safety work;
- Basic/Advanced Map Maker development with a dedicated native Map Maker stage architecture;
- Citadel restored as a dedicated multiplayer stage, using community restoration work by Krijy and Zoinkity;
- a Perfect Dark-inspired, room-aware and cylinder-aware STAN ground-support backport used generically;
- selective, evidence-based Perfect Dark backports where the underlying behavior is genuinely equivalent or appropriate.

The project deliberately keeps GoldenEye behavior as the baseline. Perfect Dark and related Rare-era material are references, not wholesale replacements.

## Current runtime notes

Some current development areas are still under stress testing. In particular, Mirrored Levels live toggling has received multiple safety fixes and should continue to be tested on large/explored stages such as Caverns.

The Surface II Third Person/Co-Op cutscene crash is runtime-confirmed fixed in:

- single-player Third Person;
- 2P Co-Op, including both players in Third Person;
- 3P and 4P Co-Op.

Dam's Third Person ending presentation has also been corrected so its scripted camera sequence can play normally while the Third Person preference remains enabled.

## Emulator note

When testing with **Project64 + GLideN64**, keep graphics-plugin behavior separate from source-code regressions. Earlier development found that GLideN64 framebuffer/depth settings can produce black-world symptoms unrelated to GoldenEye game logic.

A known useful diagnostic setting is:

`Force depth buffer clear = ON`

Record the Project64 version, GLideN64 revision, framebuffer-emulation state, and depth-buffer-clear setting with renderer bug reports.

## Building

The current tested build configuration is:

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

`MODDED_CHEATS=YES` currently enables the Plus 16 Kbit EEPROM backend by default, so the normal output path includes `-eep16`.

A helper script is also provided:

```sh
./build_geplus.sh
```

The helper intentionally defaults to **2 build jobs** rather than consuming every CPU core. Override with `JOBS=<n>` if desired.

Place a legally obtained, unmodified NTSC-U ROM in the repository root as:

```text
baserom.u.z64
```

The helper verifies the expected retail SHA-1 before extraction/build.

## Repository organization

The public source layout is intentionally cleaned of development-session clutter and generated ROM/build outputs.

Useful areas include:

- `src/game/` — gameplay, menus, AI, player systems and rendering-facing game code;
- `src/libultra/` and `src/libultrare/` — N64 runtime/library code carried by the upstream project;
- `assets/` — build-time asset definitions and source material permitted in the public tree;
- `scripts/` — build, audit, generation and extraction helpers;
- `tools/` — build and conversion utilities;
- `docs/` — project documentation where applicable.

Generated/extracted retail assets, ROMs, object files and local build products are intentionally excluded. The additive `MP_CITADEL.bin` portrait is tracked explicitly because it is part of the Plus Citadel integration rather than a retail extracted image.

## Development principles

1. **Correctness before optimization.** Known-good checkpoints are retained as comparison points and risky changes are regression-tested across 1P/2P/3P/4P.
2. **All relevant modes stay in sync.** New options, cheats and campaign state should behave consistently in every appropriate interface/player mode.
3. **Preserve attribution and provenance.** Upstream code, research, third-party tools and backported material must retain their original notices.
4. **Backport selectively.** Perfect Dark or related-engine code is used only when the function/algorithm is genuinely equivalent or clearly appropriate for GoldenEye.
5. **Separate game bugs from emulator/plugin bugs.** Reproduce and document the distinction before altering game code.
6. **Campaign Co-Op objectives are team-wide.** Mission-objective truth and required mission-item authority must not become accidentally player-local.

## Citadel restoration credits

Special thanks to **Krijy** and **Zoinkity** for their foundational work on Citadel.

- **Krijy** was the first to discover Citadel and make it playable for the wider GoldenEye community. His original playable implementation reused **Cradle's setup**, which was a practical solution because Cradle keeps all of its rooms loaded at once.
- **Zoinkity** later reworked Citadel's STAN/clipping data so the level could function correctly with the final GoldenEye engine's collision system.

GoldenEye 007 Plus builds on those community efforts while integrating Citadel as its own dedicated level rather than replacing or reusing another retail stage.

## Documentation and provenance

Before redistributing or contributing, keep these files with the source:

- `LICENSES_AND_NOTICES.txt`
- `CREDITS.txt`
- `REFERENCES.txt`
- `UPSTREAM_GOLDENEYE_README.md`

See `CURRENT_DEVELOPMENT_STATUS.md` for a concise summary of the current post-GitHub-snapshot work.

## Legal / trademark notice

GoldenEye 007, James Bond, Nintendo 64, Rare, Nintendo, MGM, EON Productions, and other names, marks, characters, artwork, audio, and game content belong to their respective owners. This fan/community project is not affiliated with, endorsed by, or sponsored by those rights holders.

No original retail ROM is distributed by this project.
