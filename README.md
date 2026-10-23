# GoldenEye 007 Plus

**GoldenEye 007 Plus** is a community modification of *GoldenEye 007* for the Nintendo 64, built upon the GoldenEye 007 decompilation project.

The project expands the original game with performance improvements, multiplayer enhancements, campaign Co-Op, Perfect Dark-inspired features, AI Simulants, Third-Person gameplay, additional cheats, restored content, experimental features, and numerous quality-of-life improvements.

The goal is to improve and expand GoldenEye while remaining faithful to the original game's mechanics, presentation, and overall experience.

**This repository contains source code and development materials only. It does not distribute the original GoldenEye 007 ROM.**

---

## Current Development Status — October 2026

GoldenEye 007 Plus is under active development.

Major implemented and ongoing features include:

- Performance optimizations and physical-code improvements.
- Expanded campaign Co-Op supporting up to four human players.
- Perfect Dark-derived AI Simulants for Multiplayer and experimental campaign Co-Op.
- Additional Multiplayer levels and configuration options.
- Third-Person gameplay with adjustable camera settings.
- Expanded Special Options and gameplay preferences.
- Additional cheats and cheat actions.
- Restored level content and Level Modifiers.
- The original **Master Control Debug Menu**, accessible through a new activation option.
- Experimental Jumping and Move Anywhere.
- A drivable speedboat in Dam.
- Additional player death animations.
- Enhanced mission statistics and reports.
- Mirrored Levels.
- Experimental Map Maker support.
- Customizable Main Menu and Watch Menu themes.

Not every feature is finalized. Experimental systems, especially Simulant navigation and moving-platform collision, remain subject to gameplay testing.

A successful build or static audit does not necessarily establish that every feature operates correctly during gameplay.

---

# Special Options and Feature Activation

GoldenEye 007 Plus introduces an expanded **Special Options** menu.

This menu provides access to additional gameplay settings, visual enhancements, cheats, experimental features, Level Modifiers, debugging options, and other modifications.

Many settings can also be accessed through the in-game watch.

## Accessing Special Options

From the game's main menu:

1. Open **Special Options**.
2. Navigate between the available pages using the menu controls.
3. Select a setting to change its value or open its submenu.
4. Return to the previous screen when finished.

In Single-Player, Multiplayer, and Co-Op, supported options are also available through the in-game watch.

Certain settings take effect immediately, while others are saved or applied when the next level begins.

Some settings may be unavailable in particular game modes.

## Special Options — Main Settings

The expanded menu includes the original gameplay settings and additional modifications.

| Option | Description |
|---|---|
| Music Volume | Adjusts the game's music volume, with a percentage indicator. |
| FX Volume | Adjusts sound-effects volume, with a percentage indicator. |
| Control Style | Selects the controller configuration. |
| Look Up/Down | Changes the vertical-look direction. |
| Auto Aim | Enables or disables automatic aiming assistance. |
| Aim Control | Selects Hold or Toggle aiming behavior. |
| Sight On Screen | Controls the standard aiming sight. |
| Look Ahead | Controls automatic camera centering. |
| Ammo On Screen | Shows or hides ammunition information. |
| Screen Size | Selects the screen presentation. |
| Screen Ratio | Supports 4:3, 16:9, 21:9, and 32:9 configurations. |
| Head Roll | Controls the head-roll movement effect. |
| Endless Death Cam | Allows the death camera to continue without automatically ending the view. |
| Real-Time Collapse | Controls whether certain death and collapse events continue in real time. |
| Damage Hitstun | Enables or disables the player's hitstun response when taking damage. |
| Damage Knockback | Enables or disables movement knockback caused by damage. |
| Damage Flash | Controls the screen-flash effect when taking damage. |
| Crosshair | Enables or disables the standard crosshair. |
| Taking Damage Sound | Controls the sound associated with taking damage. |
| Experimental | Opens experimental gameplay features. |
| Enhancements | Opens additional gameplay and visual enhancements. |
| Main Menu Theme | Selects the music theme used by the main menu. |
| Watch Menu Theme | Selects the watch menu's music theme. |

Additional submenus are available for Level Modifiers, Patches, Single-Player Options, Third-Person Options, and Debug.

## Special Options — Additional Categories

