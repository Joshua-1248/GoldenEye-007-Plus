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

/* R22/V33 options stored in the former tail-padding byte of save_data.
 * V33 reduces the options-format marker to the top two bits, reclaiming bit 5
 * for Directional Shoulder View Toggle without growing the EEPROM record.
 * The exact V32 0xa0 marker is migrated explicitly and defaults the new
 * option Off. */
#define MODOPT3_SIGNATURE_MASK          0xc0
#define MODOPT3_SIGNATURE               0xc0
#define MODOPT3_LEGACY_SIGNATURE_MASK   0xe0
#define MODOPT3_LEGACY_SIGNATURE_V32    0xa0
#define MODOPT3_DIRECTIONAL_SHOULDER    0x20
#define MODOPT3_TP_CROUCH_CAM           0x10
#define MODOPT3_ENABLE_MICROOPT          0x01
#define MODOPT3_EXPERIMENTAL_JUMP       0x02
/* Micro-optimizations stay Off by default.  TP Sight Translucency is stored
 * in the extended-settings journal, not in the camera-pack/signature bits. */
#define DEFAULT_MOD_OPTIONS3            MODOPT3_SIGNATURE

/* V29 Third Person camera settings are packed into bits which retail never
 * consumes, while preserving the fixed 0x60-byte EEPROM record. */
#define MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30  0x05
#define MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30B 0x06
#define MOD_CAMERA_PACK_SIGNATURE3             0x07
#define MOD_CAMERA_STAY_TP_DEATH_BIT  (1u << 26)

extern u8 g_ModGameplayOptions2;
extern u8 g_ModGameplayOptions3;
extern s32 g_ModAntiAliasingEnabled;
s32 modMicroOptimizationsEnabled(void);
void modSetMicroOptimizationsEnabled(s32 enabled);
s32 modExperimentalJumpEnabled(void);
void modSetExperimentalJumpEnabled(s32 enabled);
void fileStoreThirdPersonCameraSettings(save_data *save);
void fileLoadThirdPersonCameraSettings(save_data *save);
s32 fileLoadExtendedSettings(save_data *save);
void fileStoreExtendedSettings(save_data *save);

extern ChrRecord *g_CurModelChr;
void fileWriteSave(save_data *save);

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
