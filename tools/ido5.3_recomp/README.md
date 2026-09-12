# `ido5.3_recomp`

This directory builds a native x86-64 Linux copy of the original SGI IRIX
5.3 IDO compiler toolchain (`cc`, `cfe`, `uopt`, `ugen`, `as1`, `acpp`,
`copt`, `ujoin`, `uld`, `umerge`, `usplit`, plus the `err.english.cc`
message table `cc` reads at runtime). This is what the root `Makefile`
invokes when building with `IDO_RECOMP=YES` (the default) — see
[`docs/SetupGuide.md`](../../docs/SetupGuide.md#recompile-ido) for how that
fits into the overall build.

## Where this comes from

The original IDO 5.3 binaries only ever ran on real IRIX (MIPS) hardware or
under an emulator like `qemu-irix`. [`decompals/ido-static-recomp`
](https://github.com/decompals/ido-static-recomp) is a *static
recompiler*: a tool (`recomp.cpp` in this directory) that disassembles one
of those original MIPS ELF binaries and emits equivalent C source, which is
then compiled normally with a host C compiler (`gcc`). The result is a
program that behaves like the original IRIX binary but runs natively,
without any emulation overhead — much faster to compile GoldenEye's C
source with than running the real thing under `qemu-irix`.

`decompals/ido-static-recomp` is itself a fork of
[`Emill/ido-static-recomp`](https://github.com/Emill/ido-static-recomp)
(the original author's repo) that has become the actively maintained
version. **This directory's source is a vendored copy of
`decompals/ido-static-recomp`, not a submodule** — the relevant files
(`recomp.cpp`, `libc_impl.c`, `libc_impl.h`, `header.h`, `helpers.h`,
`elf.h`, `version_info.c`) are copied in directly rather than referenced as
a git submodule or fetched at build time, so this repo can be built
offline/reproducibly without depending on that repo staying available.

## What's tracked here vs. generated

Only source files and this `Makefile`/`README.md` are committed to git:

* `recomp.cpp`, `libc_impl.c`, `libc_impl.h`, `header.h`, `helpers.h`,
  `elf.h`, `version_info.c` — the recompiler and the IRIX-syscall runtime
  shim (`libc_impl.*`) the recompiled binaries link against
* `rabbitizer/` — vendored dependency, see below
* `Makefile`, `.gitignore`, this `README.md`

Everything else in this directory when you look at it locally (`recomp`,
`cc`, `cfe`, `uopt`, `ugen`, `as1`, `acpp`, `copt`, `ujoin`, `uld`,
`umerge`, `usplit`, `err.english.cc`, the intermediate `*_c.c` files,
`*.o`) is build output, covered by `.gitignore`, and gets regenerated from
scratch by `make`.

## Building

### Automatically (normal case)

A plain `make` from the repo root builds this toolchain automatically if
`tools/ido5.3_recomp/cc` doesn't exist yet — wired up via `tools/Makefile`
(`$(MAKE) -C ido5.3_recomp` runs as a dependency whenever `cc` is missing)
and `scripts/make/build_tools.sh`, which the root `Makefile`'s
`prerequisites` target calls. You don't need to do anything extra for this
in the common case.

### Manually

```bash
cd tools/ido5.3_recomp
make
```

This requires the original SGI IRIX 5.3 IDO binaries to be present at
`../irix/root/usr/{bin,lib}/*` (checked into this repo — `cc` is under
`usr/bin`, everything else recompiled is under `usr/lib`). The recipe
(per binary): run `./recomp <irix binary> > <name>_c.c`, then compile that
generated C against `libc_impl.o` (the IRIX syscall/runtime shim) and
`version_info.o` with `gcc`.

`ugen` needs `recomp --conservative` specifically — the original `ugen`
binary relies on undefined-behavior stack reads, and `--conservative`
makes `recomp` emit C that reproduces that behavior safely instead of
leaving it to chance under a different compiler/optimization level.

`make clean` removes all generated binaries/intermediate files and also
runs `make -C rabbitizer distclean`.

## Rabbitizer (the vendored dependency)

`recomp` needs a MIPS instruction disassembler to make sense of the
original IRIX binaries' machine code before it can emit equivalent C.
Older versions of this tool used the general-purpose `libcapstone`
library for that (a system package, `libcapstone-dev`). As of decompals'
rewrite in October 2022, upstream replaced Capstone with
[`rabbitizer`](https://github.com/Decompollaborate/rabbitizer) — a small,
purpose-built MIPS/RSP/R5900 disassembly library maintained by the same
decompilation-tooling group, with better support for the instruction
quirks recomp actually cares about.

**"Vendored in" means the `rabbitizer/` subdirectory here is a plain copy
of rabbitizer's source**, not a git submodule/subrepo and not a system
package — there's nothing to `apt install` for it. Upstream
`decompals/ido-static-recomp` tracks rabbitizer as a `git subrepo` (see
their `tools/rabbitizer/.gitrepo`) pinned at commit
`72bf240f468d30286888212b5fb773fae94340f6`; the copy here was taken from
that same commit, with the Python bindings directory and standalone test
programs stripped out (`recomp` only needs the C/C++ static library, not
rabbitizer's Python module or its own test suite).

It's built automatically as a dependency of `recomp` — the `Makefile`
here runs `make -C rabbitizer static` first, which produces
`rabbitizer/build/librabbitizer.a` and `librabbitizerpp.a` (both
gitignored build output), and `recomp` links against the C++ one.

To refresh rabbitizer to a newer version: check what commit current
`decompals/ido-static-recomp` pins in `tools/rabbitizer/.gitrepo`, fetch
[`Decompollaborate/rabbitizer`](https://github.com/Decompollaborate/rabbitizer)
at that commit, and replace this `rabbitizer/` directory with it (again
trimming the Python bindings/tests if you want to keep the vendored copy
minimal — they aren't required for `make static`).

## Version pinning / updating to a newer decompals release

The recompiler source here is currently pinned to
`decompals/ido-static-recomp` @ commit `d5aec59` (`origin/main` HEAD at
the time it was last updated, 17 commits past tag `v1.2`). This is
recorded in two places: a comment at the top of this `Makefile`, and the
`RECOMP_PACKAGE_VERSION` variable baked into every built binary — check it
with `./cc --version` (or any other built binary) after building.

To pick up a newer version:

1. Clone or update a local checkout of
   [`decompals/ido-static-recomp`](https://github.com/decompals/ido-static-recomp)
   and check out whatever commit/tag you want to move to.
2. Copy `recomp.cpp`, `libc_impl.c`, `libc_impl.h`, `header.h`,
   `helpers.h`, `elf.h`, and `version_info.c` from that checkout over the
   copies in this directory. Diff first — upstream's `main` root `Makefile`
   is much bigger than this one (it builds IDO 7.1 too, multiple binary
   sets, universal macOS builds, etc.); this `Makefile` only needs to keep
   pace with whatever those individual source files require to compile.
3. If rabbitizer's pinned commit changed upstream, refresh `rabbitizer/`
   per the section above.
4. Update the pinned-commit comment at the top of this `Makefile` and the
   `RECOMP_PACKAGE_VERSION` variable.
5. `make clean && make` here, then — **this is the important part** — do a
   full clean rebuild of the ROM for all three regions from the repo root
   (`make clean && make VERSION=US && make VERSION=EU && make VERSION=JP`)
   and confirm `scripts/make/checksum.sh` still reports `MATCH!` for each.
   This project's entire premise is a byte-identical ROM; a recomp version
   bump that changes how any of these programs behaves when compiling
   GoldenEye's actual source is a real regression even if everything
   "builds successfully." If a mismatch shows up, bisect by swapping
   individual binaries (`cc`, `cfe`, `uopt`, `ugen`, `as1`, `uld`,
   `umerge`, `ujoin`, `usplit`, `acpp`, `copt`) between old and new builds
   to isolate which one changed behavior.