| Category | Purpose |
|---|---|
| Enhancements | Additional gameplay, presentation, and quality-of-life settings. |
| Experimental | Features still under development, including Jumping and Move Anywhere. |
| Level Modifiers | Stage-specific changes, restored content, and experimental alterations. |
| Patches | Optional gameplay fixes and behavior adjustments. |
| Third-Person Options | Third-Person camera, aiming, and related settings. |
| Single-Player Options | Single-player character and associated presentation settings. |
| Debug | Access to the Master Control Debug Menu activation setting. |
| Map Maker | Experimental map editor, available only in Map Maker-enabled builds. |

The options shown may differ between the main menu and in-game watch.

### Enhancements

The Enhancements category contains optional improvements and additional functionality.

Examples include:

- Third-Person gameplay-related options.
- Additional player death animations.
- Staggering Backwards Death.
- Other visual and gameplay preferences.

Certain enhancements are configurable through their own submenus.

### Patches

The Patches category provides optional corrections and gameplay modifications.

These include features such as:

| Option | Description |
|---|---|
| Micro-Optimizations | Enables supported runtime micro-optimizations. |
| Noise Dithering | Controls the relevant dithering presentation option. |
| Unlimited Explosions | Removes the normal restriction on the number of simultaneous explosions. |
| Disable Body Armor | Disables supported body-armor pickup behavior. |
| Silo X Music Loop Fix | Addresses the Silo X music-loop issue. |
| AR33 Prop Fix (MP) | Applies the associated multiplayer weapon-prop correction. |

Other patch options may be available depending on the development revision.

### Main Menu and Watch Menu Themes

GoldenEye 007 Plus supports selectable music themes.

Available selections include:

- GoldenEye 007
- Goldfinger 64
- Tomorrow Never Dies 64
- The Spy Who Loved Me 64

The Main Menu Theme and Watch Menu Theme are separate settings.

These settings are saved where supported by the selected save backend.

---

# Master Control Debug Menu

GoldenEye 007 Plus restores access to GoldenEye's original **Master Control Debug Menu** through a configurable option.

This is an internal debugging interface originating from the game's development systems.

It is intended for exploration, testing, and investigating the game's behavior.

**The Master Control Debug Menu is disabled by default.**

## How to Enable the Master Control Debug Menu

From the main menu:

1. Open **Special Options**.
2. Select **Debug**.
3. Find **Master Control Debug Menu**.
4. Change the option from **Off** to **On**.
5. Start a mission or enter gameplay.

The Debug submenu is located alongside the other additional Special Options categories.

The option can also be changed through the corresponding **Debug** page in the in-game watch, where supported.

This setting is saved using GoldenEye 007 Plus's extended settings system.

## How to Open the Master Control Debug Menu

Once the setting is enabled and you are inside a level, press the following buttons simultaneously:

**L + R + D-Pad Up + C-Up**

The complete combination is:

| Button | Required |
|---|---|
| L | Hold |
| R | Hold |
| D-Pad Up | Hold |
| C-Up | Hold |

All four buttons must be held together. At least one of the combination's buttons must register a new press.

The combination is handled by Controller 1.

It is intended for use during gameplay, not on the title screen.

## Master Control Debug Menu Controls

| Control | Action |
|---|---|
| D-Pad Up | Move the selection upward. |
| D-Pad Down | Move the selection downward. |
| D-Pad Left | Navigate or adjust the selected debug option. |
| D-Pad Right | Navigate or adjust the selected debug option. |
| A | Activate the highlighted option. |
| Start | Close the Master Control Debug Menu and return to gameplay. |

The menu retains the original GoldenEye debugging functionality, with adaptations to its activation and input handling.

Its available operations include development-oriented display and gameplay controls, such as view-related and STAN-related functionality.

Some options affect the internal game state and may behave differently from ordinary gameplay settings.

**Warning:** The Master Control Debug Menu is intended for debugging. Certain operations may interrupt normal gameplay, produce unusual behavior, or make the current mission impossible to complete normally.

Use it with caution.

## Disabling the Master Control Debug Menu

Return to:

**Special Options → Debug → Master Control Debug Menu → Off**

Turning this setting off prevents the debug menu from being opened through the GoldenEye 007 Plus activation combination.

The ordinary retail debug-menu activation behavior is not substituted for this combination in standard modded gameplay builds.

