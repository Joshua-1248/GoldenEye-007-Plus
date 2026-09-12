# Makefile to build Goldeneye 007

### Default target ###
default: all

### Default Build Options ###
# Version of the game to build
FINAL := YES
VERSION := US
IDO_RECOMP := YES
VERBOSE := 2
# Build the play-testable optimized ROM in a separate output directory.
# The safe default keeps IDO codegen and enables only audited source-level fast paths;
# selected files can be promoted to the modern compiler explicitly.
# The normal matching/IDO build remains the default and is untouched.
OPTIMIZED_ROM ?= NO
# Experimental architecture branch: relocate all GoldenEye executable code out
# of KUSEG/TLB space into directly mapped KSEG0 physical RAM. This is kept
# separate from both the matching retail build and OPTIMIZED_ROM.
PHYSICAL_CODE ?= NO
# Opt-in source-level fast paths for the physical-code branch.  Keep this
# separate so the known-good R1 physical baseline remains reproducible.
PHYSICAL_FASTPATHS ?= NO
UNLOCK_SAVE1 ?= NO
MODDED_CHEATS ?= NO
PHYSICAL_FAST_CFILES := chrprop.c stan.c
# Native-12MiB optimized ROM profile. "balanced" keeps most game code at -Os
# (important both for ROM capacity and the VR4300's small I-cache) while
# promoting the most frequently exercised AI/collision/model modules to -O2.
# "conservative" is all -Os; "speed" is all -O2 and is expected to exceed
# the retail 12 MiB layout unless further size work is done.
OPTIMIZED_PROFILE ?= balanced
OPT_HOT_CFILES := chrai.c chraction.c chrprop.c stan.c prop.c bg.c model.c
# Safety-first hybrid ROM: only these translation units use the modern compiler.
# Every other src/game C file is compiled by IDO exactly like the matching build.
# Keep this whitelist small and expand it only after a real-ROM runtime audit.
OPT_MODERN_CFILES ?=
# Legacy experimental whole-object fast paths. Keep empty in the native-12 MiB
# fixed-layout build: changing a src/game object size moves later 0x7F functions
# and 8 KiB TLB page boundaries. Game optimizations must use fixed-address
# in-place replacements or explicitly managed trampolines/overlays instead.
OPT_IDO_FAST_CFILES ?=

# Backward compatibility: an explicit OPTIMIZED_LEVEL overrides the profile and
# applies one optimization level to every game C translation unit.
ifneq ($(origin OPTIMIZED_LEVEL), undefined)
 OPT_GAME_DEFAULT_OPT := -O$(OPTIMIZED_LEVEL)
 OPT_GAME_HOT_OPT := -O$(OPTIMIZED_LEVEL)
else ifeq ($(OPTIMIZED_PROFILE), conservative)
 OPT_GAME_DEFAULT_OPT := -Os
 OPT_GAME_HOT_OPT := -Os
else ifeq ($(OPTIMIZED_PROFILE), balanced)
 OPT_GAME_DEFAULT_OPT := -Os
 OPT_GAME_HOT_OPT := -O2
else ifeq ($(OPTIMIZED_PROFILE), speed)
 OPT_GAME_DEFAULT_OPT := -O2
 OPT_GAME_HOT_OPT := -O2
else
 $(error Unknown OPTIMIZED_PROFILE='$(OPTIMIZED_PROFILE)' (use conservative, balanced, or speed))
endif
OPT_CC ?= $(shell command -v mips-linux-gnu-gcc-13 2>/dev/null || command -v mips-linux-gnu-gcc 2>/dev/null || echo mips-linux-gnu-gcc-13)
OPT_USE_LLVM ?= NO
ifeq ($(OPT_USE_LLVM), YES)
 OPT_CC := scripts/toolchain/llvm-mips-gcc
endif
# If COMPARE is 1, check the output sha1sum when building 'all', and if fail to match
# then compare ELF sections to known md5 checksums.
COMPARE := 1

# --- Tier 0 "FAST" build ------------------------------------------------
# Opt-in, behavior-preserving toolchain swap: compiles the SAME C, with NO
# logic changes, through a modern GCC MIPS cross-compiler + LTO instead of
# IDO. This never touches the matching build above (VERSION/IDO_RECOMP are
# untouched by it) and does not itself produce a linked ROM yet — it is a
# verification target that compiles every file we've confirmed is
# GCC-clean (see docs/FastBuildAudit.md and scripts/fastbuild_sweep.sh)
# with real optimization + whole-program LTO, so the toolchain path is
# proven end-to-end rather than assumed.
#
#   make fastbuild            # compile all confirmed-clean src/game files
#   scripts/fastbuild_sweep.sh   # regenerate the confirmed-clean file list
#
# Deliberately excluded: -ffast-math, -funsafe-math-optimizations,
# -ffinite-math-only, strict aliasing. Those change floating-point
# results and are NOT behavior-preserving; see docs/FastBuildAudit.md
# Tier 2 for why they're kept separate from this target.
FAST_CC       ?= $(OPT_CC)
FAST_INCLUDE  := -I . -I include -I include/ultra64 -I include/PR -I src -I src/game -I src/inflate
FAST_LCDEFS   := -DVERSION_US -DLANG_US -DREFRESH_NTSC -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DBYTEMATCH -DTARGET_N64 \
                 -DGE_FASTBUILD
FAST_CFLAGS   := -march=mips2 -mtune=vr4300 -mfix4300 -mabi=32 -G0 \
                 -fno-strict-aliasing -fwrapv -fno-pic -mno-abicalls -fno-builtin \
                 -fms-extensions -include src/fastbuild_abi.h \
                 $(FAST_INCLUDE) $(FAST_LCDEFS)
FAST_OPT      := -O2 -flto
FAST_BUILD_DIR := build/fast
FAST_COMBINED   := $(FAST_BUILD_DIR)/game-combined.o
FAST_CLEAN_LIST := build/fastbuild_clean_files.txt
# The compatibility blockers are fixed, so the build target now attempts the
# complete game source set. The sweep list remains a diagnostic/audit artifact.
FAST_SOURCES  := $(shell find src/game -name '*.c' | sort)
FAST_OBJECTS  := $(foreach f,$(FAST_SOURCES),$(FAST_BUILD_DIR)/$(f:.c=.o))

