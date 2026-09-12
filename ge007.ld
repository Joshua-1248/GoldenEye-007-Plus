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
   /* R8: reserve 640 KiB for Co-Op split-screen dynamic display-list and
    * vertex/matrix buffers.  The complete first-person texture prewarm uses
    * about 626 KiB, so a 768 KiB cache still retains comfortable headroom. */
   _weaponTexCacheEnd = 0x804C0000;
   _coopDynBuffersStart = 0x804C0000;
   _coopDynBuffersEnd = 0x80560000;
   _mpLeftWeaponBuffersStart = 0x80560000;
   _mpLeftWeaponBuffersEnd = 0x805B2080;
   _fontCacheStart = 0x805C0000;
   _fontCacheEnd = 0x805C8000;
   _resourceNameHashStart = 0x805C8000;
   _resourceNameHashEnd = 0x805C9004;
   _loadScratchStart = 0x805CA000;
   _loadScratchEnd = 0x80600000;
   ASSERT(_weaponTexCacheStart >= 0x80400000, "weapon texture cache overlaps retail 4 MiB RDRAM")
   ASSERT(_weaponTexCacheEnd <= _coopDynBuffersStart, "weapon texture cache overlaps Co-Op dynamic buffers")
   ASSERT((_coopDynBuffersEnd - _coopDynBuffersStart) >= 0xA0000, "Co-Op dynamic buffer reservation is too small")
   ASSERT(_coopDynBuffersEnd <= _mpLeftWeaponBuffersStart, "Co-Op dynamic buffers overlap multiplayer left-hand buffers")
   ASSERT((_mpLeftWeaponBuffersEnd - _mpLeftWeaponBuffersStart) >= (0x14820 * 4), "multiplayer left-hand buffers are too small")
   ASSERT(_mpLeftWeaponBuffersEnd <= _fontCacheStart, "multiplayer left-hand buffers overlap font cache")
   ASSERT(_fontCacheEnd <= _resourceNameHashStart, "persistent font cache overlaps resource hash")
   ASSERT(_resourceNameHashEnd <= _loadScratchStart, "resource-name hash overlaps load scratch")
   ASSERT(_loadScratchEnd <= _gameSegmentStart, "physical load scratch overlaps game code")

   /* Optimization-only runtime scratch lives above resident game text.  This
    * is NOLOAD-style reserved RDRAM, intentionally outside retail's 4 MiB
    * data/BSS map. */
   _gameFastBssStart = ALIGN(_gameSegmentEnd, 16);
   _gameFastBssEnd = _gameFastBssStart + 0x2400;
   ASSERT(_gameFastBssEnd <= 0x80800000, "physical optimization scratch exceeds 8 MiB RDRAM")
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
   ASSERT(_romSegmentEnd <= 0x00C00000, "physical ROM content exceeds native 12 MiB")
#endif



   __csegtempPos = 0xC00000;
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
