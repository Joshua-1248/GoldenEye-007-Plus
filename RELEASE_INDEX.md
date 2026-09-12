# GoldenEye 007 Plus — Release Preservation Index

**Prepared:** 2026-09-12  
**Scope:** GoldenEye 007 Plus / earlier modded physical branch material currently available in this conversation.

## Preservation status legend

- **Preserved** — original source/test/release artifact is currently available.
- **Reconstructable** — exact patch and a compatible known baseline/source are available, or a later source bundle contains enough historical material to reconstruct the revision.
- **Documented** — addendum/history/hashes exist, but an exact original release package is not currently confirmed.
- **Partial** — some original artifacts exist, but not a complete source/ROM/docs set.
- **Missing** — no sufficiently complete artifact has been identified yet.

## Mainline / early modded revisions

| Revision | Status | Currently preserved / known |
|---|---|---|
| R1 | Documented | Historical addendum records paged Cheat Options, hidden/unused cheat Page 2, and Next/Previous navigation. Exact original R1 package not currently confirmed. |
| R2 | **Preserved** | Full source archive and R2 addendum. Standard + Everything Unlocked ROM identities/hashes documented. |
| R3 | Reconstructable / Partial | R3 source-change patch and historical addendum material preserved. Exact original full source/ROM package not currently confirmed. |
| R4 | **Preserved / Reconstructable** | R4 patch and R4 addendum preserved; ROM hashes documented. |
| R5 | Documented / likely reconstructable | Historical R5 addendum exists in later source bundles; exact original release package not currently confirmed. |
| R6 | **Documented strongly** | R6 addendum with both ROM identities/hashes and first runtime-test Co-Op implementation. Exact source ZIP not separately confirmed. |
| R7 | **Documented strongly** | R7 crash-fix addendum with ROM hashes. |
| R8 | **Preserved / Reconstructable** | R8 patch + addendum with ROM identities/hashes. |
| R9 | **Documented strongly** | R9 addendum documents dump-confirmed collision-bounds fix and ROM hashes. |
| R10 | Documented | Historical addendum preserved in later source bundle. Exact original release package not separately confirmed. |
| R11 | Documented | Historical addendum preserved in later source bundle. Exact original release package not separately confirmed. |
| R12 | **Preserved / Reconstructable** | R12 patch, addendum and SHA-256 manifest preserved; both ROMs, source archive, patch and addendum hashes recorded. |
| R13 | Documented / reconstructable | Historical addendum + BUILDING_R13 material retained in later source bundle. |
| R14 | Documented / reconstructable | Historical addendum + BUILDING_R14 material retained. |
| R15 | Documented / reconstructable | Historical addendum + BUILDING_R15 material retained. |
| R16 | Documented / reconstructable | Historical addendum + BUILDING_R16 material retained. |
| R17 | Documented / reconstructable | Historical addendum + BUILDING_R17 material retained. |
| R18 | Documented / reconstructable | Historical addendum + BUILDING_R18 material retained. |
| R19 | Documented / reconstructable | Historical addendum + BUILDING_R19 material retained. |
| R20 | **Documented strongly / reconstructable** | R20 hashes and package policy preserved in R21 WIP handoff; historical R20 addendum/build material retained in later source. |

## R21 development lineage

| Revision | Status | Currently preserved / known |
|---|---|---|
| R21 Test/RC | **Preserved** | Community source, test-build ZIP, addendum, WIP handoff, Project64 test matrix, package SHA-256 manifest. |
| RC2 | **Preserved** | Community source + test-build ZIP. |
| RC3 | **Preserved** | Community source + test-build ZIP. |
| RC4 | **Preserved** | Community source + test-build ZIP. |
| RC5 | **Preserved** | Community source + test-build ZIP + package SHA-256 manifest. |
| RC5 GC fix | **Preserved** | Dedicated GC-fix source snapshot. Important for split-screen room-lifetime regression history. |
| RC6 | **Preserved** | Community source + test-build ZIP + package SHA-256 manifest. |
| RC7 | **Preserved** | Community source + test-build ZIP. |
| RC7 Hotfix | **Preserved** | Hotfix test-build ZIP + SHA-256 manifest. |
| RC8 | **Preserved** | Test-build ZIP. |
| RC9 WIP | **Preserved** | WIP source handoff ZIP, WIP addendum, current-changes patch, WIP hashes. |
| RC9 Test Candidate | **Preserved** | Community source, test build, candidate addendum, Project64 matrix, release-candidate hashes, static-regression update, continuation audit bundle. |
| RC10 | **Preserved (multiple checkpoints)** | Community source, test build, cumulative handoff, all-features black-screen-fixed source, 2P render-fix source, feature-restoration source-only checkpoint. |
| RC10 UI alignment variants | **Preserved** | Co-Op Game Over stat alignment ROM/source/addendum; MP pause Kills/Losses alignment source. |
| RC11 FB-clear A/B | **Preserved** | Dedicated framebuffer-clear A/B test package. |
| R21 Final | **Preserved** | Full final source + final cumulative addendum. Final public ROM identity documented. |

## R21 Final public identity

- **Display/header:** `GoldenEye 007 Plus`
- **File:** `GoldenEye_007_Plus_R21_Final.z64`
- **Size:** 12,582,912 bytes
- **CRC1/CRC2:** `450D5CA9-173182EB`
- **SHA-1:** `6072523d963b3db7c272909c30565e40fc41cd4b`
- **SHA-256:** `16e13b788343f43176bf249f56d7e6d72bc87bbca9801da7ebd9a8367dc55e89`

R21 Final is the point where the project moved to a **single public ROM** because `Unlock Everything` became a per-save action.

## R22 development lineage

| Revision/checkpoint | Status | Currently preserved / known |
|---|---|---|
| Disable Knockback | **Preserved** | ROM, full modified source, patch. |
| Disable Noise Dithering WIP | **Preserved** | Source snapshot + patch. |
| Disable Noise Dithering Final | **Preserved** | Full source + final patch + cumulative addendum. |
| SP Watch Special Options Scroller | **Preserved** | Final ROM, full source + patch + cumulative addendum. |
| GitHub-ready source | **Preserved** | GitHub-ready source archive. |
| GitHub clean source | **Preserved** | Clean repository snapshot uploaded/pushed to GitHub. |

## Latest known R22 ROM checkpoint

SP Watch Scroller build:

- **Size:** 12,582,912 bytes
- **CRC1/CRC2:** `D5D69F6C-227788D4`
- **SHA-1:** `97e0a318d5ce4bc6d201ad55b0bf27b927da9e88`
- **SHA-256:** `0731d23b51a00be61f201d94c0ea23f3d7f3d70fb03a7e7ef3b67fde93d4ab00`
- **Compressed C/data:** 72,704 / 72,704 bytes
- **PHYSICAL_CODE audit:** PASS

## Important renderer-history correction

Historical R21 notes recorded **Force depth buffer clear = ON** as the reliable GLideN64 fix for later black-world corruption. Current testing on **GLideN64 revision `1f4d04f`** refines this:

- Force depth buffer clear helps, but does **not** fully eliminate every observed manifestation.
- Disabling **Emulate framebuffer** eliminates the current observed corruption.
- Therefore framebuffer-emulation behavior must remain an independent test variable.
- This does **not** invalidate the earlier genuine source-side 3P/4P room-lifetime/GC regression. These are two distinct bug classes.

Recommended current GLideN64 diagnostic baseline:
- `Force depth buffer clear = ON`
- test both `Emulate framebuffer = ON` and `OFF`
- record GLideN64 revision with every rendering report