---

# Cheats and Cheat Descriptions

GoldenEye 007 Plus expands GoldenEye's original cheat system with additional cheats and immediate-action commands.

Most cheats can be accessed from:

- The main-menu **Cheats** screen.
- The in-game watch's **In-Game Cheats** page, where available.

Some cheats may require unlocking or appropriate game-mode conditions.

Cheats fall into two main categories:

**Toggle cheats** remain enabled until switched off or otherwise cleared.

**Action cheats** execute an immediate action when selected and do not remain enabled afterward.

## Gameplay Cheats

| Cheat | Description |
|---|---|
| Invincibility | Prevents normal damage to Bond. |
| Invisibility | Enables the original enemy-visibility cheat behavior. |
| Infinite Ammo | Prevents ordinary ammunition depletion. |
| No Reload | Removes the normal requirement to reload weapons. |
| Rapid Fire | Reduces weapon firing delays, allowing weapons to fire more rapidly. |
| All Guns | Makes the available weapon collection accessible in supported game modes. |
| No-Clipping | Allows Bond to move through geometry without normal collision restrictions. |
| Fly Mode | Enables movement through the air using the player's viewing direction. |
| Turbo Mode | Increases Bond's movement speed. |
| Tiny Bond | Reduces Bond's size. |
| DK Mode | Activates the familiar large-head effect. |
| Paintball Mode | Changes bullet-impact marks to paintball-style effects. |

### No-Clipping

No-Clipping bypasses normal collision restrictions.

It allows Bond to move through walls and other level geometry.

Because GoldenEye's maps rely on collision and room data, leaving the normal playable environment may cause unusual behavior.

### Fly Mode

Fly Mode enables flight-style movement.

Vertical movement follows the viewing direction rather than relying exclusively on the normal floor-based movement system.

Fly Mode and No-Clipping are separate cheats.

### No Reload and Infinite Ammo

These are independent options.

**No Reload** removes the need to perform normal weapon reload operations.

**Infinite Ammo** prevents ammunition from being consumed normally.

They can be used independently or together.

## Additional GoldenEye 007 Plus Cheats

| Cheat | Description |
|---|---|
| Super Kinetics | Increases the explosion-related death/launch impulse by 1.5×. |
| Ultra Kinetics | Increases the explosion-related death/launch impulse by 3.5×. |
| Super Tank | Increases the tank's forward/reverse maximum speeds and turning response by 2.5×. |
| Mirrored Levels | Mirrors supported level geometry and gameplay objects across the world X axis. |
| Level Never Ends | Prevents supported normal mission-ending events from finishing the level. |
| Freeze Timer | Freezes the mission countdown. |
| Max Head-On | Extends the tank-versus-guard head-on impact audio feedback for a limited period. |

### Super Kinetics

Super Kinetics increases the impulse applied to characters by explosions.

The multiplier is **1.5×**.

It changes the resulting launch/death movement rather than increasing explosion damage or blast radius.

### Ultra Kinetics

Ultra Kinetics increases the same impulse by **3.5×**.

When Super Kinetics and Ultra Kinetics are enabled together, the effective multiplier is **5.0×**.

### Super Tank

Super Tank increases the tank's:

- Maximum forward speed.
- Maximum reverse speed.
- Turning response.

The speed/handling multiplier is **2.5×**.

### Mirrored Levels

Mirrored Levels reflects supported gameplay geometry and world objects along the world X axis.

Supported transformations include the relevant:

- Stage geometry.
- STAN collision.
- Characters.
- Props.
- Doors.

Runtime transitions involving this cheat remain experimental and require further gameplay testing.

### Level Never Ends

Level Never Ends blocks supported normal level-ending conditions.

This can be useful when exploring a level or investigating gameplay systems without triggering ordinary mission completion.

### Freeze Timer

Freeze Timer stops the supported mission countdown from decreasing.

It does not freeze every internal game timer or automatically stop all game processing.

### Max Head-On

Max Head-On modifies the tank's head-on collision feedback.

It allows repeated impact/crush audio feedback for a limited, controlled period while retaining the normal initial collision effect.

## Immediate Cheat Actions

Unlike toggle cheats, these commands activate immediately.

