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

The Map Maker is **experimental**, and some features remain incomplete or subject to stability testing.

---

## Optimization and Engine Improvements

Performance optimization remains a central part of the project.

Development has included investigations and optimizations involving:

- Character and guard processing.
- Explosion calculations.
- Model animation and rendering.
- Visibility and fog processing.
- Rendering-related memory usage.
- Multiplayer and Co-Op performance.
- Physical-code execution paths.
- Asset handling and loading behavior.

Optimization changes are intended to maintain gameplay correctness while reducing unnecessary processing and resource usage.

Regression testing remains important, especially under multiple-player configurations and on original N64 hardware.

---

## Emulator Compatibility

Testing has included Project64 with GLideN64.

Some graphics issues can originate from emulator or graphics-plugin settings rather than the GoldenEye source.

For certain GLideN64 black-screen or black-world symptoms, a useful diagnostic setting has been:

`Force depth buffer clear = ON`

When reporting graphics problems, please include:

- Project64 version.
- GLideN64 version or revision.
- Framebuffer-emulation settings.
- Depth-buffer-clear configuration.
- Whether the issue occurs under other emulator or hardware configurations.

Source-code regressions and emulator/plugin behavior should be investigated separately.

---

## Feature Activation and Cheat Guide

This guide describes **GoldenEye 007 Plus with `MODDED_CHEATS=YES`**, not retail GoldenEye's secret button codes. Menu layout and availability can differ between development revisions, Single-Player, Multiplayer, and Co-Op. Some options are save-backed, some apply only to the current mission, and experimental features may not work in every level.

### Where to activate features

| Feature | Where / how to activate it | What it does |
|---|---|---|
| **Special Options** | Open **Special Options** on the front-end, or access Special Options from the in-game watch. | Opens the main mod feature categories and game settings. |
| **Campaign Co-Op** | Select the campaign Co-Op configuration, set the participating players, and begin a mission. | Allows multiple players to complete campaign objectives together, sharing mission-item progression and statistics. |
| **Simulants (bots)** | Open **Multiplayer** and the **Bots** configuration; select bot slots, characters, difficulty, and traits before the match. | Adds configurable AI opponents or participants (up to eight); Co-Op companions are still experimental. |
| **Third Person** | **Special Options → Enhancements → Third-Person Options**; corresponding controls are also present in the watch. | Switches to a third-person view; includes camera, crouch-height, shoulder and sight configuration. |
| **Experimental Jumping** | **Special Options → Experimental → Jumping (Press L)**, then press **L** in-game. | Enables jumping, distinct from Fly Mode. Jump/floor interactions are still experimental. |
| **Level Modifiers** | **Special Options → Level Modifiers**; choose the relevant stage and option. In-game, the watch presents modifiers applicable to the current level. | Enables stage-specific restorations, collision changes, or gameplay changes. |
| **Drivable Speedboat (Dam)** | **Level Modifiers → Single-Player → Dam → Drivable Speedboat**. Activate it before loading Dam or select **ACTIVATE** during the level. Approach the boat and press **B** to board; press **B** again to exit. | Creates a driveable boat with steering and deck support. Once activated during a level it remains **ACTIVE** until the level ends/reloads. |
| **Restore Speedboat (Dam)** | **Level Modifiers → Single-Player → Dam → Restore Speedboat**. | Restores the separate historical speedboat prop; **not** the same as Drivable Speedboat. |
| **Map Maker** | Requires **`MAP_MAKER=YES`** when compiling; then open the experimental map editor in the available mod menus. | Provides an experimental in-game level editor. The normal gameplay build below uses **`MAP_MAKER=NO`**. |

### Cheat descriptions

Added mod cheats are available through the front-end **Cheats** page and, where supported, the watch's **In-Game Cheats** page. **These added menu toggles do not require entering a controller-code sequence.** Some legacy/unlockable cheats may still be governed by save progression or cheat-unlock conditions.

