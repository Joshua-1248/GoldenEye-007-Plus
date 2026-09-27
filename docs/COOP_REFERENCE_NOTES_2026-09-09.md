# GoldenEye Mod Co-Op Reference Notes — 2026-09-09

## User-supplied historical references

- `gefix2pcoop.zip` — “The Spy Who Loved Me - Co-Op Death Patches”; notes credit the original co-op patch to SubDrag and this revised 1–4 player version to Zoinkity.
- `dualeyescoopge[rucksackgamer].zip` — Dual Eyes Remade V2.0 by Rucksack Gamer; its readme credits SubDrag's co-op patch and Zoinkity's 8 MB allocation patch as stability requirements.

These files were used as reverse-engineering references. They are not redistributed in this source bundle.

## CoOp-USA.ips mapping

The USA IPS contains one 54-byte record at retail ROM offset `0x000BE820`, inside `record_damage_kills()`.

Its important semantic change is:

- compare the incoming damage-source `playerid` against the active player count;
- if it is outside the active range (for example `-1` from a guard, autogun, hazard, etc.), replace it with the current/victim player index;
- the subsequent multiplayer death bookkeeping therefore treats the death as a suicide rather than indexing invalid player data or calling `set_cur_player()` with an invalid index.

R10 ports this behavior at source level and applies the same range check before the earlier multiplayer damage-direction accounting, avoiding an additional out-of-range use present in retail.

## R9 crash dump finding

`DUMP5(2).bin` contains a stopped/faulted main thread at:

- thread object: `0x8005DA70`
- PC: `0x806B4210`
- Cause: `0x0000003C` (floating-point exception)
- FPCSR: `0x01010814`

The PC maps inside `stanPointProjectsOntoEdge()` (R7 physical map: `0x806B4194..0x806B4268`). The caller is `stanTestVolume()`.

At the fault:

- Player 0 struct: `0x800D3600`
- Player 0 `collision_bounds`: `0x800D36B0` and contains invalid/debug-pattern floats.
- Player 0 authoritative `field_488.collision_position`: approximately `(20197.742, -106.002, 16902.072)`.
- Player 0 `field_488.collision_radius`: `30.0`.

Thus the position/radius are valid while the cached rectangle consumed by the collision system is not. R10 rebuilds the four-point viewer collision rectangle from those authoritative values immediately in `bondviewGetPropHeightRelatedValues()` before returning it to collision code during campaign Co-Op.
