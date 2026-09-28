# Citadel multiplayer restoration (V90)


## Earlier Citadel discovery and playable release

**Krijy** is credited with first discovering Citadel and making it playable for the wider GoldenEye community. The early playable implementation reused Cradle's setup because Cradle keeps all of its rooms loaded at once, which made it a practical host setup for exposing the unfinished Citadel geometry.

Zoinkity's later restoration/reclip work built on that community discovery and addressed Citadel's obsolete collision/STAN data for the final GoldenEye engine.

GoldenEye Plus V90 restores the unused Citadel stage as a multiplayer map.

The playable collision, multiplayer setup data and level-select portrait are
reconstructed from **Zoinkity's 2005 Citadel restoration** supplied with the
original NTSC/PAL IPS patches, `Citadel.bin`, and GameShark code package.
Zoinkity manually reclipped the level; GoldenEye Plus preserves that work and
ports it into the decompiled source/resource pipeline rather than running the
original RAM uploader or applying an IPS at runtime.

## What is retained from Zoinkity's work

- the hand-reclipped Citadel STAN/collision data;
- the Citadel multiplayer setup payload;
- the runtime multiplayer intro/start-pad data embedded in `Citadel.bin`;
- the Citadel multiplayer level-select portrait;
- the corrected Citadel level scale (`0.5763920546`, represented as
  `0.57639205` in the source level table).

## GoldenEye Plus integration

- Citadel is exposed only in regular multiplayer for V90.
- The existing retail Citadel background and Citadel text/music identities are
  retained.
- `Ump_setupcatZ.bin` is loaded by the ordinary setup resource system.
- The original uploader's absolute pointer redirects are replaced by a
  source-level Citadel MP handoff which resolves STAN links by their tile names.
- STAN traversal now supports the 11-15 point record sizes needed by the
  reclip; Zoinkity's data uses polygons up to 13 points.
- The portrait artwork is reconstructed from the original IPS image payload.
  V90 R2 converted that artwork to the same 68x44 I8 runtime format used by the
  retail multiplayer portraits. V90 R3 gives it a dedicated appended image ID,
  preserving every retail image slot unchanged while using the decomp's
  mod-friendly image-extension path.
- Appended GoldenEye Plus obseg resources are emitted at the physical end of
  ob_seg in the same order as their appended resource-table IDs. This preserves
  all retail IDs while keeping GoldenEye's next-address resource-size calculation
  correct for Citadel and Map Maker assets.

## Attribution

Original Citadel reclip/restoration: **Zoinkity**.

GoldenEye Plus source integration: V90 (2026-09-27).

This file documents provenance only; it does not replace any upstream project
license or notices.