| Action | Description |
|---|---|
| Full Health | Restores Bond's health. |
| Full Armor | Restores body armor. |
| Normal Animation | Restores normal animation speed following Fast/Slow Animation changes. |
| Complete Objectives | Completes applicable current mission objectives. |
| Complete Mission | Completes the current campaign mission through its normal success path. |

### Complete Objectives

Marks applicable mission objectives as completed.

In Co-Op, the shared objective display is updated accordingly.

### Complete Mission

Completes the current mission using the normal mission-success and exit processing.

It also resets the mission time used by the associated target-time reward logic.

This is an immediate action, not an On/Off cheat.

**Warning:** Cheat usage can affect progression, gameplay balance, and mission outcomes. Consider using a separate save file when experimenting.

---

# Level Modifiers

Level Modifiers provide optional changes to individual levels.

They can restore unused content, modify level collision, alter starting conditions, or introduce experimental gameplay functionality.

## Accessing Level Modifiers

From the main menu:

1. Open **Special Options**.
2. Select **Level Modifiers**.
3. Select the appropriate game-mode category.
4. Select the level.
5. Choose the modifier.

Supported modifiers can also be accessed through the in-game watch for the current stage.

## Modifier Activation Behavior

Level Modifiers generally use one of two activation methods.

### Reversible Modifiers

These can be enabled or disabled when their implementation supports it.

They typically display **On** or **Off**.

### Latched Modifiers

These use the **ACTIVATE** command.

Once activated, they display **ACTIVE**.

A latched modifier cannot be disabled during the same level.

Its state is reset when the level ends or reloads.

## Dam Level Modifiers

| Modifier | Description |
|---|---|
| Restore Doors Near Dam Docks | Restores the additional doors in the Dam dock area. |
| Restore Speedboat | Restores the historical speedboat prop. |
| Drivable Speedboat | Adds a player-controllable speedboat. |

### Drivable Speedboat

The Drivable Speedboat is separate from the standard restored speedboat.

To activate it:

1. Open **Level Modifiers**.
2. Select **Single-Player**.
3. Select **Dam**.
4. Select **Drivable Speedboat**.
5. Activate the modifier.
6. Start or continue the mission.

During gameplay:

**Press B near the drivable speedboat to board it.**

**Press B again to exit.**

The boat supports movement, steering, and deck-related player interaction.

Once activated, the modifier remains active until leaving or restarting the level.

The system is still being refined, particularly around shoreline collision, moving-platform interaction, and performance.

### Dam Island Fixed Collisions

Dam Island Fixed Collisions is another Dam-specific gameplay modifier.

Its activation is intended to work during gameplay without requiring a level restart.

Once activated, it remains active until leaving the level.

Other Dam restoration features should not be confused with Drivable Speedboat.

## Other Level Modifiers

Additional modifiers include:

- Silo Beta Vent Start.
- Facility restoration options.
- Surface and Bunker restoration options.
- Frigate-related restorations.
- Multiplayer collision alterations.
- Multiplayer respawn-related modifications.
- Cradle multiplayer kill-plane behavior.

Not every modifier is available in every level.

### Cradle Multiplayer Kill Plane

The Cradle multiplayer kill-plane modifier incorporates behavior originally developed by **Zoinkity**.

It uses the relevant vertical boundary behavior for the multiplayer stage while retaining its original setup.

Additional modifier provenance is documented in `CREDITS.txt`.

---

# Experimental Features

The Experimental category contains features that are still being developed or refined.

These features are optional.

## Jumping

To enable:

**Special Options → Experimental → Jumping (Press L)**

Once enabled:

**Press L during gameplay to jump.**

This introduces jumping functionality beyond GoldenEye's original movement system.

Jumping remains experimental, particularly around moving platforms, unusual floor geometry, and collision boundaries.

## Move Anywhere

Move Anywhere extends movement beyond the original STAN-based traversal restrictions.

It is intended to improve movement across geometry that the original game does not normally allow Bond to traverse.

Development work includes:

- Expanded floor and triangle selection.
- Improved handling of some floor seams.
- Geometry and room-visibility handling.
- Experimental traversal outside ordinary STAN movement paths.

Known limitations still under development include:

- Certain stairs and steep slopes.
- Some prop collision interactions.
- Ladder traversal.
- Interactions with No-Clipping and Fly Mode.
- Moving platforms, including the Dam speedboat.
- Additional performance optimization.

