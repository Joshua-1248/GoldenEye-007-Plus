# Current Development Status

Current public-development line: **R22-derived GoldenEye 007 Plus with later R27/R30 development — October 2026**.

The project has advanced substantially beyond the older V90 public snapshot.

## Major current work

- Perfect Dark-derived Simulant/bot architecture adapted for GoldenEye multiplayer;
- up to 8 configurable bots;
- six bot difficulty levels from Very Easy through Extreme;
- independent combinable bot personality traits;
- bot weapon pickup, reload, switching, melee, armor, scoring, radar and respawn support;
- STAN-based bot navigation independent of GoldenEye guard waypoint AI;
- Co-Op bots that follow human players and engage hostile guards;
- player-style bot animations, death animations, pain sounds and damage behavior;
- expanded Multiplayer maps and characters;
- Citadel integration using restoration work by Krijy and Zoinkity;
- Courtyard integration by BMW, creator of the first GoldenEye level made with Valve Hammer Editor;
- expanded 1-4 player campaign Co-Op;
- Co-Op mission-state, objective and mission-report work;
- Third Person gameplay and camera systems;
- Mirrored Levels;
- save-backed gameplay and Special Options;
- experimental Map Maker development;
- ongoing optimization and stability work.

## Simulant development status

Simulants now participate in normal Multiplayer and campaign Co-Op.

Current functionality includes weapon seeking and pickup, armor, ranged combat, melee, reloads, weapon switching, scoring, radar, natural corpse cleanup and respawning, player-like damage grace, pain audio, Co-Op following, hostile-guard targeting and door interaction.

Bot navigation and behavior remain under active runtime testing.

## Current tested build

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

The current development build uses the Plus 16 Kbit EEPROM configuration.

## Attribution

- **Krijy** — early playable Citadel restoration work;
- **Zoinkity** — final-engine-compatible Citadel STAN/clipping rework;
- **BMW** — creator of Courtyard, the first GoldenEye level made using Valve Hammer Editor.

See `CREDITS.txt` and `REFERENCES.txt` for fuller provenance.
