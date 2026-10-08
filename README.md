# GoldenEye 007 Plus

GoldenEye 007 Plus is a community modification of **GoldenEye 007 for Nintendo 64**, built on the fully decompiled GoldenEye source code.

The project focuses on improving performance, stability, gameplay flexibility, expanded multiplayer and campaign Co-Op, Perfect Dark-derived simulants (AI bots), Third Person gameplay, quality-of-life improvements, experimental map creation, and preservation-minded enhancements.

**This repository contains source code and project materials only.** It does not distribute a retail GoldenEye 007 ROM. A legally obtained original game ROM is required for the applicable build and extraction processes.

## Current Development Status — October 2026

The current public source is the **R22-derived GoldenEye 007 Plus development tree with subsequent R27/R30 integration**.

The latest major source update was published in commit [`95656a3`](https://github.com/Joshua-1248/GoldenEye-007-Plus/commit/95656a3), making the current Perfect Dark-derived simulant implementation publicly available.

This supersedes the older September 2026 V90 development snapshot.

### Major Features

- Performance optimizations covering character processing, model rendering, visibility, explosions, and other engine systems.
- Expanded **1–4 player campaign Co-Op**.
- Expanded multiplayer maps, characters, weapons, menus, and settings.
- **Perfect Dark-derived simulants (AI bots)** for multiplayer and campaign Co-Op.
- STAN-based bot navigation independent of GoldenEye's original guard waypoint system.
- Optional Third Person gameplay with adjustable camera and crosshair settings.
- Additional player death animations and gameplay options.
- Expanded multiplayer weapon handling and dual-wield functionality.
- Save-backed Special Options, cheats, and gameplay preferences.
- Expanded Co-Op objectives, shared mission items, mission reporting, and player statistics.
- Mirrored Levels support.
- Experimental Basic/Advanced Map Maker.
- Restored Citadel multiplayer stage.
- Courtyard multiplayer integration.
- Selective Perfect Dark engine and gameplay backports.

The project aims to preserve GoldenEye's original behavior wherever practical. Perfect Dark features are adapted only where necessary to accommodate differences between the two game engines.

---

## Perfect Dark-Derived Simulants (AI Bots)

One of GoldenEye 007 Plus's largest ongoing additions is the backport and adaptation of Perfect Dark's simulant architecture.

GoldenEye originally lacked Perfect Dark's multiplayer simulant system. GoldenEye 007 Plus introduces AI-controlled participants that can operate alongside human players.

### Multiplayer Simulants

The current implementation includes:

- Support for **up to 8 configurable bots**.
- **6 difficulty levels**, ranging from Very Easy to Extreme.
- Independently combinable bot personality traits.
- AI-controlled movement, combat, and target selection.
- Weapon searching, pickup, and inventory management.
- Weapon switching and reloading.
- Ranged and melee combat.
- Body Armor interaction.
- Radar participation.
- Multiplayer scoring.
- Damage handling and pain sounds.
- Player-style animation handling.
- Death animations, corpse cleanup, and respawning.
- Navigation and door interaction.

Bot difficulty and personality settings are intended to influence their behavior in ways consistent with the Perfect Dark simulant system, adapted to GoldenEye's available mechanics.

### STAN-Based Navigation

Unlike GoldenEye's original guards, which rely on guard waypoint/navigation behavior, the new simulants use a separate navigation system built around the game's STAN collision and ground geometry.

This allows the bot navigation architecture to operate independently of GoldenEye's original guard AI waypoint infrastructure.

Navigation remains under active development and runtime testing.

### Campaign Co-Op Simulants

The simulant architecture also supports campaign Co-Op.

Current development functionality includes:

- AI companions participating alongside human players.
- Following human players through missions.
- Engaging hostile guards.
- Weapon and combat handling.
- Door interaction.
- Damage and death handling.
- Integration with the expanded campaign Co-Op architecture.

Campaign Co-Op bot behavior is still being refined and tested.

### Source Code and Audits

The primary simulant implementation is available publicly:

- [`src/game/mpbots.c`](src/game/mpbots.c)
- [`src/game/mpbots.h`](src/game/mpbots.h)

Related regression and integration audits include:

- `scripts/audit_r30_p4_bot_navigation.py`
- `scripts/audit_r30_p5_bot_lifecycle_animation.py`
- `scripts/audit_r30_p6_bot_crash_hardening.py`
- `scripts/audit_r30_p7_one_player_mp_fpv.py`
- `scripts/audit_r30_p9_simulant_integration.py`
- `scripts/audit_r30_p10_simulant_player_death_stability.py`
- `scripts/audit_r30_p11_stan_navigation.py`
- `scripts/audit_r30_p12_p13_simulant_features.py`

**Development warning:** Simulants are still experimental. Navigation, animation timing with multiple human players, death-animation completion, respawning, frame pacing, and overall stability remain subjects of runtime testing. A successful compilation or static audit does not guarantee that every behavior works correctly in-game.

The current implementation is not claimed to have complete behavioral parity with Perfect Dark.

---

## Expanded Campaign Co-Op

GoldenEye 007 Plus expands the original single-player campaign to support up to four human players, with ongoing support for AI companions.

Development work includes:

- Shared campaign mission progression and objective state.
- Team-wide mission-item handling.
- Improved Co-Op player lifecycle management.
- Expanded player targeting and friendly-fire handling.
- Co-Op mission reporting and statistics.
- Campaign death and GAME OVER handling.
- Cutscene and end-of-level transitions.
- Improved split-screen gameplay behavior.
- Multiple-player mission testing and stability corrections.

The Co-Op architecture is designed to preserve GoldenEye's original campaign systems while adapting them for multiple participants.

---

## Third Person Gameplay

GoldenEye 007 Plus includes an optional Third Person gameplay mode.

Development work includes:

- Adjustable camera behavior.
- Crosshair and aiming improvements.
- Weapon projectile, tracer, and beam alignment.
- Camera positioning and collision-related refinements.
- Third Person death presentation.
- Cutscene transitions and camera handoffs.
- Compatibility improvements for campaign and Co-Op gameplay.

The Surface II Third Person/Co-Op cutscene crash was runtime-confirmed fixed in single-player Third Person and 2P–4P Co-Op configurations.

Dam's ending camera presentation was also corrected to allow its scripted sequence to function while Third Person remains enabled.

---

## Multiplayer Enhancements

GoldenEye 007 Plus includes several multiplayer expansions and refinements.

These include:

- Additional playable multiplayer maps.
- Expanded multiplayer character availability.
- Duplicate-character selection.
- Expanded weapon selections.
- Dual-wield weapon handling.
- Multiplayer HUD improvements.
- Additional gameplay options and cheats.
- Expanded multiplayer settings and menu functionality.
- One-player multiplayer support.
- Perfect Dark-derived bot integration.

### Citadel

Citadel is integrated as a dedicated multiplayer stage.

Special thanks to:

**Krijy** — First discovered Citadel and made the level playable for the wider GoldenEye community. His early playable implementation reused Cradle's setup because Cradle keeps all its rooms loaded simultaneously.

**Zoinkity** — Later reworked Citadel's STAN and clipping information to make the stage compatible with the final GoldenEye engine's collision system.

GoldenEye 007 Plus builds on this community restoration work while integrating Citadel as its own stage.

### Courtyard

GoldenEye 007 Plus also includes Courtyard multiplayer integration.

**BMW** is credited as the creator of Courtyard, the first GoldenEye level created using Valve Hammer Editor.

The original community authorship and contributions remain recognized.

---

## Map Maker

GoldenEye 007 Plus includes ongoing development of an experimental in-game Map Maker.

Development work includes:

- Basic and Advanced editing functionality.
- Geometry and mesh editing.
- Texture selection and application.
- Entity and pickup placement.
- Dedicated Map Maker stage architecture.
- Native map testing.
- Experimental in-game editing and storage systems.

The Map Maker is **experimental**, and
