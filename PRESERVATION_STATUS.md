# GoldenEye 007 Plus — Preservation Status & Recovery Plan

## Goal

Preserve every meaningful GoldenEye 007 Plus revision so that future users can:
- understand what changed;
- verify authenticity with hashes;
- recover exact source where possible;
- distinguish original artifacts from reconstructed revisions;
- avoid distributing retail ROM content improperly.

## Currently strong preservation areas

### R21
R21 is the best-preserved era. We currently have a dense chain of:
- community-source snapshots;
- test-build packages;
- RC-to-RC patches;
- WIP handoffs;
- cumulative addenda;
- test matrices;
- static-regression/audit bundles;
- SHA-256 manifests;
- special hotfix and A/B test packages;
- final full R21 source.

### R22
Current R22 checkpoints are also strongly preserved:
- Disable Knockback source/patch/ROM;
- Noise Dithering WIP/final source and patches;
- SP Watch scroller final source/patch/ROM;
- cumulative R22 addendum;
- GitHub-ready/clean source snapshots.

### R2 / R4 / R8 / R12
These have enough original source/patch/addendum/hash material to serve as strong early historical anchors.

## Highest-value missing artifacts to recover later

When attachment capacity returns or old local backups are found, prioritize:

1. **Exact original R1 package**
   - source snapshot
   - Standard ROM
   - Everything Unlocked ROM if one existed
   - patch/addendum/hash manifest

2. **Exact original R3 package**
   - current patch is preserved, but original full source/test-build package would improve authenticity.

3. **R5 full package**

4. **R6–R11 full source/test-build packages**
   - addenda/hashes exist for several of these, but complete original packages would be better.

5. **R13–R20 original release packages**
   - later source bundles preserve extensive history and may allow reconstruction, but original packaged artifacts remain preferable.

6. Any missing **R21 RC1 / RC8 source / RC11 source** packages if they exist separately.

## Reconstruction policy

When an original package is unavailable:

- never silently label a reconstructed build as the original;
- tag it as **RECONSTRUCTED**;
- record:
  - exact baseline used;
  - exact patch(es) applied;
  - toolchain/build command;
  - resulting hashes;
  - whether the reconstructed binary matches any historical hash;
- if hashes match a historical identity, the reconstruction can be considered byte-identical;
- if hashes do not match, preserve it as a reconstruction only.

## GitHub release policy

Recommended GitHub Releases should contain:
- source archive or source tag;
- xdelta/BPS patch where legally appropriate;
- release notes;
- `SHA256SUMS.txt`;
- build instructions;
- compatibility/test notes.

Do **not** upload retail-derived full ROMs to the public repository/releases unless distribution rights are established.

## Suggested tag scheme

Use a scheme that distinguishes historical mod revisions from RCs and modern Plus releases:

- `mod-r1`
- `mod-r2`
- ...
- `mod-r20`
- `r21-rc2`
- `r21-rc3`
- ...
- `r21-rc11-fb-clear-ab`
- `r21-final`
- `r22-knockback`
- `r22-noise-dither`
- `r22-watch-scroller`

Avoid inventing tags for revisions whose source identity is not sufficiently established.

## Integrity requirements

For each archived revision preserve:
- CRC1/CRC2 for N64 ROM builds;
- SHA-1;
- SHA-256;
- source ZIP SHA-256;
- patch SHA-256;
- build config;
- compressed-slot usage;
- PHYSICAL_CODE audit result;
- runtime-certification state.

## Emulator/plugin provenance

Rendering notes must include:
- emulator and build;
- graphics plugin;
- exact plugin revision when known;
- relevant plugin settings.

Current known test baseline includes:
- Project64 Dev 4.0.0.6712-1fa439e
- GLideN64 rev. `1f4d04f`

Current renderer lesson:
- Force depth buffer clear can mitigate black-world behavior;
- disabling Emulate framebuffer currently eliminates it;
- do not collapse this plugin behavior into the older confirmed room-lifetime/GC source bug.

## Next archive actions

1. Commit `RELEASE_INDEX.md`, `RELEASE_HISTORY.md`, and this file under `docs/releases/` or `docs/history/`.
2. Create a non-ROM `releases/` archival tree for old notes, hashes and patches.
3. Create Git tags only after exact source identity is confirmed.
4. Recover missing original packages when available.
5. Generate a master `SHA256SUMS.txt` for every preserved non-ROM archival artifact.
6. Keep a fresh cumulative addendum with every substantial future R22+ build.
