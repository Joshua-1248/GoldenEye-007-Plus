# GoldenEye 007 Plus — Release History

This is the public-facing chronological history reconstructed from the preserved addenda, source snapshots, patches, test builds and handoffs currently available.

## R1
Established the modded branch's paged Cheat Options. Normal named cheats remained on Page 1; hidden/unused entries were exposed on Page 2 with Next/Previous navigation.

## R2
Major frontend and multiplayer expansion:
- added `4. OPTIONS`;
- improved unused-cheat labeling;
- paged multiplayer level selection;
- added Statue and Cradle;
- enabled Bunker, Archives, Caverns and Egyptian for 3–4 players;
- removed duplicate-character restriction;
- allowed backing out of MP setup submenus;
- enabled All Guns and dual wield in multiplayer;
- added initial 3–4 player dual-wield ammo HUD support.

## R3–R5
Iterative stabilization and presentation work:
- Statue/Cradle eligibility and presentation;
- main-menu spacing and Options polish;
- dual-wield HUD/camera/animation corrections;
- persistent weapon texture-cache safety;
- progressive frontend cleanup and slider behavior.

## R6
Introduced the first runtime-test campaign Co-Op path and one-player multiplayer. Co-Op routed through campaign Mission Select while retaining multiplayer player construction.

## R7
Corrected a core mode-selection mistake: setup/memory behavior now follows the actual mode rather than assuming `playerCount >= 2` means competitive multiplayer.

## R8
Added dedicated Co-Op dynamic graphics memory, repartitioned Expansion Pak space, hardened intro state handling, and preserved normal multiplayer isolation.

## R9
Used a Project64 RAM dump to identify a real first-frame Co-Op fault: uninitialized player collision bounds entering STAN collision. Added Co-Op collision-bounds bootstrap before gameplay ticks.

## R10–R12
Continued Co-Op correctness and Perfect Dark-inspired architecture:
- player/AI targeting improvements;
- simulation-vs-viewport tick separation;
- dropped weapon simulation correction;
- switch/model-instance state correction;
- renderer-state isolation experiments;
- ongoing background investigation.

## R13–R20
Cumulative expansion and stabilization:
- larger Co-Op lifecycle work;
- objectives/reporting/UI improvements;
- tank work for Runway and Streets;
- B+Z mission-item quick switch;
- expanded MP menus/options;
- new death/presentation options;
- hard compressed-C slot overflow protection;
- Silver/Gold PP7 third-person material preservation;
- gradual movement toward Perfect Dark-style Co-Op lifecycle semantics.

R20 became the last fully packaged checkpoint before the long R21 release-candidate cycle.

# R21 release-candidate era

## R21 / RC2 / RC3 / RC4
Completed broad source integration for:
- Co-Op death/reference cleanup;
- terminal failure routing;
- P1-owned shared cinematics;
- mission-result snapshot/report routing;
- shared essential mission items;
- objective HUD messaging;
- Co-Op watch rewrite;
- MP Settings and per-player options;
- expanded aspect-ratio handling;
- 64-entry Extra MP Characters support.

The RC3/RC4 period also fixed several concrete crashes:
- malformed `getStringID()` ternary usage;
- prop-only inventory weapon-cycle hang;
- 3P/4P pickup-message uninitialized buffer;
- invalid competitive-score scenario trap.

## RC5
Focused on single-player death-state correctness and the new Watch Special/Features page:
- Endless Death Cam one-shot music behavior;
- Real-Time Collapse timing separation;
- Game Over exit controls;
- new SP Watch Special Options / Cheats integration.

## RC5 GC fix
Critical historical checkpoint. A genuine 3P/4P split-screen world-render regression was traced to room garbage collection freeing room geometry while another player's already-built display list could still reference it. The fix constrained that reclamation path appropriately.

## RC6–RC8
Further Co-Op/menu/death/render stabilization, including packaging, test-matrix and feature refinements.

## RC9
Important cleanup and regression pass:
- Perfect Dark-style current-player viewport bounds audit;
- Rapid Fire rewritten to match a proven timer manipulation;
- Endless Death Cam explicit A/B/Z/START exit;
- order-sensitive B-first/Z-second mission-item switch;
- six-page SP Watch remap;
- terminal Co-Op GAME OVER freeze behavior.

## RC10
Multiple rendering and UI alignment checkpoints were preserved. This period also refined understanding of code-side render bugs versus GLideN64 false positives.

## RC11 FB-clear A/B
Dedicated framebuffer-clear A/B test package preserved for renderer diagnosis.

# R21 Final

R21 Final established the public identity **GoldenEye 007 Plus**.

Major final-state features included:
- `4. OPTIONS` two-page frontend;
- per-save `Unlock Everything`;
- six-page single-player Watch;
- SP and MP/Co-Op Special Options and in-game Cheats;
- MP Watch Settings hub;
- per-player MP options;
- expanded multiplayer and 1–4P Co-Op;
- selective Perfect Dark-derived lifecycle behavior;
- dual-wield MP work;
- Rapid Fire and No Reload;
- B+Z mission-item switching;
- end-cinematic and report routing work;
- UI/stat alignment cleanup.

Final public R21 identity:
- CRC1/CRC2: `450D5CA9-173182EB`
- SHA-1: `6072523d963b3db7c272909c30565e40fc41cd4b`
- SHA-256: `16e13b788343f43176bf249f56d7e6d72bc87bbca9801da7ebd9a8367dc55e89`

# R22

## Disable Knockback
Added a save-backed `Disable Knockback` option, default Off, available in all appropriate interfaces. It suppresses damage-caused player push while preserving damage and NPC knockback.

## Disable Noise Dithering
Added a save-backed VI dither-filter option, default Off. It controls the N64 VI output dither filter without globally disabling RDP local color dithering.

## SP Watch Special Options scroller
Expanded the Special Options page beyond the visible Watch area using a seven-row scrolling window and corrected row/index handling for Taking Damage Sound and In-Game Cheats.

## Current engineering constraint
The current R22 compressed C/data slot is effectively/full exactly at **72,704 bytes**, so further C work should reclaim space before adding substantial new code.

# Renderer compatibility chronology

Two distinct classes must remain separate in historical documentation:

1. **Confirmed source regression:** split-screen room-lifetime/GC could invalidate geometry still referenced by another viewport.
2. **GLideN64-side framebuffer/depth behavior:** later black-world symptoms could depend on graphics-plugin configuration.

Current testing on GLideN64 rev. `1f4d04f` shows:
- Force depth buffer clear helps;
- disabling Emulate framebuffer eliminates the currently observed corruption.

Future reports should always record emulator, GLideN64 revision, framebuffer-emulation state, and Force depth buffer clear state.
