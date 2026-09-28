# GoldenEye 007 Plus — N64 Flashcart Compatibility

GoldenEye 007 Plus uses **EEPROM 16 Kbit (2 KiB)** as its default cartridge-save backend.
This is the N64 save-type name commonly shown as `EEPROM16K`, `EEPROM 16K`, or
`EEPROM 16Kbit` in flashcart menus. It is **not 16 KiB**.

## V84 compatibility design

GoldenEye 007 Plus now identifies modded ROMs with the established N64 homebrew
flashcart header:

- ROM ID at offsets `0x3c..0x3d`: `ED`
- region byte at `0x3e`: the normal build region (`E` for NTSC-U)
- configuration byte at `0x3f`:
  - `0x20` = EEPROM 16 Kbit / 2 KiB (default Plus build)
  - `0x30` = SRAM 256 Kbit / 32 KiB (optional `SAVE_SRAM=YES` build)

This avoids masquerading as retail GoldenEye (`GE`), whose normal save type is
EEPROM 4 Kbit. Retail/non-modded builds keep the original GoldenEye `GE` header.

The EEPROM16 backend also uses the later libultra-style Joybus transaction
sequence used by 16-Kbit EEPROM titles: 4K/16K type discrimination, an intact
command-to-response DMA transaction, no EEPROM write-cycle delay after reads,
and the normal write-cycle delay after writes.

## SummerCart64

Use current SummerCart64 firmware and a current N64FlashcartMenu release.
SummerCart64 supports EEPROM 4K/16K, SRAM and FlashRAM. N64FlashcartMenu supports
the homebrew header and should therefore select **EEPROM 16 Kbit automatically**
for a V84+ default Plus ROM.

If testing an older pre-V84 Plus ROM, manually select EEPROM 16 Kbit / 2 KiB.
Do not reuse a forced per-ROM override after moving to a V84+ ROM unless it
matches the new header.

## EverDrive-64 X-series

EverDrive-64 supports the same `ED` homebrew ROM ID convention. Its documented
configuration byte uses save type `2` for EEPROM 16K, so the Plus `0x20` header
requests EEPROM 16 Kbit automatically.

For old OS versions or pre-V84 Plus ROMs, the fallback is a manual/save-database
entry selecting EEPROM 16K. V84+ should not require a GoldenEye-specific save
override.

## 64drive and other flashcarts

Modern menus which understand the established homebrew header can use the ROM's
embedded save recommendation. Menus which do not support it should be set to:

**EEPROM 16 Kbit / 2 KiB**

The game itself uses standard N64 Joybus EEPROM commands; there is no
SummerCart- or EverDrive-specific runtime code in GoldenEye 007 Plus.

## Hardware test matrix

For each flashcart, test with a fresh/blank save and with an existing save:

1. Boot without a manual save-type override.
2. Enter File Select and create/change a save.
3. Power-cycle the N64 fully.
4. Confirm the save reloads.
5. Change several Plus extended settings, power-cycle again, and confirm they persist.
6. Repeat with 1P and Co-op to catch unrelated startup regressions.

Record flashcart model, firmware/menu version, ROM SHA-256, and whether the save
was fresh or migrated. A successful emulator test is useful but does not replace
real hardware validation of Joybus timing and flashcart save emulation.
