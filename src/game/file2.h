#ifndef _FILE2_H_
#define _FILE2_H_
#include <ultra64.h>

#include <bondconstants.h>
#include <bondtypes.h>
#include "file.h"



/* EEPROM masks for in-game settings */
#define OPTION_INVERTLOOK    0x0001
#define OPTION_AUTOAIM       0x0002
#define OPTION_AIMCONTROL    0x0004
#define OPTION_SIGHTONSCREEN 0x0008
#define OPTION_LOOKAHEAD     0x0010
#define OPTION_DISPLAYAMMO   0x0020
#define OPTION_SCREENWIDE    0x0040
#define OPTION_SCREENRATIO   0x0080
#define OPTION_CONTROLTYPE   0x0700
#define OPTION_SCREENCINEMA  0x0800
#define OPTION_HEADROLL      0x1000
#define OPTION_CROSSHAIR     0x2000
#define OPTION_R21_MIGRATED  0x4000
#define OPTION_SCREENRATIO2  0x8000

#define DEFAULT_OPTIONS (OPTION_AUTOAIM | OPTION_SIGHTONSCREEN | OPTION_LOOKAHEAD | OPTION_DISPLAYAMMO | OPTION_HEADROLL | OPTION_R21_MIGRATED)

/* R21 extended gameplay options stored in save_data.mod_options2 (formerly padding). */
#define MODOPT2_ENDLESS_DEATHCAM  0x01
#define MODOPT2_REALTIME_COLLAPSE 0x02
#define MODOPT2_DISABLE_HITSTUN   0x04
#define MODOPT2_DAMAGE_FLASH      0x08
#define MODOPT2_REVERSE_DEFAULT   0x10
#define MODOPT2_DISABLE_DAMAGE_SFX 0x20
#define MODOPT2_DISABLE_KNOCKBACK 0x40
#define MODOPT2_DISABLE_NOISE_DITHER 0x80
#define MODOPT2_R21_MIGRATED         0x80 /* legacy meaning, only while MODOPT3 signature is absent */
#define DEFAULT_MOD_OPTIONS2         (MODOPT2_REVERSE_DEFAULT | MODOPT2_DAMAGE_FLASH)

/* R22 options stored in the former tail-padding byte of save_data.
 * 0xa0 is a format signature so older saves cannot accidentally enable bits. */
#define MODOPT3_SIGNATURE_MASK       0xf0
#define MODOPT3_SIGNATURE            0xa0
#define DEFAULT_MOD_OPTIONS3         MODOPT3_SIGNATURE

extern u8 g_ModGameplayOptions2;

extern ChrRecord *g_CurModelChr;

u8 fileGetBondForFolder(u32 folder);
void fileValidateSaves(void);
bool fileGetIsCheatUnlocked(save_data *save, s32 cheat);
STAGESTATUS fileIsStageUnlockedAtDifficulty(s32 foldernum, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty);
void fileUnlockStageInFolderAtDifficulty(s32 foldernum, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty, s32 newtime);
void fileSaveFolderUnlockCheat(s32 foldernum, s32 cheat);
void fileUnlockEverythingInFolder(s32 foldernum);
void fileLoadSettingsForFolder(u32 folder);
void fileDeleteSaveForFolder(s32 foldernum);

void fileGetHighestStageDifficultyCompletedForFolder(s32 foldernum, LEVEL_SOLO_SEQUENCE *levelid, DIFFICULTY *difficulty);
bool check_aztec_completed_any_folder_secret_00(void);
bool fileIsEgyptCompletedOn00AnyFolder(void);
LEVEL_SOLO_SEQUENCE fileGetHighestStageUnlockedAnyFolder(void);

#endif