Move Anywhere is experimental and is not guaranteed to work correctly with every level feature.

## Map Maker

GoldenEye 007 Plus includes an experimental Map Maker.

The current work covers:

- Basic and Advanced editing functionality.
- Geometry and mesh editing.
- Texture selection.
- Entity and pickup placement.
- Dedicated Map Maker stage functionality.
- Native testing.
- Experimental editing and storage systems.

**Map Maker is disabled in the normal gameplay build.**

To compile a Map Maker-enabled configuration, use:

`MAP_MAKER=YES`

For ordinary GoldenEye 007 Plus gameplay builds, use:

`MAP_MAKER=NO`

Map Maker is not considered finalized.

---

# Third-Person Gameplay

GoldenEye 007 Plus supports optional Third-Person gameplay.

This mode provides an alternative camera view while preserving the original First-Person option.

## Accessing Third-Person Options

Open:

**Special Options → Enhancements → Third-Person Options**

The corresponding Third-Person settings are also available through supported in-game menus.

Available settings include:

| Option | Description |
|---|---|
| Third Person | Enables or disables Third-Person gameplay. |
| Stay In TP On Death | Retains the Third-Person presentation during supported death sequences. |
| TP Crouch Cam | Enables crouch-related camera-height behavior. |
| Directional Shoulder | Controls directional shoulder-camera behavior. |
| TP Sight Translucency | Adjusts Third-Person aiming-sight translucency. |
| TP Cam Distance | Adjusts camera distance from Bond. |
| TP Cam Height | Adjusts camera height. |
| TP Cam Horizontal | Adjusts the horizontal camera offset. |
| TP Cam Down Frame | Adjusts the downward camera framing. |
| TP Crouched Cam Height | Controls the camera height while crouching. |
| TP Crosshair Range | Adjusts the Third-Person crosshair range. |
| TP World-Space Crosshair | Controls the world-space crosshair presentation. |

Additional development work has addressed:

- Camera collision behavior.
- Aiming and reticle positioning.
- Projectile and beam alignment.
- Third-Person death animations.
- Cutscene transitions.
- Multiplayer and Co-Op compatibility.

Third-Person integration remains an active development area.

---

# Perfect Dark-Derived Simulants (AI Bots)

One of GoldenEye 007 Plus's largest additions is a backport and adaptation of Perfect Dark's Simulant architecture.

The original GoldenEye 007 did not contain Perfect Dark's multiplayer Simulant system.

GoldenEye 007 Plus introduces AI-controlled participants for supported Multiplayer and campaign Co-Op configurations.

## Multiplayer Simulants

The implementation includes support for **up to eight configurable Simulants**.

Features include:

- Multiple difficulty levels.
- Independently configurable personality traits.
- Target selection.
- AI movement and navigation.
- Weapon searching and pickup.
- Weapon switching and reloading.
- Ranged and melee combat.
- Body Armor interaction.
- Radar participation.
- Multiplayer scoring.
- Damage and pain reactions.
- Player-style animations.
- Death handling and respawning.
- Door interaction.

## Configuring Simulants

From Multiplayer, open the **Bots** settings.

Available configuration options include:

- Bot slots.
- Character selection.
- Difficulty.
- Team assignment.
- Personality traits.

The menu supports multiple independently configured AI participants.

Some options depend on the selected Multiplayer mode.

## STAN-Based Navigation

Simulants use a navigation system based on GoldenEye's STAN collision and floor geometry.

This differs from the original guard waypoint navigation system.

The intention is to allow Simulants to navigate levels using their own traversal logic rather than relying on the behavior of regular GoldenEye guards.

Navigation is still being refined and tested.

## Campaign Co-Op Simulants

Experimental campaign Co-Op support allows Simulants to participate alongside human players.

Development includes:

- Following human players.
- Attacking hostile guards.
- Weapon and combat behavior.
- Door interaction.
- Damage and death handling.
- Integration with shared campaign objectives.

Campaign Simulants are still experimental.

## Source Code

Primary implementation:

- [`src/game/mpbots.c`](src/game/mpbots.c)
- [`src/game/mpbots.h`](src/game/mpbots.h)

Relevant source audits can be found in the `scripts/` directory.