| Cheat | Effect |
|---|---|
| **No Reload** | Removes the normal need to reload equipped weapons; separate from Infinite Ammo. |
| **Rapid Fire** | Increases weapon firing speed by reducing normal intervals between shots. |
| **No-Clipping** | Bypasses ordinary player collision restrictions, allowing movement through geometry. Use carefully: Bond can leave valid level space. |
| **Fly Mode** | Enables free vertical movement controlled by the viewing direction rather than normal floor-following movement. |
| **Super Kinetics** | Applies **1.5×** explosion death/launch impulse; does **not** increase explosion damage or radius. |
| **Ultra Kinetics** | Applies **3.5×** explosion death/launch impulse. Enabled together with Super Kinetics, the total multiplier is **5.0×**. |
| **Super Tank** | Increases the tank's maximum forward/reverse speeds and turning response by **2.5×**. |
| **Mirrored Levels** | Mirrors supported world geometry, STAN collision, characters, props and doors along the world X axis. Runtime toggling remains experimental. |
| **Level Never Ends** | Prevents supported normal stage-completion triggers from ending the mission. |
| **Freeze Timer** | Freezes the **mission countdown**, not every game timer or the mission clock. |
| **Max Head-On** | Extends/repeats tank-versus-guard head-on impact and crush audio feedback for a bounded duration; the normal initial impact is preserved. |
| **Infinite Ammo** | Prevents ordinary ammunition depletion; reload behavior is controlled separately. |
| **Invincibility** | Prevents normal damage to Bond. |
| **Invisibility** | Enables the classic enemy-visibility cheat behavior. |
| **All Guns** | Makes the game's weapon collection available in supported game modes. |
| **DK Mode** | Activates the classic oversized-head effect. |
| **Tiny Bond** | Makes Bond unusually small. |
| **Paintball Mode** | Uses paintball-style bullet impact marks. |
| **Turbo Mode** | Increases Bond's movement speed, separate from Super Tank. |
| **No Radar (Multiplayer)** | Hides normal multiplayer radar information. |

The following are **one-time actions**, not persistent On/Off toggles:

| Action | Effect |
|---|---|
| **Full Health** | Immediately restores Bond's health. |
| **Full Armor** | Immediately restores Bond's body armor. |
| **Normal Animation** | Resets Fast/Slow Animation to the normal speed. |
| **Complete Objectives** | Completes the applicable mission objectives and updates the shared Co-Op objective display. |
| **Complete Mission** | Immediately exits the current campaign mission through its success path; mission time is reset for the target-time reward logic. |

The original GoldenEye cheats remain available subject to their normal unlocking and game-mode rules. Cheat use may alter mission progression and challenge; use a separate save while experimenting.

### Level Modifier activation rules

Some modifiers are **reversible**, using On/Off while the level is active. Other modifiers are **latched**: selecting **ACTIVATE** changes the option to **ACTIVE**, and it cannot be turned off until leaving/restarting that level. **Drivable Speedboat** is latched; **Restore Speedboat** is a separate reversible modifier.

For Dam, the main modifier entries include **Restore Doors Near Dam Docks**, **Restore Speedboat** and **Drivable Speedboat**. Other stage-specific options include **Beta Vent Start** (Silo), selected original/beta door or object restorations, and some multiplayer geometry/respawn changes. Only modifiers available for the current stage are shown in-game.

### Development status and bug reports

Passing a build or static audit does **not** prove every feature works in-game. Bots, Map Maker, Jumping, moving-platform support, Mirrored Levels, and recent boat performance/collision corrections require continued runtime testing. For a useful bug report, include the mission/map, game mode, player count, active cheats and modifiers, and emulator/hardware used.

---

## Building GoldenEye 007 Plus

The standard **gameplay** configuration is NTSC-U, 16 Kbit EEPROM, physical fastpaths, modded cheats and **Map Maker disabled**. Run this from the repository root:

```sh
GE_CDATA_TURTLE=1 \
GE_CDATA_TURTLE_WORKERS=4 \
GE_CDATA_TURTLE_BIN="/absolute/path/to/turtledeflate_app" \
make VERSION=US \
    PHYSICAL_CODE=YES \
    PHYSICAL_FASTPATHS=YES \
    MODDED_CHEATS=YES \
    MAP_MAKER=NO \
    OPT_USE_LLVM=YES \
    COMPARE=0 \
    -j4
```

Replace the Turtledeflate binary path with your own. **Turtledeflate environment-variable integration exists in newer local development builds** and may be ignored by older GitHub snapshots. For a published snapshot without that integration, use the same ordinary flags without the three `GE_CDATA_TURTLE` variables:

