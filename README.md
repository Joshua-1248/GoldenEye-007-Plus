# GoldenEye 007 Plus

**GoldenEye 007 Plus** is a community modification of **GoldenEye 007 for Nintendo 64**, built upon the GoldenEye 007 decompilation. It expands the original game with performance optimizations, campaign Co-Op, Perfect Dark-derived Simulants, enhanced Multiplayer, Third-Person gameplay, optional gameplay fixes, restored content, cheats, and experimental features.

The aim is to preserve the original game's mechanics and presentation wherever possible while offering optional improvements. Backported Perfect Dark behavior is adapted to GoldenEye only where necessary.

**Source-only distribution:** No retail GoldenEye ROM or built game ROM is supplied. To build the project, provide your own legally obtained, unmodified original ROM. See [Building](#building-goldeneye-007-plus), [Credits](#credits-and-acknowledgments), and `LICENSES_AND_NOTICES.txt`.

> **Development status (October 2026):** This repository contains the current public R22-derived GoldenEye 007 Plus source with later R27/R30 integration. Experimental features and some recent gameplay fixes still need runtime verification. A successful compile or static audit is not proof that every feature works in-game.

## Contents

- [Feature overview](#feature-overview)
- [Special Options: access and complete menu guide](#special-options-access-and-complete-menu-guide)
- [Melee Quick-Swap](#melee-quick-swap)
- [Master Control Debug Menu](#master-control-debug-menu)
- [Cheats and one-shot actions](#cheats-and-one-shot-actions)
- [Level Modifiers](#level-modifiers)
- [Third-Person Options](#third-person-options)
- [Experimental features](#experimental-features)
- [Multiplayer and Simulants](#multiplayer-and-simulants)
- [Campaign Co-Op](#campaign-co-op)
- [Building](#building-goldeneye-007-plus)
- [Repository, development, credits, and legal notes](#repository-structure)

---

## Feature overview

- Physical-code execution and audited performance improvements for Nintendo 64.
- Single-Player and **1–4-human-player campaign Co-Op**, with enhanced objectives, statistics, deaths, and mission reports.
- Expanded Multiplayer maps, playable characters, duplicate-character selection, guns, dual-wielding, settings, and HUD presentation.
- Up to **eight configurable Perfect Dark-derived Simulants** in Multiplayer; experimental campaign companion Simulants.
- Optional Third-Person camera and related aiming, camera, and death-presentation controls.
- Extended **Special Options**, **Enhancements**, **Patches**, **Debug**, **Experimental**, **Level Modifiers**, and in-game watch configuration pages.
- **Melee Quick-Swap**, additional death animations, and other quality-of-life features.
- Restored **Master Control Debug Menu** access using a dedicated enabled/disabled option and controller combination.
- Modded and original GoldenEye cheats, including No Reload, Rapid Fire, Fly Mode, No-Clipping, Mirrored Levels, Super Tank, Super/Ultra Kinetics, and immediate mission actions.
- Dam restoration and a **Drivable Speedboat**; additional Facility, Silo, Surface, Bunker, Frigate, Citadel, and Multiplayer Level Modifiers.
- Experimental **Jumping**, **Move Anywhere**, and the optional **Map Maker** build.
- Main Menu Theme and Watch Menu Theme selection.

---

## Special Options: access and complete menu guide

**Special Options** is the principal location for GoldenEye 007 Plus gameplay settings. Open it from the front-end/main menu; its settings span multiple pages. The in-game watch exposes a corresponding Special Options page, with different presentation and some mode-specific differences in Single-Player, Multiplayer, and Co-Op.

Use the menu's normal selection controls to change a setting, select a category to enter its submenu, and back out when finished. Some changes are immediate, while saved settings are committed when leaving the relevant menu or submenu. Certain settings depend on the active save, game mode, or stage.

**Important:** This is the modified game's menu system (`MODDED_CHEATS=YES`), not a list of retail GoldenEye secret controller sequences. The exact menu positions may differ between the front-end and the watch.

### Standard Options

| Menu option | Function |
|---|---|
| Music Volume | Adjust music volume; the front-end displays the percentage. |
| FX Volume | Adjust sound effects volume; the front-end displays the percentage. |
| Control Style | Choose the control scheme. |
| Look Up/Down | Select standard or reversed vertical look. |
| Auto Aim | Enable or disable auto-aim assistance. |
| Aim Control | Select Hold or Toggle behavior. |
| Sight On Screen | Show or hide the aiming sight as configured. |
| Look Ahead | Control automatic view recentering. |
| Ammo On Screen | Display ammunition information. |
| Screen Size | Choose Full, Wide, or Cinema presentation. |
| Screen Ratio | Choose **4:3, 16:9, 21:9, or 32:9**. |

### Special Options gameplay settings

| Menu option | Function |
|---|---|
| Head Roll | Enable or disable the head-roll camera effect. |
| Endless Death Cam | Keep the death camera active instead of ending the view at the normal limit. |
| Real-Time Collapse | Allow relevant death/collapse animation behavior to proceed in real time. |
| Damage Hitstun | When disabled, allows firing even while taking damage. |
| Damage Knockback | Enable or disable movement knockback when hit. |
| Damage Flash | Enable or disable the white screen flash caused by damage. |
| Crosshair | Show or hide the crosshair. |
| Taking Damage Sound | Enable or disable the sound played when the player takes damage. |
| Third Person | Enable Third-Person presentation where the mode permits it. |
| Enemy Bullet Holes | Enable additional bullet-hole effects on supported enemy surfaces. |
| **Melee Quick-Swap** | Enable the Slappers/Sniper Rifle Buttstock shortcut; see the complete instructions below. |
| Watch Menu Theme | Select the in-game watch's musical theme. |
| Main Menu Theme | Select the front-end main menu's musical theme. |

The Special Options interface also links to the categories below. Their order and access method may vary between front-end and watch menus.

### Special Options categories

| Category | What it contains |
|---|---|
| **Level Modifiers** | Stage-by-stage restored content and gameplay changes, including the Dam boat and island collision modifier. |
| **Patches** | Optional optimizations, rendering choices, compatibility fixes, and gameplay corrections. |
| **Third-Person Options** | Camera position, crosshair, crouch height, shoulder, and death-camera settings. |
| **Single-Player Options** | Character Modifier and Match View Height. |
| **Debug** | **Master Control Debug Menu** On/Off setting. |
| **Experimental** | **Jumping (Press L)** and **Move Anywhere**. |
| **Enhancements** | Additional player death animations, Always Show Crosshair, and Disable Body Armor. |
| **In-Game Cheats** | Toggle cheats and immediate actions where supported. |
| **Map Maker** | Experimental editor, only if the game was compiled with `MAP_MAKER=YES`. |

### Enhancements: every currently named entry

| Option | Description |
|---|---|
| **Additional Death Animations For Player** | Opens additional death animation choices for the player. |
| **Facility Gas Leak Death** | Enables the associated player death animation/behavior. |
| **Staggering Backwards Death** | Enables the additional staggering-backward death behavior. |
| **Always Show Crosshair** | Keeps the supported crosshair displayed rather than relying solely on its normal context. |
| **Disable Body Armor** | Disables supported body armor behavior. |

The two death-animation options are grouped under **Additional Death Animations For Player** where that submenu is present. They are optional and should be tested under the intended game mode.

### Patches: every currently named entry

| Option | Description |
|---|---|
| **Micro-Optimizations** | Enable or disable the supported runtime micro-optimization paths. |
| **Noise Dithering** | Toggle the relevant display dithering behavior. |
| **Anti-Aliasing** | Toggle the supported anti-aliasing presentation setting. |
| **TP Corner Shooting Fix** | Optional Third-Person corner-shooting correction. |
| **Unlimited Explosions** | Removes the original limit on simultaneously active explosions where supported. |
| **Silo X Music Loop Fix** | Optional correction for the Silo X music-loop behavior. |
| **AR33 Prop Fix (MP)** | Optional AR33 prop-related correction in Multiplayer. |

**Patches are selectable options, not a promise that every historical or emulator-related issue is fixed.** If an option is experimental or untested on your configuration, verify it during gameplay.

### Single-Player Options

| Option | Description |
|---|---|
| **Character Modifier** | Override the Single-Player character selection with a supported character. |
| **Match View Height** | Adjust the Single-Player viewpoint height to match the selected character where supported. |

### Main Menu and Watch Menu themes

Main Menu Theme and Watch Menu Theme are independent selectors. The supported track choices include **GoldenEye 007**, **Goldfinger 64**, **Tomorrow Never Dies 64**, and **The Spy Who Loved Me 64**. Theme changes are saved in the supported Plus save settings.

---

## Melee Quick-Swap

**Melee Quick-Swap is a gameplay option, not a cheat. It is Off by default.** It lets the player switch the presentation of GoldenEye's melee/Fist slot between **Slappers** and **Sniper Rifle Buttstock**.

### Enable it

Open **Special Options**, find **Melee Quick-Swap**, and set it to **On**. It is available through the corresponding Special Options page of the in-game watch in supported modes. The selection is save-backed where supported.

### Use it during gameplay

1. Equip the **Slappers/Fists** melee slot.
2. **Hold B first.**
3. While continuing to hold B, **press Z** (a fresh Z press).
4. The game switches between the Slappers and Sniper Rifle Buttstock presentation when the alternate is available.

**The Sniper Rifle must be owned to select its buttstock.** If it is not available, the game falls back to Slappers/Fists rather than granting a free sniper-rifle melee style. The shortcut applies to the melee slot; it does **not** cycle arbitrary guns.

The swap waits until the melee action is idle: it does not interrupt an active punch or buttstock strike. It uses the game's existing weapon lower/swap/raise transition, suppresses the normal fire/action side effects of the shortcut, and tracks the player's explicit melee choice independently for each player. Normal automatic melee selection still operates until the player makes an explicit choice. If the Sniper Rifle becomes unavailable, the style falls back safely to Fists.

**In short: turn on Melee Quick-Swap, equip Slappers, hold B, then tap Z.**

---

## Master Control Debug Menu

GoldenEye 007 Plus restores access to the original game's **Master Control Debug Menu (MCM)**, an internal development-oriented debug interface. It is **Off by default** and is not normally opened by the retail controller combination in modded builds.

### Enable it

In the front-end, open **Special Options → Debug → Master Control Debug Menu** and set it to **On**. The Debug page is also available in supported Single-Player and Multiplayer/Co-Op watch menus. Its value is saved through the extended save settings.

### Open it during gameplay

On **Controller 1**, press and hold **L + R + D-Pad Up + C-Up** together. The game requires the entire combination to be down and at least one newly pressed button edge. This shortcut is available only after enabling the menu and entering a level; it does not work from the title screen.

### Controls

| Button | Function |
|---|---|
| **D-Pad Up / Down** | Navigate the debug entries. |
| **D-Pad Left / Right** | Move or adjust supported selections. |
| **A** | Activate the highlighted debug command. |
| **Start** | Close the debug menu and restore regular gameplay controls. |

The code retains the original debug-menu initialization and functionality, including development-oriented view and STAN tools. Internal debug commands can interrupt normal gameplay or put the level in an unintended state. Save progress before experimenting. To prevent further opening, return to **Special Options → Debug** and set **Master Control Debug Menu** to **Off**.

---

## Cheats and one-shot actions

Use the front-end **Cheats** screen or the watch's **In-Game Cheats** page where available. The front-end may present multiple pages. Some original cheats still follow their normal unlock conditions. The mod adds menu-accessible cheats and development choices; enabling those does not require typing a secret controller code.

**Toggle cheats** remain enabled until disabled or cleared by a game/reset path. **One-shot actions** execute immediately. Some internal/debug cheats exist for preservation and testing, and their exact side effects can depend on game mode.

### Added and expanded cheat toggles

| Cheat | Effect |
|---|---|
| **No Reload** | Removes the normal requirement to reload. Independent of Infinite Ammo. |
| **Rapid Fire** | Reduces or relaxes the firing interval to allow faster shots. |
| **No-Clipping** | Bypasses ordinary player collision restrictions, permitting traversal through solid geometry; moving beyond valid rooms may cause problems. |
| **Fly Mode** | Enables flight-style movement, with vertical movement related to the view direction. |
| **Super Kinetics** | Applies a **1.5×** explosion-related launch/death impulse; does not boost damage or explosion radius. |
| **Ultra Kinetics** | Applies a **3.5×** explosion-related launch/death impulse. When enabled with Super Kinetics, their combined multiplier is **5.0×**. |
| **Super Tank** | Increases the tank's forward/reverse top speed and turning response by **2.5×**. |
| **Mirrored Levels** | Mirrors supported stage geometry, STAN collision, characters, props, and doors along the world X axis. In-level transitions remain experimental. |
| **Level Never Ends** | Blocks supported normal end-of-stage triggers. |
| **Freeze Timer** | Stops the supported mission countdown from decreasing; it does not freeze every internal clock or all gameplay. |
| **Max Head-On** | Repeats/extends tank head-on guard impact and crush audio feedback for a limited period while retaining the original initial impact. |
| **Infinite Ammo** | Prevents normal ammo depletion; it does not automatically disable reload animations. |
| **Invincibility** | Protects Bond from normal damage. |
| **Invisibility** | Uses GoldenEye's normal enemy-visibility cheat behavior. |
| **All Guns** | Makes the game's weapons available in supported modes. |
| **DK Mode** | Uses the classic oversized-head appearance. |
| **Tiny Bond** | Makes Bond unusually small. |
| **Paintball Mode** | Changes bullet-impact markings. |
| **Turbo Mode** | Increases Bond's movement speed, separately from Super Tank. |
| **No Radar (MP)** | Removes the normal Multiplayer radar information. |
| **All MP Characters** | Makes additional Multiplayer character choices available where supported. |
| **Invisible [Multi]** | Uses the internal Multiplayer invisibility/phase behavior. |

### Other preserved cheat and debug entries

The source also includes **Max Ammo**, **2x Health**, **2x Armor**, **10x Health**, **Extra Weapons**, **Deactivate Invincibility**, **Debug Position**, and **Debug Unk5**. These are preserved original/internal/development entries where exposed. **Debug Unk5 is an unknown original debug entry; no unverified gameplay effect is claimed here.** Their availability and exact outcome may depend on the selected build, current level, or game mode.

### Immediate action cheats (not On/Off toggles)

| Action | Effect |
|---|---|
| **Full Health** | Restore Bond's health immediately. |
| **Full Armor** | Restore body armor immediately. |
| **Normal Animation** | Restore the normal animation rate following Fast/Slow Animation changes. |
| **Complete Objectives** | Mark the applicable mission objectives complete; updates the shared Co-Op objectives display where applicable. |
| **Complete Mission** | Complete the campaign mission through the supported success/exit route; resets the time used by target-time reward processing. |

Cheats can affect save progression, mission awards, difficulty, or the ability to finish a mission normally. Consider a separate save while experimenting.

---

## Level Modifiers

Go to **Special Options → Level Modifiers**, select **Single-Player**, **Multiplayer**, or **Miscellaneous**, then choose a level and modifier. Where supported, the current level's entries are also available from the in-game watch.

- **Reversible options** normally show **On/Off** and can be changed as their implementation allows.
- **Latched options** show **ACTIVATE**, then **ACTIVE** until the current level ends/reloads; they cannot be deactivated mid-level.
- Some stage changes are intended for **preload**, so selecting the option before entering the level is safest. Certain explicitly runtime-capable modifiers can be activated during the stage.

### Single-Player Level Modifiers

| Stage | Modifier | Description |
|---|---|---|
| **Dam** | Restore Doors Near Dam Docks | Restore the additional doors near the docks. |
| **Dam** | Restore Speedboat | Restore the static/historical speedboat feature; distinct from the drivable boat. |
| **Dam** | **Drivable Speedboat** | Make a controllable speedboat available (latched activation). |
| **Dam** | **Dam Island Fixed Collisions** | Apply the island's corrected collision behavior (latched activation, designed not to require a restart). |
| **Facility** | Walk Through Grate In Vents | Allow the supported vent-grate traversal behavior. |
| **Facility** | Restore Beta Facility Doors | Restore the selected beta doors. |
| **Facility** | Restore Beta Facility Stacked Tanks | Restore the stacked tanks. |
| **Surface** | White Sky | Select the alternate white-sky presentation. |
| **Surface** | Beta Doors | Restore the beta doors. |
| **Bunker** | Beta Doors | Restore the selected beta doors. |
| **Silo** | Beta Vent Start | Start in the ventilation-shaft configuration. |
| **Frigate** | "Keep Clear" Doors Leading Into Engine Room | Restore the selected engine-room entry doors. |
| **Frigate** | Restore Removed Doors | Restore the removed doors. |
| **Frigate** | Clipping Change In Room With 3 Pipes | Apply the relevant clipping modification in the three-pipe room. |

### Multiplayer and Miscellaneous Level Modifiers

| Mode/stage | Modifier | Description |
|---|---|---|
| Multiplayer: Temple | Fall Down Into Hole | Enable falling into the supported hole. |
| Multiplayer: Temple | Better Respawning | Apply the alternate respawn behavior. |
| Multiplayer: Complex | Fall Down Into Holes | Enable the supported hole traversal. |
| Multiplayer: Cradle | Kill Plane | Apply the original community kill-plane behavior, credited to Zoinkity. |
| Miscellaneous: Citadel | Water | Enable the Citadel water-related level modifier. |

The list above reflects the named entries in `src/game/levelmodifiers.c`. More modifiers may be added in future versions.

### Drivable Speedboat controls

Choose **Level Modifiers → Single-Player → Dam → Drivable Speedboat**. Activate before entry or choose **ACTIVATE** while playing Dam. Near the speedboat, **press B to board**, then **press B again to exit**. The modifier stays ACTIVE for the remainder of the level. Deck movement, shore transitions, boat collision, and performance are actively being refined. The **Restore Speedboat** entry is a separate feature.

### Dam Island Fixed Collisions

This option is intended to take effect immediately while playing Dam, like the drivable-boat activation, and to stay active until the level ends. It is not a generic collision-off cheat and does not replace the game's normal STAN system.

---

## Third-Person Options

Enable **Third Person** from supported gameplay/Special Options controls, then open **Third-Person Options** from Special Options (or its watch submenu) to configure the view.

| Option | Description |
|---|---|
| **Stay In TP On Death** | Retain supported Third-Person presentation through the death sequence. |
| **TP Crouch Cam** | Enable crouch-related camera adjustment. |
| **Directional Shoulder** | Change the directional shoulder behavior. |
| **TP Sight Translucency** | Control translucency of the Third-Person sight. |
| **TP Cam Distance** | Set the camera's distance. |
| **TP Cam Height** | Set the camera's vertical position. |
| **TP Cam Horizontal** | Set the lateral offset. |
| **TP Cam Down Frame** | Adjust downward framing. |
| **TP Crouched Cam Height** | Set the camera height when crouched. |
| **TP Crosshair Range** | Set the crosshair range. |
| **TP World-Space Crosshair** | Control the world-space crosshair representation. |

Third-Person implementation also includes weapon/projectile alignment, cutscene transitions, death presentation, and camera collision work. A Multiplayer **Lock Viewport** setting may restrict changing view modes during a match. Some character or stage-specific camera situations remain subject to testing.

---

## Experimental features

| Feature | Activation | Notes |
|---|---|---|
| **Jumping (Press L)** | **Special Options → Experimental**, enable **Jumping (Press L)**, then press **L** in gameplay. | Extra jumping movement; landing and collision handling remain experimental. |
| **Move Anywhere** | **Special Options → Experimental → Move Anywhere**. | Broadens movement outside the normal STAN traversal restrictions. |
| **Map Maker** | Compile with **`MAP_MAKER=YES`**, then use the Map Maker menu. | Experimental Basic/Advanced editor; excluded from standard gameplay builds. |

**Move Anywhere** has received improvements in floor-triangle selection, floor seams, and room/portal geometry handling. Known work remains for stairs, steep slopes, props, ladders, Fly Mode/No-Clipping interactions, and moving-platform support. Do not treat it as a finished general-purpose collision replacement.

**Map Maker** includes experimental geometry/mesh editing, textures, entities/pickups, dedicated stage setup, and test features. Use `MAP_MAKER=NO` for the standard build unless intentionally developing the editor.

---

## Multiplayer and Simulants

GoldenEye 007 Plus expands the original Multiplayer configuration, weapons, characters, and maps. The game supports duplicate-character selection, more arena choices, additional player options, more weapon handling, and even a single human in Multiplayer.

### Perfect Dark-derived Simulants

The mod backports/adapts an AI participant system inspired by **Perfect Dark**. In supported Multiplayer setups, **up to eight Simulants** can be configured. Visit the **Multiplayer → Bots** configuration before launching a match and adjust available bot slots, character selections, difficulty, team, and personality/trait settings.

Current development covers combat targeting, weapon selection/pickup, ammunition and reloads, armor, radar participation, scoring, movement, death handling, and respawning. Bot navigation uses a separate **STAN-based navigation architecture** rather than simply reusing the original guard waypoints.

Six difficulty levels, including **Very Easy through Extreme**, and combinable bot traits are part of the expanded design. **The bots remain experimental**: navigation, doorways, animation, multiple-player performance, respawn behavior, and crashes require runtime testing. No complete Perfect Dark behavioral parity is claimed.

Primary implementation: [`src/game/mpbots.c`](src/game/mpbots.c) and [`src/game/mpbots.h`](src/game/mpbots.h). Integration audits live under `scripts/`.

### Multiplayer modes and settings

Features include more character/map selections, **All Guns**, dual-wield support, improved split-screen HUDs, bot/player team assignment, optional Friendly Fire, and expanded watch and player settings. Settings may apply to an individual player or the entire match.

### Citadel and Courtyard

**Citadel** was first discovered/made playable by **Krijy**, whose earlier implementation used Cradle's setup. **Zoinkity** later reworked the STAN/clipping information for compatibility with the final GoldenEye engine. GoldenEye 007 Plus integrates Citadel as its own Multiplayer stage.

**Courtyard** was created by **BMW** and is credited as the first GoldenEye level created using Valve Hammer Editor. The project preserves that original authorship.

---

## Campaign Co-Op

The project expands the GoldenEye campaign to **one to four human players**, with ongoing experimental AI companion support.

- Shared mission objectives and mission-item handling.
- Improved escort, guard-targeting, and friendly-fire behavior.
- Expanded player-death, mission-failure, GAME OVER, and end-cutscene handling.
- Split-screen camera and player lifecycle fixes.
- Co-Op mission report and hit-location statistics.
- Experimental companion Simulant movement, combat, following, and doors.

The expanded mission report includes time, deaths, shots, kills, accuracy, head/body/limb/other hit counts and percentages, and **Weapon of Choice**, with player-specific figures where available. Co-Op avoids incorrectly invoking the standard Single-Player death path for every participant.

The campaign and Simulant systems are still being refined; some multiplayer player counts, cutscenes, mission logic, and bot interactions need further tests.

---

## Optimization and engine improvements

Performance work covers character/guard processing, animation and model rendering, visibility/fog, explosion processing, collision and movement, multiplayer/Co-Op workloads, asset compression, memory, and physical-code execution. Changes are intended to be behavior-preserving rather than shortcuts that change GoldenEye's rules.

Testing has included **Project64** with **GLideN64**. Some apparent graphics glitches are emulator/plugin behaviors rather than source regressions. For certain black-screen/black-world issues, **Force depth buffer clear = ON** may help. Include emulator/plugin version and configuration in bug reports; compare against unmodified GoldenEye behavior when possible.

---

## Building GoldenEye 007 Plus

This is a **source-only project**. Provide a legally obtained, unmodified **NTSC-U GoldenEye 007** ROM in the project root named `baserom.u.z64`. The ROM and generated `.z64` files are **not distributed** here. The physical-code build requires the appropriate **N64 Expansion Pak / 8 MiB RDRAM** environment.

### Standard gameplay configuration (Linux)

The established development configuration uses **EEPROM16**, LLVM, the physical-code fastpaths, modded cheats, **Map Maker disabled**, and **four Make jobs**. Recent local development builds support Turtledeflate compression through environment variables:

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

Replace `/absolute/path/to/turtledeflate_app` with your installed compressor's executable path. **The Turtledeflate variables apply only to source revisions with that integration**; older published trees may ignore them. Where Turtledeflate support is absent, use the ordinary build flags without the environment variables:

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

`-j4` sets up to four concurrent Make jobs; `GE_CDATA_TURTLE_WORKERS=4` sets the compressor's worker count where implemented. `MAP_MAKER=NO` is deliberate for the standard game. The usual gameplay build directory is `build/u-phys-opt-mod-eep16-nomap/`.

The supplied `build_geplus.sh` helper may select **`MAP_MAKER=YES`** and a different number of jobs. Review that helper before using it: it is not necessarily equivalent to the standard command shown above. To develop the editor intentionally, compile with `MAP_MAKER=YES`.

### Build configuration reference

| Variable | Purpose |
|---|---|
| `VERSION=US` | Select NTSC-U. |
| `PHYSICAL_CODE=YES` | Use the physical-code architecture. |
| `PHYSICAL_FASTPATHS=YES` | Enable supported physical fastpaths. |
| `MODDED_CHEATS=YES` | Compile the Plus gameplay and cheat features. |
| `MAP_MAKER=NO` | Exclude the experimental editor. |
| `OPT_USE_LLVM=YES` | Use the supported LLVM toolchain route. |
| `COMPARE=0` | Do not compare the modified ROM against the retail ROM hash. |
| `-j4` | Four concurrent Make jobs. |
| `GE_CDATA_TURTLE_WORKERS=4` | Four compression workers on compatible source trees. |

No `make clean` is required just to perform an incremental build.

---

## Repository structure

| Path | Contents |
|---|---|
| `src/`, `src/game/` | Engine, gameplay, menus, player state, AI, and collision code. |
| `src/game/mpbots.c`, `src/game/mpbots.h` | Simulant implementation and declarations. |
| `assets/` | Required asset definitions and build-time resources. |
| `scripts/` | Build/extraction utilities and source/physical audits. |
| `tools/` | Native development, conversion, and build tools. |
| `include/`, `ld/`, `rsp/` | Headers, linker-related material, and RSP source. |
| `Makefile`, `build_geplus.sh` | Build entry points. |
| `README.md`, `CURRENT_DEVELOPMENT_STATUS.md` | User instructions and development status. |
| `CREDITS.txt`, `REFERENCES.txt`, `LICENSES_AND_NOTICES.txt` | Attribution, research references, and licenses/notices. |
| `docs/` | Additional focused project documentation and contributor provenance. |

The public repository should focus on current source, necessary assets/build tools, useful documentation, and attribution. Numerous old `R##` patches, dated addendums, temporary logs, backups, and duplicate root-level C copies are **historical workbench artifacts**, not independent current releases. Retain history in Git and local archives instead of cluttering the public project tree.

## Development principles

1. **Preserve the original game:** original GoldenEye behavior is the authoritative baseline for original systems.
2. **Respect backport fidelity:** match Perfect Dark behavior where it is intentionally backported, with only necessary engine adaptations.
3. **Preserve stable functionality:** make targeted corrections rather than unrelated code changes.
4. **Do not sacrifice correctness for speed:** optimizations must not introduce movement, collision, rendering, or AI regressions.
5. **Test all relevant modes:** Single-Player, Multiplayer, Co-Op, different player counts, emulators, and N64 hardware.
6. **Treat experimental features as experimental:** a passing static audit or successful compile is not full runtime verification.
7. **Preserve proper attribution and licensing:** credit creators of third-party code, levels, assets, and research.

## Bug reports and testing

Include the level/map, game mode, player count, Simulants, active cheats and modifiers, exact action that failed, expected versus actual behavior, emulator/hardware and plugin version, and relevant build configuration. Investigate whether the behavior exists in retail GoldenEye before attributing every visual symptom to this mod.

---

## Credits and acknowledgments

GoldenEye 007 Plus is possible because of original developers, reverse engineers, preservationists, community researchers, tool authors, modders, and testers.

- **Rare** — Original development team of GoldenEye 007 for Nintendo 64.
- **GoldenEye 007 decompilation project and contributors** — Foundational source reconstruction and research: <https://gitlab.com/kholdfuzion/goldeneye_src>.
- **Perfect Dark decompilation project and contributors, including Ryan Dwyer** — Essential reference for the adapted Simulant and Perfect Dark-derived systems.
- **Krijy** — Discovery and early playable restoration of Citadel.
- **Zoinkity** — Citadel STAN/clipping reconstruction and original community gameplay/modifier work, including the Cradle (MP) kill-plane behavior.
- **BMW** — Creator of Courtyard and its pioneering Hammer Editor workflow.
- **Goldfinger 64 developers and contributors** — Authorship of Goldfinger 64 content referenced or adapted by this project. See `docs/GOLDFINGER64_HALL_OF_FAMERS_PROVENANCE.md` and the source notices.
- **GoldenEye and Perfect Dark community contributors** — Technical research, preservation, tooling, testing, and modding discoveries.
- **Project64 and GLideN64 teams** — Emulator and graphics-plugin development enabling testing and investigation.
- **OpenAI ChatGPT** — Development and research assistance.

**Additional mod provenance:** The **Cradle (MP) Kill Plane** was originally developed by **Zoinkity**; GoldenEye 007 Plus ports its relevant Y-boundary behavior while preserving the stage's underlying Multiplayer setup. Additional historic GameShark-related modifier provenance is documented in `CREDITS.txt`.

For fuller attribution and applicable rights, consult **`CREDITS.txt`**, **`REFERENCES.txt`**, and **`LICENSES_AND_NOTICES.txt`**. This README does not relicense third-party material or claim ownership of community-created content. All applicable redistribution requirements remain in force.

## Legal and trademark notice

GoldenEye 007, James Bond, Nintendo 64, Rare, Nintendo, MGM, EON Productions, their characters, artwork, music, names, and trademarks belong to their respective rights holders.

GoldenEye 007 Plus is an **unofficial fan/community project** and is not affiliated with, sponsored by, or endorsed by those rights holders. It is intended for preservation, research, experimentation, and appreciation of the original game.

**No retail GoldenEye 007 ROM is included or distributed.** Users must supply their own legally obtained original game ROM for supported build/extraction operations.