**Development warning:** Simulant navigation, animations, multiplayer stability, respawning, and campaign behavior require continued runtime testing.

The implementation does not claim complete behavioral parity with Perfect Dark.

---

# Expanded Campaign Co-Op

GoldenEye 007 Plus expands the original campaign for multiple players.

The current architecture supports up to **four human players**, with additional experimental AI companion functionality.

Features and improvements include:

- Shared mission progression.
- Shared objective state.
- Team-wide mission-item handling.
- Improved player lifecycle management.
- Friendly-fire handling.
- Co-Op mission statistics.
- Multiple-player death and GAME OVER handling.
- Mission cutscene handling.
- Expanded split-screen gameplay.
- Campaign-specific stability improvements.

## Co-Op Mission Report

The expanded mission report provides additional player statistics.

These include:

- Mission time.
- Deaths.
- Accuracy.
- Shots fired.
- Kills.
- Head hits.
- Body hits.
- Limb hits.
- Other hits.
- Hit percentages.
- Weapon of Choice.

Statistics are presented for the relevant participating players.

## Multiplayer and Co-Op Death Handling

GoldenEye 007 Plus includes improvements to distinguish between:

- Single-Player deaths.
- Multiplayer deaths.
- Co-Op player deaths.
- All-players-dead campaign GAME OVER states.

The goal is to maintain appropriate game-mode behavior without incorrectly invoking the Single-Player death sequence during multiplayer gameplay.

---

# Multiplayer Enhancements

GoldenEye 007 Plus expands the original multiplayer experience.

Features include:

- Additional playable multiplayer levels.
- Expanded multiplayer character selection.
- Duplicate-character selection.
- Additional weapon selections.
- Dual-wield support.
- Expanded HUD configurations.
- One-human-player multiplayer support.
- Improved game settings.
- Simulant support.
- Additional Level Modifiers.
- Expanded gameplay and cheat options.

## Citadel

Citadel is integrated as a dedicated multiplayer stage.

Special thanks to:

**Krijy** — First discovered Citadel and made the level playable for the wider GoldenEye community. His early playable version reused Cradle's setup because Cradle keeps its rooms loaded simultaneously.

**Zoinkity** — Later reworked Citadel's STAN and clipping information to make it compatible with the final GoldenEye engine's collision system.

GoldenEye 007 Plus builds upon this community restoration work.

## Courtyard

GoldenEye 007 Plus includes Courtyard multiplayer integration.

**BMW** is credited as the creator of Courtyard, the first GoldenEye level created using Valve Hammer Editor.

Original community authorship and contributions remain recognized.

---

# Additional Gameplay Enhancements

GoldenEye 007 Plus includes other optional features beyond the major systems.

These include:

- Additional player death animations.
- Staggering Backwards Death.
- Damage Flash settings.
- Damage Hitstun settings.
- Damage Knockback settings.
- Endless Death Cam.
- Real-Time Collapse.
- Enemy Bullet Holes.
- Optional gameplay patches.
- Micro-Optimizations.
- Expanded screen-ratio options.
- Customizable menu themes.
- Additional menu and HUD improvements.

These features are intended to expand customization while preserving the original game's fundamental behavior.

---

# Performance and Engine Improvements

Performance optimization is a major part of GoldenEye 007 Plus.

Development has included optimizations and investigations involving:

- Character processing.
- Guard processing.
- Model animation.
- Model rendering.
- Visibility.
- Fog calculations.
- Explosion calculations.
- Physics and collision.
- Multiplayer and Co-Op performance.
- Memory usage.
- Physical-code execution.
- Asset handling and compression.

Performance improvements are intended to reduce unnecessary processing while maintaining the original gameplay behavior.

The project prioritizes correctness and stability over changes that only improve performance by altering gameplay rules.

Runtime verification remains important, particularly in split-screen configurations and on original Nintendo 64 hardware.

---

# Building GoldenEye 007 Plus

The recommended standard gameplay configuration uses:

- NTSC-U / United States.
- Physical-code execution.
- Physical fastpaths.
- Modded cheats and enhancements.
- EEPROM16 saving.
- LLVM-enabled build tools.
- Map Maker disabled.
- Four Make jobs.

## Requirements

The project is developed primarily using Linux.

Linux Mint, Ubuntu, and other Debian-compatible distributions are suitable environments.

