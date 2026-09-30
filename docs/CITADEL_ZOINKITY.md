# Citadel multiplayer restoration (V90)

GoldenEye Plus V90 restores the unused Citadel stage as a multiplayer map.

The playable collision, multiplayer setup data and level-select portrait are
reconstructed from **Zoinkity's 2005 Citadel restoration** supplied with the
original NTSC/PAL IPS patches, `Citadel.bin`, and GameShark code package.
Zoinkity manually reclipped the level; GoldenEye Plus preserves that work and
ports it into the decompiled source/resource pipeline rather than running the
original RAM uploader or applying an IPS at runtime.

## What is retained from Zoinkity's work

- the hand-reclipped Citadel STAN/collision data (377 tile records, verified byte-for-byte against the clipping payload in `Citadel.bin`);
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
  reclip; Zoinkity's data uses polygons up to 13 points. The ordinary final-game
  STAN loader is used; no retail clipping resource or stage is substituted.
- V90 R5 explicitly ties the Citadel STAN and all split image bytes into the
  incremental build graph, so the exact reclip and portrait cannot remain stale
  inside `ob_seg.o` / `combined.bin` after source or asset changes.
- The generated STAN has the normal 12-byte file prefix followed by the exact
  14,164-byte Zoinkity collision block (`SHA-256`
  `1ae6a1a58f505fbdf43864ca546bb79168f02c5e8b62d61f36b99e720412dcc2`).
- The portrait artwork is reconstructed from the original IPS image payload.
  V90 R2 converted that artwork to the same 68x44 I8 runtime format used by the
  retail multiplayer portraits. V90 R3 gives it a dedicated appended image ID,
  preserving every retail image slot unchanged while using the decomp's
  mod-friendly image-extension path. V90 R5 vertically orients only this
  dedicated 68x44 asset for the final engine's level-select renderer; its size,
  format and additive image ID are unchanged. The historical patch's emulator/
  console image-slot tricks are deliberately not reproduced.
- Appended GoldenEye Plus obseg resources are emitted at the physical end of
  ob_seg in the same order as their appended resource-table IDs. This preserves
  all retail IDs while keeping GoldenEye's next-address resource-size calculation
  correct for Citadel and Map Maker assets.

## Attribution

Original Citadel reclip/restoration: **Zoinkity**.

GoldenEye Plus source integration: V90 (2026-09-27).

This file documents provenance only; it does not replace any upstream project
license or notices.
