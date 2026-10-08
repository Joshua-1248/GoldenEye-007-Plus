#ifndef GE_ROM_SIZE
#define GE_ROM_SIZE 0x00C00000
#endif

#define BEGIN_SEG(name, rompos, rampos) \
	_##name##SegmentStart = ADDR(.name); \
	_##name##SegmentRomStart = rompos; \
	.name rampos : AT(rompos)

#define END_SEG(name) \
	_##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
	_##name##SegmentRomEnd = _##name##SegmentRomStart + SIZEOF(.name);

/*OUTPUT_FORMAT ("elf32-bigmips")*/
OUTPUT_ARCH (mips)

SECTIONS
{
   
	osTvType = 0x80000300;
	osRomType = 0x80000304;
	osRomBase = 0x80000308;
	osResetType = 0x8000030c;
	osCicId = 0x80000310;
	osVersion = 0x80000314;
	osMemSize = 0x80000318;
	osAppNMIBuffer = 0x8000031c;

	_romSegmentStart = 0;
   /* header */
   BEGIN_SEG(header, _romSegmentStart, 0x04000000)
   {
      build/OUTCODE/src/rom_header.o(.data);
   }
   END_SEG(header)



   /* boot */
   BEGIN_SEG(boot, _headerSegmentRomEnd, 0x04000040)
   {
      build/OUTCODE/src/bootcode.o(.text);
   }
   END_SEG(boot)




   /* 0x80000400, 0x80000450 */
   BEGIN_SEG(start, _bootSegmentRomEnd, 0x80000400)
   {
      build/OUTCODE/src/_start.o (.text);
   }
   END_SEG(start)


   _alt_startSegmentRomStart = _startSegmentRomStart + 0x100000;
#if defined(GE_PHYSICAL_CODE)
   _alt_startSegmentStart = _startSegmentStart + 0x100000;
#else
   _alt_startSegmentStart = _startSegmentStart - 0x10000000 + 0x100000;
#endif
   

   /* Retail executes this region through a 0x70000000 TLB mapping.
    * GE_PHYSICAL_CODE links the identical resident code directly in KSEG0. */
#if defined(GE_PHYSICAL_CODE)
   BEGIN_SEG(code, _startSegmentRomEnd, _startSegmentEnd) SUBALIGN(16)
#else
   BEGIN_SEG(code, _startSegmentRomEnd, (_startSegmentEnd - 0x10000000)) SUBALIGN(16)
#endif
   {
#include "ld/lib.text.ld.inc"
   }
   END_SEG(code)
#if defined(GE_PHYSICAL_CODE)
   ASSERT(_codeSegmentStart == 0x80000450, "physical resident code start drifted")
#endif


#if defined(GE_PHYSICAL_CODE)
   /* Keep every ROM segment from cdata onward at its retail offset.
    * Physical mode shrinks the resident boot/code block by 0x260 bytes, but
    * changing the following ROM offsets would needlessly invalidate hardcoded
    * cartridge-address assumptions and make binary auditing harder. */
   ASSERT(_codeSegmentRomEnd <= 0x21990, "physical resident code exceeds retail cdata ROM start")
   BEGIN_SEG(cdata, 0x21990, 0x21990)
#else
   BEGIN_SEG(cdata, _codeSegmentRomEnd, _codeSegmentRomEnd)
#endif
   {
      build/OUTCODE/src/c_data_filler.o (.data);
   }
   END_SEG(cdata)



   /* Retail inflate code runs at 0x70200000 through the boot TLB mapping.
    * Physical mode uses the same physical offset through KSEG0 instead. */
#if defined(GE_PHYSICAL_CODE)
   BEGIN_SEG(inflate, _cdataSegmentRomEnd, 0x80200000)
#else
   BEGIN_SEG(inflate, _cdataSegmentRomEnd, 0x70200000)
#endif
   {
      build/OUTCODE/src/inflate/inflate.o (.text);
      build/OUTCODE/src/inflate/inflate.o (.data);
   }
   END_SEG(inflate)
#if defined(GE_PHYSICAL_CODE)
   ASSERT(_inflateSegmentStart == 0x80200000, "physical inflate segment start drifted")
#endif





   /* Retail game text is demand-paged at 0x7F000000.
    * Physical mode requires the Expansion Pak and keeps the entire segment
    * resident in the upper half of 8 MiB RDRAM.  0x80600000 deliberately
    * leaves the retail 4 MiB memory map untouched and reserves 2 MiB for
    * physical game code plus future optimized growth. */
#if defined(GE_PHYSICAL_CODE)
   BEGIN_SEG(game, _inflateSegmentRomEnd, 0x80600000)
#else
   BEGIN_SEG(game, _inflateSegmentRomEnd, 0x7F000000)
#endif
   {
#include "ld/game.text.ld.inc"
   }
   END_SEG(game)
#if defined(GE_PHYSICAL_CODE)
   /* Persistent optimized-build caches live entirely in the Expansion Pak
    * below resident game code. Retail never uses 0x80400000..0x80600000.
    * R11 no longer needs a 256 KiB gun-model staging buffer: weapon texture
    * IDs are generated from the decomp's model tables at build time. */
   _weaponTexCacheStart = 0x80400000;
#if defined(GE_MAP_MAKER)
   /* Map Maker borrows this front-end-only Expansion-Pak window instead of
    * consuming retail .bss / stage heap. Weapon prewarm owns it again after
    * stage loading begins, and the editor validates a scratch magic on entry. */
   _mapMakerScratchStart = _weaponTexCacheStart;
   _mapMakerScratchEnd = 0x80440000;
   ASSERT(_mapMakerScratchEnd <= _weaponTexCacheEnd, "Map Maker scratch exceeds weapon texture cache window")
#endif
   /* R22 stress fix: Perfect Dark can add extra master-display-list memory
    * for Co-Op (its -mgfxtra path). GoldenEye Plus needs the same headroom
    * when campaign rooms full of guards are rendered into 3-4 viewports.
    * A four-crowd-view RDRAM dump measured a 233,160-byte master list, 3,784
    * bytes beyond the previous 224 KiB 4P GFX half. Extend Co-Op scratch by
    * 48 KiB using the existing alignment gap before the persistent font cache;
    * shift the MP left-hand buffers upward by the same amount. */
   _weaponTexCacheEnd = 0x804A0000;
   _coopDynBuffersStart = 0x804A0000;
   _coopDynBuffersEnd = 0x8056C000;
   _mpLeftWeaponBuffersStart = 0x8056C000;
   _mpLeftWeaponBuffersEnd = 0x805BE080;
   _fontCacheStart = 0x805C0000;
   _fontCacheEnd = 0x805C8000;
   _resourceNameHashStart = 0x805C8000;
   _resourceNameHashEnd = 0x805C9004;

   /* Plus Third Person: two disposable per-hand weapon caches.  TP21 used
    * modelLoad() for visible body weapons, which permanently accumulated
    * each new PROP_CHR* model and its decoded textures in the stage pools.
    * Reuse the previously-reserved load-scratch window instead: each hand
    * receives 0x1B000 bytes (0x8000 model + 0x13000 textures), and the slot
    * is rebuilt only after that hand's previous prop has been released. */
   _thirdPersonWeaponCacheStart = 0x805CA000;
   _thirdPersonWeaponCacheEnd = 0x80600000;
   _loadScratchStart = _thirdPersonWeaponCacheStart;
   _loadScratchEnd = _thirdPersonWeaponCacheEnd;
   ASSERT(_weaponTexCacheStart >= 0x80400000, "weapon texture cache overlaps retail 4 MiB RDRAM")
   ASSERT((_weaponTexCacheEnd - _weaponTexCacheStart) >= 0xA0000, "weapon texture cache is too small")
   ASSERT(_weaponTexCacheEnd <= _coopDynBuffersStart, "weapon texture cache overlaps Co-Op dynamic buffers")
   ASSERT((_coopDynBuffersEnd - _coopDynBuffersStart) == 0xCC000, "Co-Op dynamic buffer reservation drifted")
   ASSERT(_coopDynBuffersEnd <= _mpLeftWeaponBuffersStart, "Co-Op dynamic buffers overlap multiplayer left-hand buffers")
   ASSERT((_mpLeftWeaponBuffersEnd - _mpLeftWeaponBuffersStart) >= (0x14820 * 4), "multiplayer left-hand buffers are too small")
   ASSERT(_mpLeftWeaponBuffersEnd <= _fontCacheStart, "multiplayer left-hand buffers overlap font cache")
   ASSERT(_fontCacheEnd <= _resourceNameHashStart, "persistent font cache overlaps resource hash")
   ASSERT(_resourceNameHashEnd <= _thirdPersonWeaponCacheStart, "resource-name hash overlaps Third Person weapon cache")
   ASSERT((_thirdPersonWeaponCacheEnd - _thirdPersonWeaponCacheStart) == 0x36000, "Third Person weapon cache reservation drifted")
   ASSERT(_thirdPersonWeaponCacheEnd <= _gameSegmentStart, "Third Person weapon cache overlaps game code")

   /* Optimization-only runtime scratch lives above resident game text.  This
    * is NOLOAD-style reserved RDRAM, intentionally outside retail's 4 MiB
    * data/BSS map. */
   _gameFastBssStart = ALIGN(_gameSegmentEnd, 16);
   _gameFastBgCacheEnd = _gameFastBssStart + 0x2200;
   _gameFastPropRoomCacheStart = _gameFastBgCacheEnd;
   _gameFastPropRoomCacheEnd = _gameFastBssStart + 0x2400;
   _gameFastBssEnd = _gameFastPropRoomCacheEnd;
   ASSERT((_gameFastPropRoomCacheEnd - _gameFastPropRoomCacheStart) == 0x200, "prop render-room cache reservation drifted")

   /* R22: the retail stage texture pool has a deliberate low-memory fallback
    * which points materials at the start of the pool (garbage texels) rather
    * than crashing.  Keep the retail lower-4-MiB stage heap unchanged and use
    * otherwise-unused Expansion Pak RDRAM as a secondary stage texture pool.
    * 128 KiB is enough for late character/prop texture bursts without stealing
    * space from gameplay allocations in the retail memory map. */
   _stageTexOverflowStart = ALIGN(_gameFastBssEnd, 16);
   _stageTexOverflowEnd = _stageTexOverflowStart + 0x20000;
   ASSERT((_stageTexOverflowEnd - _stageTexOverflowStart) == 0x20000, "stage texture overflow reservation drifted")
   ASSERT(_stageTexOverflowEnd <= 0x80800000, "stage texture overflow exceeds 8 MiB RDRAM")
   ASSERT(_gameFastBssEnd <= _stageTexOverflowStart, "stage texture overflow overlaps physical fast BSS")

   /* Plus Third Person body/head cache.  A live TP player cannot keep Bond's
    * model in the ordinary first-person hand buffers because normal weapon
    * cycling reuses them.  Loading the body/head through fileLoad() instead
    * consumes MEMPOOL_STAGE permanently, which can exhaust memory on heavy
    * stages (Frigate is the reproducible case) and GoldenEye then spins in
    * the allocator failure loop.  Reserve two stock-sized disposable buffers
    * above the stage texture overflow pool: one for model data and one for
    * decoded body/head textures.  Cinematics continue to use the stock hand
    * buffers, while live TP gameplay reuses this cache every time the body is
    * rebuilt. */
   _thirdPersonBodyCacheStart = ALIGN(_stageTexOverflowEnd, 16);
   _thirdPersonBodyCacheEnd = _thirdPersonBodyCacheStart + 0x2A000;
   ASSERT((_thirdPersonBodyCacheEnd - _thirdPersonBodyCacheStart) == 0x2A000, "Third Person body cache reservation drifted")
   ASSERT(_thirdPersonBodyCacheEnd <= 0x80800000, "Third Person body cache exceeds 8 MiB RDRAM")
   ASSERT(_stageTexOverflowEnd <= _thirdPersonBodyCacheStart, "Third Person body cache overlaps stage texture overflow")

   /*
    * R27I: immutable animation-frame cache.
    *
    * Keep this separate from the R27G2 0x2400 render/visibility scratch and
    * from all retail lower-4-MiB state.  64 x 80-byte data slots plus
    * metadata/replacement state fit in 0x1700 bytes.
    */
   _animFrameCacheStart = ALIGN(_thirdPersonBodyCacheEnd, 16);
   _animFrameCacheEnd = _animFrameCacheStart + 0x1700;
   ASSERT((_animFrameCacheEnd - _animFrameCacheStart) == 0x1700, "animation frame cache reservation drifted")
   ASSERT(_thirdPersonBodyCacheEnd <= _animFrameCacheStart, "animation frame cache overlaps Third Person body cache")
   ASSERT(_animFrameCacheEnd <= 0x80800000, "animation frame cache exceeds 8 MiB RDRAM")

   /*
    * R27L: exact model sin/cos memoization cache.
    *
    * 256 entries x 16 bytes + 128 replacement bytes = 0x1080;
    * reserve 0x1100 for alignment/headroom.
    */
   _modelTrigCacheStart = ALIGN(_animFrameCacheEnd, 16);
   _modelTrigCacheEnd = _modelTrigCacheStart + 0x1100;
   ASSERT((_modelTrigCacheEnd - _modelTrigCacheStart) == 0x1100, "model trig cache reservation drifted")
   ASSERT(_animFrameCacheEnd <= _modelTrigCacheStart, "model trig cache overlaps animation frame cache")
   ASSERT(_modelTrigCacheEnd <= 0x80800000, "model trig cache exceeds 8 MiB RDRAM")

   /*
    * R27O R3: expanded explosion storage must not consume MEMPOOL_STAGE.
    * struct Explosion is 0x3E0 bytes in the N64 ABI; 64 entries = 0xF800.
    */
   _unlimitedExplosionPoolStart = ALIGN(_modelTrigCacheEnd, 16);
   _unlimitedExplosionPoolEnd = _unlimitedExplosionPoolStart + 0xF800;
   ASSERT((_unlimitedExplosionPoolEnd - _unlimitedExplosionPoolStart) == 0xF800, "Unlimited Explosions pool reservation drifted")
   ASSERT(_modelTrigCacheEnd <= _unlimitedExplosionPoolStart, "Unlimited Explosions pool overlaps model trig cache")
   ASSERT(_unlimitedExplosionPoolEnd <= 0x80800000, "Unlimited Explosions pool exceeds 8 MiB RDRAM")

   /* R30 P5: Simulant body/head model-file cache.  Bot characters are selected
    * freely and therefore cannot rely on the retail stage having preloaded the
    * corresponding chr resources.  Keep those late loads out of MEMPOOL_STAGE. */
   _modMpBotModelCacheStart = ALIGN(_unlimitedExplosionPoolEnd, 16);
   _modMpBotTextureCacheEnd = 0x80800000;
   _modMpBotTextureCacheStart = _modMpBotTextureCacheEnd - 0x10000;
   _modMpBotModelCacheEnd = _modMpBotTextureCacheStart;
   ASSERT((_modMpBotTextureCacheEnd - _modMpBotTextureCacheStart) == 0x10000, "Simulant texture cache reservation drifted")
   ASSERT((_modMpBotModelCacheEnd - _modMpBotModelCacheStart) >= 0x5E000, "Simulant model cache too small")
   ASSERT(_unlimitedExplosionPoolEnd <= _modMpBotModelCacheStart, "Simulant model cache overlaps explosion pool")
   ASSERT(_modMpBotModelCacheEnd <= _modMpBotTextureCacheStart, "Simulant model cache overlaps texture cache")
   ASSERT(_modMpBotTextureCacheEnd <= 0x80800000, "Simulant texture cache exceeds 8 MiB RDRAM")
   ASSERT(_gameSegmentStart == 0x80600000, "physical game segment start drifted")
   ASSERT(_gameSegmentRomStart == 0x00034B30, "physical game ROM start drifted from retail 0x34B30")
   ASSERT(_gameSegmentStart >= 0x80400000, "physical game segment must live in Expansion Pak RDRAM")
   ASSERT(_gameSegmentEnd <= 0x80800000, "physical game segment exceeds 8 MiB RDRAM")
#endif


   BEGIN_SEG(fontdl, _gameSegmentRomEnd, 0x01000000)
   {
      build/OUTCODE/assets/font_dl.o (.data);
   }
   END_SEG(fontdl)

   BEGIN_SEG(jfontchardata, _fontdlSegmentRomEnd, 0x00000000)
   {
      build/OUTCODE/assets/font_chardataj.o (.data);
   }
   END_SEG(jfontchardata)

   BEGIN_SEG(efontchardata, _jfontchardataSegmentRomEnd, 0x00000000)
   {
      build/OUTCODE/assets/font_chardatae.o (.data);
   }
   END_SEG(efontchardata)
   
   BEGIN_SEG(animation_entries, _efontchardataSegmentRomEnd, 0x00000000)
   {
      build/OUTCODE/assets/animationtable_entries.o(.data);
   }
   END_SEG(animation_entries)

   BEGIN_SEG(animation_data, _animation_entriesSegmentRomEnd, 0x00000000)
   {
      build/OUTCODE/assets/animationtable_data.o(.data);
   }
   END_SEG(animation_data)

   BEGIN_SEG(Globalimagetable, _animation_dataSegmentRomEnd, 0x02000000)
   {
      build/OUTCODE/assets/oddtextures.o (.data);
   }
   END_SEG(Globalimagetable)

   BEGIN_SEG(rarewarelogo, _GlobalimagetableSegmentRomEnd, 0x02000000)
   {
      build/OUTCODE/assets/rarewarelogo.o (.data);
   }
   END_SEG(rarewarelogo)

   BEGIN_SEG(romfiles2, _rarewarelogoSegmentRomEnd, _romfiles2SegmentRomStart)
   {
      build/OUTCODE/assets/romfiles2.o (.data);
   }
   END_SEG(romfiles2)

   BEGIN_SEG(ramromfiles, _romfiles2SegmentRomEnd, _ramromfilesSegmentRomStart)
   {
      build/OUTCODE/assets/ramrom/ramrom.o (.data);
   }
   END_SEG(ramromfiles)


   BEGIN_SEG(fontbankgothic, _ramromfilesSegmentRomEnd, 0)
   {
      build/OUTCODE/assets/font/fontBankGothic.o (.data);
   }
   END_SEG(fontbankgothic)

   BEGIN_SEG(fontzurichbold, _fontbankgothicSegmentRomEnd, 0)
   {
      build/OUTCODE/assets/font/fontZurichBold.o (.data);
   }
   END_SEG(fontzurichbold)



   BEGIN_SEG(musicfiles, _fontzurichboldSegmentRomEnd, _musicfilesSegmentRomStart)
   {
      build/OUTCODE/assets/music/sfx.ctl.o (.data);
      build/OUTCODE/assets/music/sfx.tbl.o (.data);
      build/OUTCODE/assets/music/instruments.ctl.o (.data);
      build/OUTCODE/assets/music/instruments.tbl.o (.data);
      build/OUTCODE/assets/music/music.o (.music);
      build/OUTCODE/assets/music/music.o (.musiccompressed);
   }
   END_SEG(musicfiles)



   BEGIN_SEG(obseg, _musicfilesSegmentRomEnd, _obsegSegmentRomStart)
   {
      build/OUTCODE/assets/obseg/ob_seg.o (.data);
   }
   END_SEG(obseg)



   BEGIN_SEG(images, _obsegSegmentRomEnd, _imagesSegmentRomStart)
   {
#include "ld/images.ld.inc"
   }
   END_SEG(images)



	_romSegmentEnd = _imagesSegmentRomEnd;
#if defined(GE_PHYSICAL_CODE)
   ASSERT(_romSegmentEnd <= GE_ROM_SIZE, "physical ROM content exceeds configured cartridge size")
#endif



   __csegtempPos = GE_ROM_SIZE;
#if defined(GE_PHYSICAL_CODE)
   /* Preserve the retail data/BSS ABI even though resident text now uses KSEG0. */
   __dataSegmentVaddrStart = 0x80020D90;
   ASSERT(_codeSegmentEnd <= __dataSegmentVaddrStart, "physical resident code overlaps data segment")
#else
   __dataSegmentVaddrStart = _codeSegmentEnd + 0x10000000;
#endif

   BEGIN_SEG(csegment, __csegtempPos, __dataSegmentVaddrStart)
   {
      build/OUTCODE/src/rspboot.o (.text);
      build/OUTCODE/src/gspboot.o (.text);
      build/OUTCODE/src/aspboot.o (.text);
#include "ld/lib.data.ld.inc"
#include "ld/lib.rodata.ld.inc"
#include "ld/game.data.ld.inc"
#include "ld/game.rodata.ld.inc"
      build/OUTCODE/src/gspboot.o (.data);
      build/OUTCODE/src/aspboot.o (.data);
   }
   END_SEG(csegment)

   _dataSegmentVaddrEnd = _csegmentSegmentRomEnd;



   _bssSegmentStart = ADDR(.bss);
   .bss (NOLOAD) :
   {
#include "ld/lib.bss.ld.inc"
#include "ld/game.bss.ld.inc"
   }
   _bssSegmentEnd = ADDR(.bss) + SIZEOF(.bss);
   _bssSegmentSize = SIZEOF(.bss);

#if defined(GE_PHYSICAL_CODE)
   ASSERT(_bssSegmentEnd < 0x80400000, "physical data/BSS exceeds original 4 MiB RDRAM map")
   ASSERT(_bssSegmentEnd < _gameSegmentStart, "physical game text overlaps data/BSS")
   ASSERT(_cfbSegmentEnd <= 0x80400000, "physical stacks/framebuffers exceed original 4 MiB RDRAM map")
   ASSERT(_cfbSegmentEnd <= _gameSegmentStart, "physical game text overlaps retail 4 MiB map")
#endif

   _stacksSegmentStart = ADDR(.stacks);
#if defined(VERSION_EU)
   .stacks 0x803A1AB0 : 
#else
   .stacks 0x803AB400 :
#endif
   {
      build/OUTCODE/src/stacks.o (.bss);
   }
   _stacksSegmentEnd = ADDR(.stacks) + SIZEOF(.stacks);
#if defined(GE_PHYSICAL_CODE)
   ASSERT(_bssSegmentEnd <= _stacksSegmentStart, "physical data/BSS steals fixed stack memory")
#endif

   _cfbSegmentStart = ADDR(.cfb);
   .cfb (NOLOAD) :
   {
      build/OUTCODE/src/cfb.o (.bss);
   }
   _cfbSegmentEnd = ADDR(.cfb) + SIZEOF(.cfb);

   /*move em to end, out of the way*/
   /DISCARD/ : {
      * (.MIPS.abiflags);
      * (.musicdecompressed);
      * (.options);
      * (.gnu.attributes);
      * (.pdr);
      * (.mdebug);
      * (.gptab.bss);
      * (.gptab.data);
      * (.reginfo);
   }
}
