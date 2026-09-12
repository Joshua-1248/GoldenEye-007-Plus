# GoldenEye 007 Plus

GoldenEye 007 Plus is a community modification of **GoldenEye 007 for Nintendo 64**, built on the fully decompiled GoldenEye source and focused on performance, stability, expanded multiplayer/co-op support, quality-of-life features, and preservation-minded enhancements.

This repository contains **source code and project material only**. It is not a substitute for the original game and is intended to be built using a legally obtained copy of the retail ROM where required by the build system.

## Current development baseline

This snapshot is the current **R22** development baseline and includes the cumulative work carried forward from R21, including:

- performance and load-time optimizations;
- expanded 1–4 player co-op architecture;
- multiplayer map/menu enhancements;
- dual-wield multiplayer work;
- expanded Watch and Options interfaces;
- save-backed gameplay options;
- `Disable Hitstun`;
- `Disable Knockback`;
- `Disable Noise Dithering`;
- configurable damage flash, crosshair and taking-damage sound;
- expanded in-game cheats including Rapid Fire and No Reload;
- SP Watch Special Options scrolling for the expanded option list;
- selective, evidence-based Perfect Dark backports where the underlying behavior is genuinely equivalent or useful.

The project deliberately keeps GoldenEye behavior as the baseline. Perfect Dark and other related Rare-era material are references, not wholesale replacements.

## Important emulator note

When testing with **Project64 + GLideN64**, enable:

`Force depth buffer clear = ON`

An intermittent split-screen black-world/background problem was traced to GLideN64 state rather than GoldenEye game code. Do not confuse that plugin issue with the separate, genuine code-side room-lifetime problem that was fixed earlier in development.

## Building

The simplest supported build path is:

```sh
./build_geplus.sh
```

Place a legally obtained, unmodified NTSC-U ROM in the repository root as `baserom.u.z64` first. The helper verifies the expected retail SHA-1, extracts only the required retail assets, preserves GoldenEye 007 Plus-specific model directories, and builds using:

```sh
make VERSION=US PHYSICAL_CODE=YES PHYSICAL_FASTPATHS=YES MODDED_CHEATS=YES OPT_USE_LLVM=YES COMPARE=0
```

`OPT_USE_LLVM=YES` is required by the current project toolchain/environment. The build may emit expected LLVM music-relocation warnings which are repaired by the project wrapper.

The compressed C/data slot is currently constrained to **72,704 bytes**. The present R22 build reaches that limit exactly, so future C additions should first reclaim space or use an appropriate size-neutral approach rather than silently expanding the established memory layout.

## Retail ROM requirement

The upstream GoldenEye decompilation build process expects a legally obtained retail ROM for extraction of copyrighted assets that are not distributed as freely licensed project material. Do not commit or redistribute retail ROMs or extracted proprietary game assets unless you have the legal right to do so.

## Repository organization

This repository has been intentionally cleaned of development-session artifacts, old release patches, build logs, test matrices, and superseded status files. It contains the current GoldenEye 007 Plus source, required build tooling/assets, and public-facing documentation.

Useful areas include:

- `src/game/` — gameplay, menus, AI, player systems, rendering-facing game code;
- `src/libultra/` and `src/libultrare/` — N64 runtime/library code carried by the upstream project;
- `assets/` — build-time asset definitions and generated/extracted material;
- `tools/` — build and conversion utilities;

## Development principles

The project follows several standing rules:

1. **Correctness before optimization.** Known-good releases are retained as comparison points and risky changes are regression-tested across 1P/2P/3P/4P.
2. **All relevant modes stay in sync.** When a new option, cheat, or configurable feature is added, it should be exposed in every appropriate menu and player mode rather than being stranded in one interface.
3. **Preserve attribution and provenance.** Upstream code, research, third-party tools, and backported material must retain their original notices and be documented.
4. **Backport selectively.** Perfect Dark or related-engine code is used only when the function/algorithm is genuinely equivalent or clearly appropriate for GoldenEye.
5. **Separate game bugs from emulator/plugin bugs.** Reproduce and document the distinction before altering game code.

## Documentation

See these files before redistributing or contributing:

- `LICENSES_AND_NOTICES.txt` — licensing and redistribution notes;
- `CREDITS.txt` — project and upstream attribution;
- `REFERENCES.txt` — source/research references and provenance;

The original GoldenEye decompilation README is preserved as `UPSTREAM_GOLDENEYE_README.md`.

## Legal / trademark notice

GoldenEye 007, James Bond, Nintendo 64, Rare, Nintendo, MGM, EON Productions, and other names, marks, characters, artwork, audio, and game content belong to their respective owners. This fan/community project is not affiliated with, endorsed by, or sponsored by those rights holders.

No original retail ROM is distributed by this project.
