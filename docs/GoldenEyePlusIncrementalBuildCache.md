# GoldenEye Plus incremental build cache

The Plus physical build uses `build/u-phys-opt-mod`. Do not delete this directory between normal development builds.

## Fast normal build

```sh
scripts/geplus_quickbuild.sh
```

GNU Make reuses every unchanged object/generated asset already present in `build/u-phys-opt-mod` and rebuilds only dependencies whose sources are newer/changed.

## Move a warm cache to another checkout

After one complete successful build:

```sh
scripts/geplus_export_build_cache.sh geplus-cache.tar.zst
```

In a fresh matching source checkout:

```sh
scripts/geplus_restore_build_cache.sh geplus-cache.tar.zst
scripts/geplus_quickbuild.sh
```

The cache is configuration-specific: US + PHYSICAL_CODE + PHYSICAL_FASTPATHS + MODDED_CHEATS + MAP_MAKER + LLVM wrapper. Do not use it for another build profile.

Never run `make clean`, `physical-clean`, or `nuke` for ordinary iterations; those intentionally throw away the acceleration.

A release may ship a known-good cache produced from the same checkpoint. New or modified source/assets remain safe: their newer timestamps/dependencies cause Make to regenerate the corresponding output, while untouched retail assets/objects remain cached.