```sh
make VERSION=US \
    PHYSICAL_CODE=YES \
    PHYSICAL_FASTPATHS=YES \
    MODDED_CHEATS=YES \
    MAP_MAKER=NO \
    OPT_USE_LLVM=YES \
    COMPARE=0 \
    -j4
```

`-j4` allows up to four Make jobs. When supported, `GE_CDATA_TURTLE_WORKERS=4` configures four separate compression workers. `MODDED_CHEATS=YES` selects the Plus EEPROM16 default, and the gameplay build uses the `-eep16-nomap` output identity.

To develop Map Maker, set `MAP_MAKER=YES` instead. The supplied `./build_geplus.sh` helper **currently builds with `MAP_MAKER=YES` and defaults to `JOBS=2`**. Its behavior is distinct from the standard gameplay command above; this README update does not silently alter the helper script.

Place a legally obtained, unmodified NTSC-U GoldenEye 007 ROM at the repository root as:

```text
baserom.u.z64
```

The helper checks its original ROM SHA-1 before extraction/build. No retail ROM or built ROM image is distributed here.


---

## Repository Structure

Important project directories and files include:

| Location | Purpose |
|---|---|
| `src/game/` | Gameplay, AI, menus, player systems, and engine functionality |
| `src/game/mpbots.c` | Simulant implementation |
| `src/game/mpbots.h` | Simulant declarations and interfaces |
| `src/libultra/` | Nintendo 64 runtime components |
| `src/libultrare/` | Additional upstream runtime components |
| `assets/` | Asset definitions and build-time resources |
| `scripts/` | Audits, build utilities, and extraction helpers |
| `tools/` | Development and conversion tools |
| `CURRENT_DEVELOPMENT_STATUS.md` | Current development overview |
| `CREDITS.txt` | Project credits and attribution |
| `REFERENCES.txt` | Research and technical references |

Retail ROM data and generated build outputs are not intended to be distributed with the source repository.

---

## Development Principles

1. **Preserve GoldenEye's original behavior.** Avoid unnecessary changes to established gameplay and engine behavior.
2. **Maintain backport fidelity.** Adapt existing Perfect Dark behavior faithfully, changing only what is required by engine differences.
3. **Prioritize correctness and stability.** Performance improvements must not introduce gameplay regressions.
4. **Support all relevant player configurations.** Test single-player, multiplayer, and Co-Op behavior under appropriate player counts.
5. **Preserve attribution.** Retain credits and provenance for original developers, decompilation researchers, and community contributors.
6. **Distinguish experimental development from completed functionality.** Document outstanding limitations and do not treat static audit results as proof of runtime correctness.

---

## Credits and Acknowledgments

GoldenEye 007 Plus is possible because of the work of many original developers, reverse engineers, preservationists, community researchers, and modders.

Important foundations and references include:

- The original GoldenEye 007 development team at Rare.
- The [GoldenEye 007 decompilation project](https://gitlab.com/kholdfuzion/goldeneye_src) and its contributors.
- The Perfect Dark decompilation project and its contributors, including Ryan Dwyer.
- Krijy and Zoinkity for Citadel restoration.
- BMW for Courtyard.
- GoldenEye and Perfect Dark community research and tooling.

Project64 and GLideN64 for testing and investigation.

Development and research assistance from OpenAI ChatGPT.

See CREDITS.txt, REFERENCES.txt, and LICENSES_AND_NOTICES.txt for additional information.

Additional Mod Provenance

Cradle (MP) Kill Plane Level Modifier: Originally developed by Zoinkity. GoldenEye 007 Plus ports the relevant Y < -2550 behavior while preserving the original multiplayer setup.

Additional historical GameShark-related level-modifier provenance is documented in CREDITS.txt.

Development Status and Contributions

GoldenEye 007 Plus is an ongoing community modification.

Features may change as development continues, and experimental functionality may have unresolved issues.

The project's current state is documented in CURRENT_DEVELOPMENT_STATUS.md.

Contributions, testing, bug reports, and research that help preserve original engine behavior and improve reliability are welcome.

Legal and Trademark Notice

GoldenEye 007, James Bond, Nintendo 64, Rare, Nintendo, MGM, EON Productions, and their respective characters, artwork, audio, names, and trademarks belong to their applicable rights holders.

GoldenEye 007 Plus is an unofficial fan/community project and is not affiliated with, endorsed by, or sponsored by those rights holders.

No original retail GoldenEye 007 ROM is distributed by this project.