$(FAST_BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(FAST_CC) -c $(FAST_CFLAGS) $(FAST_OPT) -o $@ $<

.PHONY: fastbuild fastbuild-sweep fastbuild-abi-audit fastpath-test
fastbuild-sweep:
	scripts/fastbuild_sweep.sh

fastpath-test:
	scripts/test_fastpaths.sh

fastbuild-abi-audit:
	scripts/audit_fastbuild_abi.sh

$(FAST_COMBINED): $(FAST_OBJECTS)
	@mkdir -p $(dir $@)
	$(FAST_CC) -nostdlib -r -flto -Werror=lto-type-mismatch -EB -mabi=32 $(FAST_OBJECTS) -o $@

fastbuild: $(FAST_COMBINED)
	@echo "Compiled all $(words $(FAST_OBJECTS)) src/game file(s) with $(FAST_CC) $(FAST_OPT)."
	@echo "MIPS LTO ABI merge passed: $(FAST_COMBINED)"
	@echo "See docs/FastBuildAudit.md for verification status and behavior-preservation notes."
# -------------------------------------------------------------------------

# Include Terminal Codes for colourising text.
include include/make/VT100Codes.make
include include/make/Gui.make

# set toolchain based on current OS
ifeq ($(shell type mips-linux-gnu-ld >/dev/null 2>/dev/null; echo $$?), 0)
  TOOLCHAIN := mips-linux-gnu-
else ifeq ($(shell type mips64-linux-gnu-ld >/dev/null 2>/dev/null; echo $$?), 0)
  TOOLCHAIN := mips64-linux-gnu-
else
  TOOLCHAIN := mips64-elf-
endif
ifeq ($(OPT_USE_LLVM), YES)
  TOOLCHAIN := scripts/toolchain/llvm-mips-
endif

# Use IDO Recomp UNLESS specified otherwise
ifeq ($(IDO_RECOMP), NO)
  QEMU_IRIX := $(shell which qemu-irix 2>/dev/null)
  ifeq (, $(QEMU_IRIX))
    $(error Using the IDO compiler requires qemu-irix. Please install qemu-irix package or set the QEMU_IRIX environment variable to the full qemu-irix binary path)
  endif
  IRIX_ROOT := tools/irix/root
else
  IRIX_ROOT := tools/ido5.3_recomp
endif

# other tools
TOOLS_DIR := tools
DATASEG_COMP := $(TOOLS_DIR)/data_compress.sh
RZ_COMP := $(TOOLS_DIR)/1172compress.sh
N64CKSUM := $(TOOLS_DIR)/n64cksum

ifeq ($(VERBOSE), 1)
 SHA1SUM = sha1sum
else
 SHA1SUM = sha1sum --quiet
endif

# Convert AI Print commands from readable strings to byte arrays automatically.
ConvertAIPRINT = sed -E -e ':loop s/PRINT\("(..*?)(.)"/PRINT\("\1",\x27\2\x27/g; tloop; \
                s/(PRINT\(.*?)\x27\\\x27,\x27(.)\x27(.*)\)/\1\x27\\\2\x27\3\)/g; \
                s/(PRINT\()"(.)"(.*)\)/\1\x27\2\x27\3\)/g; \
                s/PRINT\((.*)\)/PRINT\(\1,\x27\\0\x27\,)/g; \
                s/PRINT\((.*)\)/AI_PRINT,\1/g'

# per VERSION flags
ifeq ($(FINAL), YES)
 OPTIMIZATION := -O2
 LCDEFS :=
 CFLAGWARNING :=
else
 OPTIMIZATION := -g
 LCDEFS := -DDEBUG
 CFLAGWARNING :=-fullwarn -wlint
endif

ifeq ($(VERSION), US)
 COUNTRYCODE := u
 OUTCODE := $(COUNTRYCODE)
 LANG := US
 LCDEFS := -DVERSION_US -DLANG_US -DREFRESH_NTSC -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DBYTEMATCH
 ASMDEFS := --defsym VERSION_US=1 --defsym LANG_US=1 --defsym REFRESH_NTSC=1 --defsym LEFTOVERDEBUG=1 --defsym LEFTOVERSPECTRUM=1 --defsym BUGFIX_R0=1 --defsym BYTEMATCH=1
 LDFILEOPTS := -DVERSION_$(LANG) -DOUTCODE=$(OUTCODE)
endif

ifeq ($(VERSION), EU)
 COUNTRYCODE := e
 OUTCODE := $(COUNTRYCODE)
 LANG := EU
 LCDEFS := -DVERSION_EU -DLANG_EU -DREFRESH_PAL -DBUGFIX_R1 -DBUGFIX_R2 -DBYTEMATCH
 ASMDEFS := --defsym VERSION_EU=1 --defsym LANG_EU=1 --defsym REFRESH_PAL=1 --defsym BUGFIX_R1=1 --defsym BUGFIX_R2=1 --defsym BYTEMATCH=1
 LDFILEOPTS := -DVERSION_$(LANG) -DOUTCODE=$(OUTCODE)
endif

ifeq ($(VERSION), JP)
 COUNTRYCODE := j
 OUTCODE := $(COUNTRYCODE)
 LANG := JP
 LCDEFS := -DVERSION_JP -DLANG_JP -DREFRESH_NTSC -DBUGFIX_R1 -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBYTEMATCH
 ASMDEFS := --defsym VERSION_JP=1 --defsym LANG_JP=1 --defsym REFRESH_NTSC=1 --defsym BUGFIX_R1=1 --defsym LEFTOVERDEBUG=1 --defsym LEFTOVERSPECTRUM=1 --defsym BYTEMATCH=1
 LDFILEOPTS := -DVERSION_$(LANG) -DOUTCODE=$(OUTCODE)
endif

ifeq ($(VERSION), DEBUG)
 COUNTRYCODE := u
 OUTCODE := d
 LANG := US
 LCDEFS := -DVERSION_US -DLANG_US -DREFRESH_NTSC -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DDEBUGMENU -DVERSION_DEBUG
 ASMDEFS := --defsym VERSION_DEBUG=1 --defsym LANG_US=1 --defsym REFRESH_NTSC=1 --defsym LEFTOVERDEBUG=1 --defsym LEFTOVERSPECTRUM=1 --defsym BUGFIX_R0=1 --defsym DEBUGMENU=1
 COMPARE := 0
 LDFILEOPTS := -DVERSION_$(LANG) -DOUTCODE=$(OUTCODE)
endif

ifeq ($(VERSION), USB)
 COUNTRYCODE := u
 OUTCODE := usb
 LANG := US
 LCDEFS := -DVERSION_US -DLANG_US -DREFRESH_NTSC -DLEFTOVERDEBUG -DLEFTOVERSPECTRUM -DBUGFIX_R0 -DDEBUGMENU -DENABLE_USB
 ASMDEFS := --defsym VERSION_US=1 --defsym LANG_US=1 --defsym REFRESH_NTSC=1 --defsym LEFTOVERDEBUG=1 --defsym LEFTOVERSPECTRUM=1 --defsym BUGFIX_R0=1 --defsym DEBUGMENU=1 --defsym ENABLE_USB=1
 COMPARE := 0
 LDFILEOPTS := -DVERSION_$(LANG) -DOUTCODE=$(OUTCODE) -DENABLE_USB
endif

# Optimized ROMs are intentionally emitted beside, never over, matching ROMs.
# COUNTRYCODE and all region defines remain unchanged so asset selection is
# identical; only the build/output tag changes.
BASE_OUTCODE := $(OUTCODE)
ifeq ($(OPTIMIZED_ROM), YES)
 OUTCODE := $(BASE_OUTCODE)-opt
 COMPARE := 0
 LDFILEOPTS := $(subst -DOUTCODE=$(BASE_OUTCODE),-DOUTCODE=$(OUTCODE),$(LDFILEOPTS))
endif

# Physical-code mode keeps its own ROM identity/output directory.  It does not
# spoof the retail header CRC and it does not share build artefacts with the
# matching or optimized builds.
ifeq ($(PHYSICAL_CODE), YES)
 ifeq ($(OPTIMIZED_ROM), YES)
  $(error PHYSICAL_CODE=YES and OPTIMIZED_ROM=YES are separate build modes)
 endif
 OUTCODE := $(BASE_OUTCODE)-phys
 COMPARE := 0
 LCDEFS += -DGE_PHYSICAL_CODE
 ASMDEFS += --defsym GE_PHYSICAL_CODE=1
 LDFILEOPTS := $(subst -DOUTCODE=$(BASE_OUTCODE),-DOUTCODE=$(OUTCODE),$(LDFILEOPTS)) -DGE_PHYSICAL_CODE
 ifeq ($(PHYSICAL_FASTPATHS), YES)
  OUTCODE := $(BASE_OUTCODE)-phys-opt
  LCDEFS += -DGE_PHYSICAL_FASTPATHS
  ASMDEFS += --defsym GE_PHYSICAL_FASTPATHS=1
  LDFILEOPTS := $(subst -DOUTCODE=$(BASE_OUTCODE)-phys,-DOUTCODE=$(OUTCODE),$(LDFILEOPTS))
 endif
else ifeq ($(PHYSICAL_FASTPATHS), YES)
 $(error PHYSICAL_FASTPATHS=YES requires PHYSICAL_CODE=YES)
endif

ifeq ($(MODDED_CHEATS), YES)
 ifneq ($(PHYSICAL_CODE), YES)
  $(error MODDED_CHEATS=YES requires PHYSICAL_CODE=YES)
 endif
 MOD_BASE_OUTCODE := $(OUTCODE)
 OUTCODE := $(MOD_BASE_OUTCODE)-mod
 LDFILEOPTS := $(subst -DOUTCODE=$(MOD_BASE_OUTCODE),-DOUTCODE=$(OUTCODE),$(LDFILEOPTS))
 LCDEFS += -DGE_MODDED_CHEATS
 ASMDEFS += --defsym GE_MODDED_CHEATS=1
endif

ifeq ($(UNLOCK_SAVE1), YES)
 ifneq ($(PHYSICAL_CODE), YES)
  $(error UNLOCK_SAVE1=YES requires PHYSICAL_CODE=YES)
 endif
 # Keep the Everything Unlocked variant in its own build directory.  Compiler
 # flags are not tracked as Make dependencies, so sharing u-phys/u-phys-opt
 # could otherwise leave stale GE_UNLOCK_SAVE1 objects when switching variants.
 UNLOCK_BASE_OUTCODE := $(OUTCODE)
 OUTCODE := $(UNLOCK_BASE_OUTCODE)-unlock
 LDFILEOPTS := $(subst -DOUTCODE=$(UNLOCK_BASE_OUTCODE),-DOUTCODE=$(OUTCODE),$(LDFILEOPTS))
 LCDEFS += -DGE_UNLOCK_SAVE1
 ASMDEFS += --defsym GE_UNLOCK_SAVE1=1
endif

ALLOWED_VERSIONS := US EU JP DEBUG USB
ALLOWED_COUNTRYCODE := u e j

BUILD_DIR_BASE := build
# BUILD_DIR is the location where all build artefacts are placed
BUILD_DIR      := $(BUILD_DIR_BASE)/$(OUTCODE)

# this file references variables defined above: BUILD_DIR, RZ_COMP
# this file defines and builds $(MUSIC_RZ_FILES)
include assets/Makefile.obseg
# this file references variables defined above: BUILD_DIR, RZ_COMP, COUNTRYCODE, LD, CC, CFLAGS, OBJCOPY, ConvertAIPRINT, OPTIMIZATION
# this file defines and builds OBSEGMENT, BG_SEG_FILES, BRIEF_RZ_FILES, CHR_RZ_FILES, GUN_RZ_FILES, PROP_RZ_FILES, ,SETUP_BUILD_FILES, STAN_BUILD_FILES, TEXT_RZ_FILES
include assets/Makefile.music

## Collect Objects ##

APPELF := $(BUILD_DIR)/ge007.$(OUTCODE).elf
APPROM := $(BUILD_DIR)/ge007.$(OUTCODE).z64
APPBIN := $(BUILD_DIR)/ge007.$(OUTCODE).bin

HEADERFILES := $(foreach dir,src,$(wildcard $(dir)/*.s))
HEADEROBJECTS := $(foreach file,$(HEADERFILES),$(BUILD_DIR)/$(file:.s=.o))

RSPCODE := $(foreach dir,rsp,$(wildcard $(dir)/*.s))
RSPOBJECTS := $(foreach file,$(RSPCODE),$(BUILD_DIR)/$(file:.s=.bin))

CODEFILES := $(foreach dir,src,$(wildcard $(dir)/*.c))
CODEOBJECTS := $(foreach file,$(CODEFILES),$(BUILD_DIR)/$(file:.c=.o))

GAMEFILES_C := $(foreach dir,src/game,$(wildcard $(dir)/*.c))
GAMEFILES_S := $(foreach dir,src/game,$(wildcard $(dir)/*.s))
GAMEOBJECTS := $(foreach file,$(GAMEFILES_S),$(BUILD_DIR)/$(file:.s=.o)) \
				$(foreach file,$(GAMEFILES_C),$(BUILD_DIR)/$(file:.c=.o))


ASSET_DATAFILES := assets/oddtextures.c assets/animationtable_data.c assets/animationtable_entries.c assets/font_dl.c assets/font_chardataj.c assets/font_chardatae.c assets/rarewarelogo.c
ASSET_DATAOBJECTS := $(foreach file,$(ASSET_DATAFILES),$(BUILD_DIR)/$(file:.c=.o))

ROMFILES2 := assets/romfiles2.s
ROMOBJECTS2 := $(BUILD_DIR)/assets/romfiles2.o

RAMROM_FILES := assets/ramrom/ramrom.s
RAMROM_OBJECTS := $(BUILD_DIR)/assets/ramrom/ramrom.o


FONTFILES_C := $(foreach dir,assets/font,$(wildcard $(dir)/*.c))
FONTOBJECTS := $(foreach file,$(FONTFILES_C),$(BUILD_DIR)/$(file:.c=.o))


MUSIC_FILES := $(foreach dir,assets/music,$(wildcard $(dir)/*.s))
MUSIC_OBJECTS := $(foreach file,$(MUSIC_FILES),$(BUILD_DIR)/$(file:.s=.o))

OBSEG_FILES := assets/obseg/ob_seg.s
OBSEG_OBJECTS := $(BUILD_DIR)/assets/obseg/ob_seg.o
OBSEG_RZ := $(BG_SEG_FILES) $(CHR_RZ_FILES) $(GUN_RZ_FILES) $(PROP_RZ_FILES) $(STAN_RZ_FILES) $(BRIEF_RZ_FILES) $(SETUP_RZ_FILES) $(TEXT_RZ_FILES)

IMAGE_BINS := assets/images/combined/combined.bin
IMAGE_OBJS := $(foreach file,$(IMAGE_BINS),$(BUILD_DIR)/$(file:.bin=.o))

RZFILES := inflate/inflate.c
RZOBJECTS := $(foreach file,$(RZFILES),$(BUILD_DIR)/src/$(file:.c=.o))

OBJECTS := $(RSPOBJECTS) $(CODEOBJECTS) $(GAMEOBJECTS) $(RZOBJECTS) $(OBSEGMENT) $(ROMOBJECTS) $(RAMROM_OBJECTS) $(FONTOBJECTS) $(MUSIC_OBJECTS) $(IMAGE_OBJS)

## Command Line args for builders ##

MIPSISET := -mips2 -32

INCLUDE := -I . -I include -I include/ultra64 -I include/PR -I src -I src/game -I src/inflate

# ignore warnings:
# 609 : The number of arguments in the macro invocation does not match the definition - disabled because CPPLib uses "VarArgs" which wasnt invented till c99
# 649 : Missing member name in structure / union                                      - used for "Inheritance"
# 709 : Incompatible pointer type assignment                                          - could be fixed by casting, but implicit is fine.
# 712 : illegal combination of pointer and integer                                    - could be fixed by casting, but implicit is fine.
# 807 : member cannot be of function or incomplete type                               - Variable length structs
# 838 : Microsoft extension (unnamed structs)                                         - used for "Inheritance" and member/array call swapping
# 763 : Max Float
WOFF :=  -woff 609,649,709,712,807,838,763

ifeq ($(IDO_RECOMP), NO)
  CC := $(QEMU_IRIX) -silent -L $(IRIX_ROOT) $(IRIX_ROOT)/usr/bin/cc
else
  CC := $(IRIX_ROOT)/cc
endif

CFLAGS := -Wab,-r4300_mul -non_shared -Olimit 2000 -G 0 -Xcpluscomm $(CFLAGWARNING) $(WOFF) $(INCLUDE) $(MIPSISET) $(LCDEFS) -DTARGET_N64

# Play-test ROM compiler flags. Keep the ISA at MIPS II (the original code
# target), tune scheduling for VR4300, and explicitly disable optimizer
# assumptions that are unsafe for this pre-ISO/alias-heavy codebase. LTO and
# fast-math are deliberately not part of the playable Tier-1 build yet.
OPT_GAME_CFLAGS = -std=gnu89 -EB -march=mips2 -mtune=vr4300 -mfix4300 \
                  -mabi=32 -mgp32 -mfp32 -mhard-float -G0 \
                  -fno-pic -mno-abicalls -mno-shared \
                  -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks \
                  -fno-builtin -fms-extensions -fno-stack-protector -fno-common \
                  -fno-asynchronous-unwind-tables -fno-unwind-tables \
                  -fno-merge-constants -fno-toplevel-reorder \
                  -fno-reorder-blocks-and-partition -fno-ipa-icf \
                  -include src/fastbuild_abi.h \
                  $(INCLUDE) $(LCDEFS) -DGE_FASTBUILD -DGE_OPTIMIZED -DTARGET_N64
OPT_THIS_GAME_OPT = $(if $(filter $(notdir $<),$(OPT_HOT_CFILES)),$(OPT_GAME_HOT_OPT),$(OPT_GAME_DEFAULT_OPT))

LD := $(TOOLCHAIN)ld
LD_SCRIPT := $(BUILD_DIR)/ge007.$(OUTCODE).ld

# --no-warn-mismatch is needed to link -mips3 object files (some libultra math) with the regular files compiled with -mips2
LDFLAGS := -T $(LD_SCRIPT) -Map $(BUILD_DIR)/ge007.$(OUTCODE).map --no-warn-mismatch

AS := $(TOOLCHAIN)as
ASFLAGS := -march=vr4300 -mabi=32 $(INCLUDE) $(ASMDEFS)
# Use the system installed armips if available. Otherwise use the one provided with this repository.
ifneq (,$(shell which armips 2>/dev/null))
  ARMIPS              := armips
else
  ARMIPS              := $(TOOLS_DIR)/armips
endif

OBJCOPY := $(TOOLCHAIN)objcopy












## Build Recipes ##

# Don't delete intermediate files from these targets on make completion.
.SECONDARY:
	$(APPELF) $(APPROM) $(APPBIN) $(ULTRAOBJECTS) $(BUILD_DIR)/ge007.$(OUTCODE).map \
	$(HEADEROBJECTS) $(BOOTOBJECTS) $(CODEOBJECTS) $(GAMEOBJECTS) $(RZOBJECTS) \
	$(OBSEG_OBJECTS) $(OBSEG_RZ) $(ROMOBJECTS) $(RAMROM_OBJECTS) $(FONTOBJECTS) $(MUSIC_OBJECTS) $(IMAGE_OBJS) $(MUSIC_RZ_FILES)

# Don't delete these intermediate targets on make cancellation.
.PRECIOUS: %.bin  %.o

# Run the following targets sequentially in this order (unnamed targets will still run in parallel)
.NOTPARALLEL: print_info create_directories $(APPROM) checksum

# Phony Recipes - These targets are not files, Get Make to do something
.PHONY: print_info create_directories build_tools prerequisites optimized-preflight optimized optimized-clean optimized-profile-force physical physical-source-audit physical-audit physical-clean checksum all_p1 all default commonclean setupclean stanclean dataclean libultraclean codeclean clean nuke help cmdbuidler test  context extractassets forceextractassets textures convert_props convert_chrs convert_guns extract_u extract_e extract_j force_extract_u force_extract_e force_extract_j extract_rsp


# this file references variables defined above: BUILD_DIR, CFLAGWARNING, INCLUDE, LCDEFS
# this file defines and builds $(ULTRAOBJECTS)
include src/libultrare/Makefile.libultrare

# Build RSP
$(BUILD_DIR)/rsp/%.bin: rsp/*.s
	$(ARMIPS) -sym $@.sym -strequ CODE_FILE $(BUILD_DIR)/rsp/$*.bin -strequ DATA_FILE $(BUILD_DIR)/rsp/$*_data.bin $<

$(BUILD_DIR)/src/rspboot.o: $(BUILD_DIR)/rsp/rspboot.bin

#Build asm files in root
$(BUILD_DIR)/%.o: src/%.s
	$(AS) $(ASFLAGS) -o $@ $<

#Build asm files in src/
$(BUILD_DIR)/src/%.o: src/%.s
	$(AS) $(ASFLAGS) -o $@ $<

#Build Images
# Generate imagelist by syncing imagelist.u.csv (ROM offsets/sizes) with images.def (names)
$(BUILD_DIR)/imagelist.csv: imagelist.u.csv assets/images.def
	@mkdir -p $(BUILD_DIR)
	python3 scripts/make/sync_imagelist_with_def.py $@

assets/images/combined/combined.bin: $(BUILD_DIR)/imagelist.csv
	scripts/make/combine_images_named.sh $(BUILD_DIR)/imagelist.csv assets/images/combined

$(BUILD_DIR)/assets/images/combined/%.o: assets/images/combined/combined.bin
	$(LD) -r -b binary $< -o $@


#Compress Obseg
$(BUILD_DIR)/$(OBSEGMENT): $(OBSEG_RZ) $(IMAGE_OBJS)


# Physical R2 fast-path build.  Only the audited translation units below receive
# GE_OPTIMIZED; every other game file remains on the same IDO path as R1.
ifeq ($(PHYSICAL_FASTPATHS), YES)
$(BUILD_DIR)/src/game/%.o: src/game/%.c
	@base="$$(basename $<)"; \
	if echo " $(PHYSICAL_FAST_CFILES) " | grep -Fq " $$base "; then extra=-DGE_OPTIMIZED; else extra=; fi; \
	if [ "$$base" = "chraidata.c" ]; then \
		$(ConvertAIPRINT) $< | $(CC) -c $(CFLAGS) $$extra tools/include-stdin.c -o $@ $(OPTIMIZATION); \
	else \
		$(CC) -c $(CFLAGS) $$extra -o $@ $(OPTIMIZATION) $<; \
	fi
endif

# Build game C for the opt-in playable optimized ROM. The safe tier uses IDO
# for normal files plus GE_OPTIMIZED on an audited whitelist; modern codegen is
# separately opt-in through OPT_MODERN_CFILES. Assets/RSP/linking stay on the
# decomp's existing pipeline.
ifeq ($(OPTIMIZED_ROM), YES)
OPT_PROFILE_STAMP := $(BUILD_DIR)/.optimized-profile

# Make does not normally know that changing optimization flags invalidates an
# existing .o. Keep one content-addressed profile stamp: its timestamp changes
# only when the effective compiler/profile/flags change, which automatically
# rebuilds every modern game object exactly when required.
optimized-profile-force:

$(OPT_PROFILE_STAMP): optimized-profile-force
	@mkdir -p $(dir $@)
	@{ \
		printf '%s\n' 'compiler=$(OPT_CC)'; \
		printf '%s\n' 'profile=$(OPTIMIZED_PROFILE)'; \
		printf '%s\n' 'default_opt=$(OPT_GAME_DEFAULT_OPT)'; \
		printf '%s\n' 'hot_opt=$(OPT_GAME_HOT_OPT)'; \
		printf '%s\n' 'hot_files=$(OPT_HOT_CFILES)'; \
		printf '%s\n' 'modern_files=$(OPT_MODERN_CFILES)'; \
		printf '%s\n' 'ido_fast_files=$(OPT_IDO_FAST_CFILES)'; \
		printf '%s\n' 'cflags=$(OPT_GAME_CFLAGS)'; \
	} > $@.tmp
	@if ! test -f $@ || ! cmp -s $@.tmp $@; then mv -f $@.tmp $@; else rm -f $@.tmp; fi

$(BUILD_DIR)/src/game/%.o: src/game/%.c $(OPT_PROFILE_STAMP)
	@base="$$(basename $<)"; \
	if echo " $(OPT_MODERN_CFILES) " | grep -Fq " $$base "; then \
		if [ "$$base" = "chraidata.c" ]; then \
			$(ConvertAIPRINT) $< | $(OPT_CC) -x c -c $(OPT_GAME_CFLAGS) $(OPT_THIS_GAME_OPT) -o $@ -; \
		else \
			$(OPT_CC) -c $(OPT_GAME_CFLAGS) $(OPT_THIS_GAME_OPT) -o $@ $<; \
		fi; \
	elif echo " $(OPT_IDO_FAST_CFILES) " | grep -Fq " $$base "; then \
		if [ "$$base" = "chraidata.c" ]; then \
			$(ConvertAIPRINT) $< | $(CC) -c $(CFLAGS) -DGE_OPTIMIZED tools/include-stdin.c -o $@ $(OPTIMIZATION); \
		else \
			$(CC) -c $(CFLAGS) -DGE_OPTIMIZED -o $@ $(OPTIMIZATION) $<; \
		fi; \
	else \
		if [ "$$base" = "chraidata.c" ]; then \
			$(ConvertAIPRINT) $< | $(CC) -c $(CFLAGS) tools/include-stdin.c -o $@ $(OPTIMIZATION); \
		else \
			$(CC) -c $(CFLAGS) -o $@ $(OPTIMIZATION) $<; \
		fi; \
	fi
endif

# Build the resident GoldenEye TLB pager with audited native optimizations.
# Keep this separate from src/game: the matching build sees the original code,
# while OPTIMIZED_ROM enables GE_OPTIMIZED only for this one resident module.
ifeq ($(OPTIMIZED_ROM), YES)
$(BUILD_DIR)/src/tlb_manage.o: src/tlb_manage.c $(OPT_PROFILE_STAMP)
	$(CC) -c $(CFLAGS) -DGE_OPTIMIZED -o $@ $(OPTIMIZATION) $<
endif

#Build C files in src/
# convert AI_PRINT commands from readable to byte-array
$(BUILD_DIR)/src/%.o: src/%.c
	@if [ "$$(basename $<)" = "chraidata.c" ]; then \
		$(ConvertAIPRINT) $< | $(CC) -c $(CFLAGS) tools/include-stdin.c -o $@ $(OPTIMIZATION); \
	else \
		$(CC) -c $(CFLAGS) -o $@ $(OPTIMIZATION) $<; \
	fi


#Build RamRom
$(BUILD_DIR)/assets/ramrom/%.o: assets/ramrom/%.s
	$(AS) $(ASFLAGS) -o $@ $<

#Build fonts
$(BUILD_DIR)/assets/font/%.o: assets/font/%.c
	$(CC) -c $(CFLAGS) -o $@ $(OPTIMIZATION) $<

#Build asm files in assets/
$(BUILD_DIR)/assets/%.o: assets/%.s
	$(AS) $(ASFLAGS) -o $@ $<

#Build Obseg
$(BUILD_DIR)/assets/obseg/%.o: assets/obseg/%.s $(OBSEG_RZ)
	$(AS) $(ASFLAGS) -o $@ $<

#Build C files in assets/
$(BUILD_DIR)/assets/%.o: assets/%.c
ifeq ($(filter-out %setup%,$<),)
	$(ConvertAIPRINT) $< | $(CC) -c $(CFLAGS) tools/include-stdin.c -o $@ $(OPTIMIZATION)
else
	$(CC) -c $(CFLAGS) -o $@ $(OPTIMIZATION) $<
endif

#$(BUILD_DIR)/src/random.o: OPTIMIZATION := -O3
#$(BUILD_DIR)/src/random.o: INCLUDE := -I . -I include -I include/PR
#$(BUILD_DIR)/src/random.o: MIPSISET := -mips3 -o32
#$(BUILD_DIR)/src/random.o: src/random.c
#	$(CC) -c -Wab,-r4300_mul -non_shared -G 0 -Xcpluscomm $(CFLAGWARNING) -woff 819,820,852,821,838,649 -signed $(INCLUDE) $(MIPSISET) $(LCDEFS) -DTARGET_N64 $(OPTIMIZATION) -o $@ $<

#Link Files
$(APPELF): $(RSPOBJECTS) $(ULTRAOBJECTS) $(HEADEROBJECTS) $(OBSEG_RZ) $(BUILD_DIR)/$(OBSEGMENT) $(MUSIC_RZ_FILES) $(BOOTOBJECTS) $(CODEOBJECTS) $(GAMEOBJECTS) $(RZOBJECTS) $(ROMOBJECTS) $(ASSET_DATAOBJECTS) $(ROMOBJECTS2) $(RAMROM_OBJECTS) $(FONTOBJECTS) $(MUSIC_OBJECTS) $(OBSEG_OBJECTS) ge007.ld
	cpp $(LDFILEOPTS) -P ge007.ld -o $(BUILD_DIR)/ge007.$(OUTCODE).ld
	@echo "Linking Files into ELF"
	$(LD) $(LDFLAGS) -o $@

$(APPBIN): $(APPELF)
	@echo "Building ROM"
	$(OBJCOPY) $< $@ -O binary --gap-fill=0xff
ifeq ($(MODDED_CHEATS), YES)
	python3 scripts/patch_r21_front_textids.py $@
endif

$(APPROM):	$(APPBIN)
	@echo "Compressing ROM"
	$(DATASEG_COMP) $< $(OUTCODE)
	@echo "Finalizing ROM"
	$(N64CKSUM) $< $@


## Phony Recipes below - Get Make to do something ##

print_info:
	$(info VERSION=$(VERSION))
	$(info Building $(VERSION) ROM...)

create_directories:
	scripts/make/create_directories.sh "$(BUILD_DIR)" "$(COUNTRYCODE)"

build_tools:
	$(info Building tools...)
	scripts/make/build_tools.sh "$(MAKE)"

prerequisites: print_info create_directories build_tools extractassets

optimized-preflight:
ifeq ($(OPTIMIZED_ROM), YES)
	@mkdir -p $(BUILD_DIR)
	@if [ -n "$(strip $(OPT_MODERN_CFILES))" ]; then \
		command -v "$(OPT_CC)" >/dev/null 2>&1 || { \
			echo "ERROR: modern MIPS compiler not found: $(OPT_CC)"; \
			echo "Install gcc-13-mips-linux-gnu (or set OPT_CC=/path/to/compiler)."; \
			exit 127; \
		}; \
		$(OPT_CC) -dumpmachine | grep -q '^mips' || { \
			echo "ERROR: OPT_CC does not target MIPS: $$($(OPT_CC) -dumpmachine)"; exit 1; \
		}; \
		printf 'int ge_opt_probe(void){return 7;}\n' | $(OPT_CC) -x c -c -o $(BUILD_DIR)/.opt-compiler-probe.o \
			-std=gnu89 -EB -march=mips2 -mtune=vr4300 -mfix4300 -mabi=32 -G0 \
			-fno-pic -mno-abicalls -O2 - >/dev/null 2>&1 || { \
			echo "ERROR: $(OPT_CC) cannot compile the required MIPS II/VR4300 configuration."; exit 1; \
		}; \
		rm -f $(BUILD_DIR)/.opt-compiler-probe.o; \
	fi
endif

# Convenience entry point. This recursive make is intentional: variables such
# as OUTCODE/BUILD_DIR are selected while parsing the optimized sub-build.
optimized:
	$(MAKE) OPTIMIZED_ROM=YES COMPARE=0 all

optimized-clean:
	$(MAKE) OPTIMIZED_ROM=YES COMPARE=0 clean

# Build the KSEG0/physical-code architecture branch.
physical:
	$(MAKE) PHYSICAL_CODE=YES COMPARE=0 all

# Fast source-side guard.  The linked ELF audit below remains authoritative for
# resolved function-pointer tables and final jump targets.
physical-source-audit:
ifeq ($(PHYSICAL_CODE), YES)
	python3 scripts/audit_physical_source.py
else
	@echo "ERROR: physical-source-audit requires PHYSICAL_CODE=YES" >&2
	@exit 2
endif

# Audit the linked physical ELF and finalized ROM.  This checks the actual
# runtime sections/function-pointer values rather than raw ROM byte patterns.
physical-audit: $(APPELF) $(APPROM)
ifeq ($(PHYSICAL_CODE), YES)
	python3 scripts/audit_physical_code.py --elf $(APPELF) --rom $(APPROM)
else
	@echo "ERROR: physical-audit requires PHYSICAL_CODE=YES" >&2
	@exit 2
endif

physical-clean:
	$(MAKE) PHYSICAL_CODE=YES COMPARE=0 clean

combine_images: assets/images/combined/combined.bin

checksum: $(APPROM)
ifeq ($(COMPARE), 1)
	scripts/make/checksum.sh "$(SHA1SUM)" "$(OUTCODE)" "$(BUILD_DIR)"
endif

ifeq ($(OPTIMIZED_ROM), YES)
all_p1: optimized-preflight
endif
all_p1: prerequisites
ifeq ($(PHYSICAL_CODE), YES)
all_p1: physical-source-audit
all: physical-audit
endif
all: all_p1 $(APPROM) checksum
	@echo "Rom File Generated in Build Directory."

commonclean:
	rm -f $(APPELF) $(APPROM) $(APPBIN) $(BUILD_DIR)/ge007.$(OUTCODE).map

setupclean: commonclean
	rm -f $(SETUP_BUILD_FILES)

stanclean: commonclean
	rm -f $(STAN_BUILD_FILES)

dataclean: commonclean stanclean setupclean
	rm -f $(OBSEG_OBJECTS) $(OBSEG_RZ) $(ROMOBJECTS) $(RAMROM_OBJECTS) $(FONTOBJECTS) $(MUSIC_OBJECTS) $(IMAGE_OBJS) $(MUSIC_RZ_FILES)
	rm -f $(BUILD_DIR)/imagelist.csv

libultraclean: commonclean
	rm -f $(ULTRAOBJECTS)

codeclean: commonclean libultraclean
	rm -f $(HEADEROBJECTS) $(BOOTOBJECTS) $(CODEOBJECTS) $(GAMEOBJECTS) $(RZOBJECTS) $(RSPOBJECTS)

clean: codeclean dataclean
	@echo "\nAll Code and Asset Binaries Cleared! Make will Re-Build these next time.\n"

nuke: clean
	scripts/make/clean_nuke.sh "$(ALLOWED_COUNTRYCODE)" "$(BUILD_DIR_BASE)"

help:
	@echo "mmakefile help"
	@echo ""
	@echo "  supported targets:"
	@echo ""
	@echo "    all                            Build all (default)"
	@echo "    optimized                      Build a separate play-testable GCC optimized ROM"
	@echo "    optimized-clean                Clean only the optimized build output for this version"
	@echo "    clean                          Delete all known build artifacts"
	@echo "    nuke                           Delete all files explicitly listed in Makefile (same as make clean),"
	@echo "                                    all build output for all versions, any .bin file in assets folders,"
	@echo "                                    and asp/rsp bin."
	@echo "    dataclean                      Delete only asset build artifacts"
	@echo "    codeclean                      Delete only code (asm, .c) build artifacts"
	@echo "    libultraclean                  Delete only code (asm, .c) build artifacts "
	@echo "                                    from Rare's libultra files"
	@echo "    stanclean                      Delete only stan build artifacts"
	@echo "    setupclean                     Delete only setup build artifacts"
	@echo "    cmdbuidler                     BuildAI Commands"
	@echo "    context [file]                 BuildContext File from [file]"
	@echo "                                    eg make context src/game/chrai.c"
	@echo "    test                            Re-Run Data Verification "
	@echo ""
	@echo "  optimized options:"
	@echo ""
	@echo "    OPTIMIZED_LEVEL=2              GCC optimization level (start with 2 for validation)"
	@echo "    OPT_CC=/path/to/gcc            Override modern MIPS GCC executable"
	@echo ""
	@echo ""
	@echo "  options:"
	@echo ""
	@echo "    VERSION=v                       Region version. (US is default)"
	@echo "                                    Supported values: ${ALLOWED_VERSIONS}\n"

include include/make/cmd.make


test: checksum


ifneq ($(filter-out context,$(MAKECMDGOALS)),)
 CONTEXTFILE := $(filter-out context ,$(MAKECMDGOALS))
else
 CONTEXTFILE := build/ctx.c
endif
context:
	@clear
	@echo Building Context File [ctx.h] from $(CONTEXTFILE)
	@echo "#define TRUE 1" > build/ctx.h
	@echo "#define FALSE 0" >> build/ctx.h
ifeq ($(CONTEXTFILE),build/ctx.c)
	@echo "#include <bondtypes.h>" > build/ctx.c
endif
	@sed -n -E ':x /\\$$/ { N; s/\\\n//g ; bx };''/(^\s*#define)|(\\$$)/p; /(\\$$)/p;' src/bondconstants.h src/bondtypes.h $(CONTEXTFILE) >> build/ctx.h
	@$(CC) -c $(CFLAGS) $(CONTEXTFILE) -E > build/ctx2.h 2> /dev/null || (rm build/ctx2.h && exit 1)
	@sed -E '/^\s*$$/d' build/ctx2.h >> build/ctx.h
	@rm build/ctx.c build/ctx2.h || exit 0
	@echo You can find it in Build [build/ctx.h].

extractassets: extract_u extract_e extract_j convert_props convert_chrs convert_guns

forceextractassets: force_extract_u force_extract_e force_extract_j convert_props convert_chrs convert_guns

extract_u:
	@if [ ! -f assets/obseg/ob__ob_end.seg ]; then \
		echo "Extracting assets for u..."; \
		if [ -f baserom.u.z64 ]; then \
			scripts/extract_baserom.u.sh; \
		else \
			echo "Error: baserom.u.z64 not found."; \
		fi \
	else \
		echo "Assets for u already extracted."; \
	fi

force_extract_u:
	@echo "Force extracting assets for u..."; \
	if [ -f baserom.u.z64 ]; then \
		scripts/extract_baserom.u.sh; \
	else \
		echo "Error: baserom.u.z64 not found."; \
	fi

extract_e:
	@if [ ! -f assets/obseg/text/e/LwaxP.bin ]; then \
		echo "Extracting assets for e..."; \
		if [ -f baserom.e.z64 ]; then \
			scripts/extract_diff.e.sh; \
		else \
			echo "Error: baserom.e.z64 not found."; \
		fi \
	else \
		echo "Assets for e already extracted."; \
	fi

force_extract_e:
	@echo "Force extracting assets for e..."; \
	if [ -f baserom.e.z64 ]; then \
		scripts/extract_diff.e.sh; \
	else \
		echo "Error: baserom.e.z64 not found."; \
	fi

extract_j:
	@if [ ! -f assets/obseg/text/j/LstatJ.bin ]; then \
		echo "Extracting assets for j..."; \
		if [ -f baserom.j.z64 ]; then \
			scripts/extract_diff.j.sh; \
		else \
			echo "Error: baserom.j.z64 not found."; \
		fi \
	else \
		echo "Assets for j already extracted."; \
	fi

force_extract_j:
	@echo "Force extracting assets for j..."; \
	if [ -f baserom.j.z64 ]; then \
		scripts/extract_diff.j.sh; \
	else \
		echo "Error: baserom.j.z64 not found."; \
	fi

extract_rsp:
	@if [ ! -f build/u/rsp/rspboot.bin ]; then \
		echo "Extracting rsp assets..."; \
		if [ -f baserom.u.z64 ]; then \
			scripts/extract_asp_gsp_rsp.sh; \
		else \
			echo "Error: baserom.u.z64 not found."; \
		fi \
	else \
		echo "RSP assets for already extracted."; \
	fi

convert_props:
	@echo "Converting prop binaries to Model.c..."
	@bin_count=$$(ls assets/obseg/prop/P*Z.bin 2>/dev/null | wc -l); \
	if [ $$bin_count -gt 0 ]; then \
		echo "Found $$bin_count prop binaries to convert..."; \
		python3 scripts/generate_prop_model_c.py --force --cleanup || true; \
	else \
		c_count=$$(find assets/obseg/prop -maxdepth 2 -name "Model.c" 2>/dev/null | wc -l); \
		if [ $$c_count -gt 0 ]; then \
			echo "Props already converted ($$c_count Model.c files found)."; \
		else \
			echo "No prop binaries found to convert."; \
		fi \
	fi

convert_chrs:
	@echo "Converting chr binaries to Model.c..."
	@bin_count=$$(ls assets/obseg/chr/C*Z.bin 2>/dev/null | wc -l); \
	if [ $$bin_count -gt 0 ]; then \
		echo "Found $$bin_count chr binaries to convert..."; \
		python3 scripts/generate_chr_c.py --force --cleanup || true; \
	else \
		c_count=$$(find assets/obseg/chr -maxdepth 2 -name "Model.c" 2>/dev/null | wc -l); \
		if [ $$c_count -gt 0 ]; then \
			echo "Chrs already converted ($$c_count Model.c files found)."; \
		else \
			echo "No chr binaries found to convert."; \
		fi \
	fi

convert_guns:
	@echo "Converting gun binaries to Model.c..."
	@bin_count=$$(ls assets/obseg/gun/G*Z.bin 2>/dev/null | wc -l); \
	if [ $$bin_count -gt 0 ]; then \
		echo "Found $$bin_count gun binaries to convert..."; \
		python3 scripts/generate_gun_c.py --force --cleanup || true; \
	else \
		c_count=$$(find assets/obseg/gun -maxdepth 2 -name "Model.c" 2>/dev/null | wc -l); \
		if [ $$c_count -gt 0 ]; then \
			echo "Guns already converted ($$c_count Model.c files found)."; \
		else \
			echo "No gun binaries found to convert."; \
		fi \
	fi

textures: tools/mktex/build/tex2png
	@echo "Processing textures..."
	mkdir -p assets/images/out
	$(foreach x,$(IMAGE_BINS),tools/mktex/build/tex2png $(x) assets/images/out ${\n})


tools/mktex/build/tex2png:
	@if [ ! -f tools/mktex/build/tex2png ]; then \
		echo "Building tex2png..."; \
		cd tools/mktex && $(MAKE); \
	fi