A legally obtained, unmodified NTSC-U GoldenEye 007 ROM is required.

Place the ROM in the project root as:

`baserom.u.z64`

The source repository does not include the game ROM.

## Standard Gameplay Build

Run the following from the project root.

```bash
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

Replace `/absolute/path/to/turtledeflate_app` with the location of your Turtledeflate executable.

The Turtledeflate environment variables are applicable to development trees containing that integration.

For source revisions without the Turtledeflate integration, the ordinary gameplay configuration is:

```bash
make VERSION=US \
    PHYSICAL_CODE=YES \
    PHYSICAL_FASTPATHS=YES \
    MODDED_CHEATS=YES \
    MAP_MAKER=NO \
    OPT_USE_LLVM=YES \
    COMPARE=0 \
    -j4
```

### Build Configuration

| Flag | Purpose |
|---|---|
| `VERSION=US` | Builds the NTSC-U version. |
| `PHYSICAL_CODE=YES` | Enables the physical-code architecture. |
| `PHYSICAL_FASTPATHS=YES` | Enables supported physical fastpaths. |
| `MODDED_CHEATS=YES` | Includes GoldenEye 007 Plus gameplay enhancements. |
| `MAP_MAKER=NO` | Excludes the experimental Map Maker from the normal build. |
| `OPT_USE_LLVM=YES` | Enables the supported LLVM toolchain configuration. |
| `COMPARE=0` | Disables retail ROM comparison for the modified build. |
| `-j4` | Uses up to four concurrent Make jobs. |
| `GE_CDATA_TURTLE_WORKERS=4` | Uses four Turtledeflate workers when supported. |

The standard gameplay configuration uses the Plus EEPROM16 save backend.

The normal gameplay output is associated with:

`build/u-phys-opt-mod-eep16-nomap/`

Exact output filenames can vary between configurations.

## Map Maker Build

To build a version with the experimental Map Maker enabled, use:

`MAP_MAKER=YES`

The supplied `build_geplus.sh` helper may use a different configuration from the standard gameplay command.

Review its selected build flags before using it.

## Nintendo 64 Requirements

The physical-code configuration requires the appropriate Nintendo 64 memory environment, including Expansion Pak support.

Testing on emulators and original hardware should use the required memory configuration.

---

# Repository Structure

| Directory or File | Purpose |
|---|---|
| `src/` | Main game source code. |
| `src/game/` | Gameplay, menus, AI, player systems, and collision logic. |
| `src/game/mpbots.c` | Simulant implementation. |
| `src/game/mpbots.h` | Simulant declarations. |
| `assets/` | Asset definitions and build resources. |
| `scripts/` | Build scripts, source audits, and utilities. |
| `tools/` | Development and conversion tools. |
| `include/` | Shared headers. |
| `rsp/` | Nintendo 64 RSP-related components. |
| `Makefile` | Main project build configuration. |
| `README.md` | Main project documentation. |
| `CURRENT_DEVELOPMENT_STATUS.md` | Development overview. |
| `CREDITS.txt` | Credits and attribution. |
| `REFERENCES.txt` | Research and technical references. |
| `LICENSES_AND_NOTICES.txt` | Licensing and third-party notices. |

The repository is intended to contain the current project source and the materials necessary to understand and build it.

Historical development snapshots, temporary logs, backup copies, and superseded revision patches are not required as separate files in the main project directory.

---

# Emulator Compatibility

Development and testing have included Project64 with GLideN64.

Some visual issues may originate from the emulator or graphics plugin rather than the game's source code.

For certain GLideN64 black-screen or black-world symptoms, a potentially useful configuration is:

**Force depth buffer clear = ON**

When reporting graphics issues, include:

- Emulator name and version.
- Graphics plugin and version.
- Framebuffer-emulation settings.
- Relevant depth-buffer settings.
- The level and game mode.
- Whether the problem also occurs on original hardware or another emulator.

Source regressions and emulator-specific behavior should be investigated separately.

---

# Development Principles

GoldenEye 007 Plus follows several core development principles.

1. **Preserve the original game.** Maintain GoldenEye's established gameplay behavior wherever possible.

2. **Respect original source behavior.** Use the original GoldenEye implementation as the authoritative reference when investigating engine behavior.

3. **Maintain backport fidelity.** Adapt features from Perfect Dark without introducing unnecessary gameplay deviations.

4. **Prioritize correctness.** Performance improvements must not compromise collision, AI behavior, player movement, or other gameplay systems.

5. **Preserve working functionality.** Avoid unnecessary changes to stable systems while correcting unrelated problems.

6. **Test multiple configurations.** Consider Single-Player, Multiplayer, Co-Op, player counts, and original hardware.

7. **Preserve attribution.** Credit original developers, reverse engineers, modders, and third-party asset creators.

8. **Distinguish finished features from experimental work.** Static checks and successful compilation are not substitutes for gameplay testing.

9. **Avoid unnecessary development clutter.** The public repository should focus on the current working source, necessary assets, tools, and useful documentation.

---

# Credits and Acknowledgments

GoldenEye 007 Plus is made possible by the work of the original game's developers, decompilation contributors, reverse engineers, preservationists, modders, and the broader GoldenEye and Perfect Dark communities.

Important foundations and contributions include:

**Rare** — Original developers of GoldenEye 007 for Nintendo 64.

**GoldenEye 007 Decompilation Project** — Source reconstruction and research forming the foundation of this project.

Original project:

https://gitlab.com/kholdfuzion/goldeneye_src

**Perfect Dark Decompilation Project** — Source and technical reference for the adaptation of Perfect Dark-derived gameplay and Simulant features.

**Ryan Dwyer and other Perfect Dark decompilation contributors** — Important research and source-development contributions.

**Krijy** — Discovery of Citadel and early work making it playable.

**Zoinkity** — Citadel STAN and clipping restoration, additional GoldenEye research, and original work behind certain Level Modifiers.

**BMW** — Creator of Courtyard, including its pioneering use of Valve Hammer Editor for GoldenEye level creation.

**Goldfinger 64 development community** — Original creators of the Goldfinger 64 project and associated resources referenced or adapted by GoldenEye 007 Plus.

**GoldenEye and Perfect Dark modding communities** — Research, tools, discoveries, and preservation work.

**Project64 and GLideN64 contributors** — Emulator and graphics-plugin development supporting investigation and testing.

**OpenAI ChatGPT** — Development and research assistance.

Additional credits, resource provenance, and third-party notices are maintained in:

- `CREDITS.txt`
- `REFERENCES.txt`
- `LICENSES_AND_NOTICES.txt`

## Additional Mod Provenance

The Cradle Multiplayer Kill Plane Level Modifier was originally developed by **Zoinkity**.

GoldenEye 007 Plus ports its relevant vertical-boundary behavior while preserving the underlying multiplayer setup.

Additional historical GameShark-related Level Modifier provenance is documented in `CREDITS.txt`.

The project does not claim authorship of assets, code, tools, or research originating from third-party contributors.

All applicable licenses, attribution requirements, and redistribution restrictions must be respected.

---

# Development Status and Contributions

GoldenEye 007 Plus remains an ongoing community modification.

Contributions involving:

- Gameplay testing.
- Bug reports.
- Performance analysis.
- Original engine research.
- Nintendo 64 hardware compatibility.
- Source improvements.
- Documentation.
- Properly credited community content.

are valuable to the project's continued development.

Please include enough information in bug reports to reproduce the problem.

For example:

- Level or multiplayer map.
- Single-Player, Multiplayer, or Co-Op.
- Number of human players and Simulants.
- Enabled cheats and Level Modifiers.
- Emulator or original hardware.
- Steps leading to the issue.
- Expected behavior and actual behavior.

The project's current status is also documented in `CURRENT_DEVELOPMENT_STATUS.md`.

---

# Legal and Trademark Notice

GoldenEye 007, James Bond, Nintendo 64, Rare, Nintendo, MGM, EON Productions, and their respective characters, artwork, audio, names, and trademarks belong to their applicable rights holders.

GoldenEye 007 Plus is an unofficial fan and community project.

It is not affiliated with, endorsed by, or sponsored by Nintendo, Rare, MGM, EON Productions, or any other applicable rights holder.

No original retail GoldenEye 007 ROM is distributed by this repository.

Users must provide their own legally obtained game ROM for supported build and extraction processes.

**GoldenEye 007 Plus is a community modification created for preservation, research, experimentation, and appreciation of the original game.**
