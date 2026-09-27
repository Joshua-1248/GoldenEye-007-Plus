#include <ultra64.h>
#ifdef GE_SAVE_SRAM
#include <PR/rcp.h>
#include "libultra/io/piint.h"
#endif
#include <bondconstants.h>
#include "debugmenu_handler.h"
#include "joy.h"
#include "player.h"
#include "options.h"
#include "file.h"
#include "file2.h"
#include "front.h"
#include "cheat.h"


// bss
//CODE.bss:80069920
//CODE.bss:80069980
//CODE.bss:800699E0
//CODE.bss:80069A40
//CODE.bss:80069AA0
//CODE.bss:80069B00
save_data saves[SAVESLOTMAX];
typedef char save_data_size_must_remain_0x60[(sizeof(save_data) == 0x60) ? 1 : -1];

#ifdef GE_MODDED_CHEATS
u8 g_ModGameplayOptions2 = DEFAULT_MOD_OPTIONS2;
u8 g_ModGameplayOptions3 = DEFAULT_MOD_OPTIONS3;
s32 g_ModAntiAliasingEnabled = TRUE;

s32 modMicroOptimizationsEnabled(void)
{
    return (g_ModGameplayOptions3 & MODOPT3_ENABLE_MICROOPT) != 0;
}

void modSetMicroOptimizationsEnabled(s32 enabled)
{
    /* The gated passes are stateless/per-call optimizations.  Changing this
     * flag therefore takes effect at the next call boundary and requires no
     * stage reload, heap rebuild, pointer swap or cached-state teardown. */
    if (enabled)
        g_ModGameplayOptions3 |= MODOPT3_ENABLE_MICROOPT;
    else
        g_ModGameplayOptions3 &= ~MODOPT3_ENABLE_MICROOPT;
    g_ModGameplayOptions3 = (g_ModGameplayOptions3 & ~MODOPT3_SIGNATURE_MASK) | MODOPT3_SIGNATURE;
}

/*
 * V29: persist the four Third Person camera tuner values without enlarging
 * GoldenEye's legacy 512-byte save layout.  V35 mirrors this layout at the start of SRAM.  The 26-bit mixed-
 * radix payload is stored only in bits/bytes which retail never consumes:
 *   - unlocked_cheats_3 high nibble (only 20 retail unlock bits exist),
 *   - flag_007 bits 1..7 (bit 0 is the only live retail flag),
 *   - times[75] (600 time bits occupy times[0]..times[74]),
 *   - the struct's former final alignment byte, now mod_camera_tail,
 *   - mod_options3 bits 1..3 (bit 0 is Micro-optimizations).
 * Bits 27..29 carry a 3-bit signature. Bit 26 stores the save-backed
 * Stay In TP On Death default. Existing V29 saves used signature 0xA at
 * bits 26..29, which maps cleanly to signature3=0x5 and option bit 0.
 */
static u32 filePackThirdPersonCameraSettings(void)
{
    s32 distance = g_ModThirdPersonCameraDistanceAdjust;
    s32 height = g_ModThirdPersonCameraHeightAdjust;
    s32 horizontal = g_ModThirdPersonCameraHorizontalAdjust;
    s32 downframe = g_ModThirdPersonCameraDownFrameAdjust;
    u32 distance_index;
    u32 height_index;
    u32 horizontal_index;
    u32 downframe_index;
    u32 data;

    if (distance <= -127) distance_index = 0;
    else
    {
        if (distance < -126) distance = -126;
        if (distance > 60) distance = 60; /* legacy mirror limit; v3 extension carries full 600 range */
        distance_index = ((distance + 126) >> 1) + 1;
    }

    if (height < -48) height = -48;
    if (height > 72) height = 72;
    height_index = (height + 48) >> 1;

    if (horizontal < -60) horizontal = -60;
    if (horizontal > 100) horizontal = 100;
    horizontal_index = (horizontal + 60) >> 1;

    if (downframe < -24) downframe = -24;
    if (downframe > 72) downframe = 72;
    downframe_index = downframe + 24;

    data = distance_index;
    data = data * 61 + height_index;
    data = data * 81 + horizontal_index;
    data = data * 97 + downframe_index;

    return data
        | (g_ModStayInTpOnDeathDefault ? MOD_CAMERA_STAY_TP_DEATH_BIT : 0)
        | ((u32)MOD_CAMERA_PACK_SIGNATURE3 << 27);
}

void fileStoreThirdPersonCameraSettings(save_data *save)
{
    u32 packed;

    if (save == NULL)
        return;

    packed = filePackThirdPersonCameraSettings();

    save->unlocked_cheats_3 = (save->unlocked_cheats_3 & 0x0f) | ((packed & 0x0f) << 4);
    save->flag_007 = (save->flag_007 & 0x01) | (((packed >> 4) & 0x7f) << 1);
    save->times[(SP_LEVEL_MAX - 1) * 4 - 1] = (packed >> 11) & 0xff;
    save->mod_camera_tail = (packed >> 19) & 0xff;
    save->mod_options3 = (save->mod_options3 & 0xf1) | (((packed >> 27) & 0x07) << 1);
}

void fileLoadThirdPersonCameraSettings(save_data *save)
{
    u32 packed;
    u32 data;
    u32 distance_index;
    u32 height_index;
    u32 horizontal_index;
    u32 downframe_index;
    s32 i;

    if (save == NULL)
        return;

    packed = ((u32)(save->unlocked_cheats_3 >> 4) & 0x0f)
        | (((u32)(save->flag_007 >> 1) & 0x7f) << 4)
        | ((u32)save->times[(SP_LEVEL_MAX - 1) * 4 - 1] << 11)
        | ((u32)save->mod_camera_tail << 19)
        | (((u32)(save->mod_options3 >> 1) & 0x07) << 27);

    /* V30B migration: the original V29/V30 camera-persistence pass could
     * preserve stale session-local adjustment values from before the camera
     * defaults were finalized.  That is how an otherwise fresh menu could
     * show values such as Height 4 / Horizontal 72 / Down Frame 96 instead
     * of the authored defaults.
     *
     * Bump the 3-bit format signature once.  When a V30-format record is
     * encountered, preserve only Stay In TP On Death, reset all four camera
     * adjustments to zero, and immediately rewrite the folder in the new
     * format.  Zero adjustments are the authoritative defaults:
     * 240 / -12 / -24 / 24.  User changes made after this migration continue
     * to persist normally. */
    if (((packed >> 27) & 0x07) == MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30
        || ((packed >> 27) & 0x07) == MOD_CAMERA_PACK_SIGNATURE3_LEGACY_V30B)
    {
        g_ModThirdPersonCameraDistanceAdjust = 0;
        g_ModThirdPersonCameraHeightAdjust = 0;
        g_ModThirdPersonCameraHorizontalAdjust = 0;
        g_ModThirdPersonCameraDownFrameAdjust = 0;
        g_ModStayInTpOnDeathDefault = (packed & MOD_CAMERA_STAY_TP_DEATH_BIT) != 0;
        for (i = 0; i < MAX_PLAYER_COUNT; i++)
            g_PlayerStayInTpOnDeath[i] = g_ModStayInTpOnDeathDefault;
        fileStoreThirdPersonCameraSettings(save);
        fileWriteSave(save);
        return;
    }

    if (((packed >> 27) & 0x07) != MOD_CAMERA_PACK_SIGNATURE3)
    {
        g_ModThirdPersonCameraDistanceAdjust = 0;
        g_ModThirdPersonCameraHeightAdjust = 0;
        g_ModThirdPersonCameraHorizontalAdjust = 0;
        g_ModThirdPersonCameraDownFrameAdjust = 0;
        g_ModStayInTpOnDeathDefault = FALSE;
        for (i = 0; i < MAX_PLAYER_COUNT; i++) g_PlayerStayInTpOnDeath[i] = FALSE;
        fileStoreThirdPersonCameraSettings(save);
        fileWriteSave(save);
        return;
    }

    {
        g_ModStayInTpOnDeathDefault = (packed & MOD_CAMERA_STAY_TP_DEATH_BIT) != 0;
        for (i = 0; i < MAX_PLAYER_COUNT; i++)
            g_PlayerStayInTpOnDeath[i] = g_ModStayInTpOnDeathDefault;
    }

    data = packed & 0x03ffffff;
    downframe_index = data % 97; data /= 97;
    horizontal_index = data % 81; data /= 81;
    height_index = data % 61; data /= 61;
    distance_index = data % 95;

    g_ModThirdPersonCameraDistanceAdjust = distance_index == 0
        ? -127 : -126 + ((distance_index - 1) << 1);
    g_ModThirdPersonCameraHeightAdjust = -48 + (height_index << 1);
    g_ModThirdPersonCameraHorizontalAdjust = -60 + (horizontal_index << 1);
    g_ModThirdPersonCameraDownFrameAdjust = -24 + downframe_index;
}
#endif

//CODE.bss:80069B60
/**
 * The chr whose model is currently being built. Has to stay here to preserve ROM layout.
 */
ChrRecord *g_CurModelChr;

//data
//D:8002C510
#ifdef ALL_BONDS
s32 save_selected_bond[] = {BOND_BROSNAN,BOND_CONNERY,BOND_DALTON,BOND_MOORE};
#else
s32 save_selected_bond[] = {BOND_BROSNAN,BOND_BROSNAN,BOND_BROSNAN,BOND_BROSNAN};
#endif


#ifdef GE_SAVE_SRAM
/*
 * V35 R1: keep SRAM transport in the game segment rather than resident joy.c.
 * Physical builds have a fixed resident-code ROM ceiling at retail cdata; the
 * initial V35 backend overflowed that ceiling by placing the transport there.
 * file2.c already lives in the resident Expansion-Pak game segment, so the
 * save backend can grow here without disturbing the fixed boot/code layout.
 */
#define GE_SRAM_BASE          0x08000000u
#define GE_SRAM_SIZE          0x00008000u
#define GE_SRAM_LEGACY_SIZE   0x00000200u
#define GE_SRAM_DMA_CHUNK     0x00000100u
#define GE_SRAM_PROBE_OFFSET  (GE_SRAM_SIZE - 0x10u)
#define GE_SRAM_PROBE_SIZE    0x10u

static s32 g_GeSramProbeState = -1;
static union {
    u64 align;
    u8 bytes[GE_SRAM_DMA_CHUNK];
} g_GeSramBounce;

static void fileSramSetPiTiming(u32 *lat, u32 *pwd, u32 *pgs, u32 *rls)
{
    *lat = IO_READ(PI_BSD_DOM2_LAT_REG);
    *pwd = IO_READ(PI_BSD_DOM2_PWD_REG);
    *pgs = IO_READ(PI_BSD_DOM2_PGS_REG);
    *rls = IO_READ(PI_BSD_DOM2_RLS_REG);

    IO_WRITE(PI_BSD_DOM2_LAT_REG, 0x05);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, 0x0c);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, 0x0d);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, 0x02);
}

static void fileSramRestorePiTiming(u32 lat, u32 pwd, u32 pgs, u32 rls)
{
    IO_WRITE(PI_BSD_DOM2_LAT_REG, lat);
    IO_WRITE(PI_BSD_DOM2_PWD_REG, pwd);
    IO_WRITE(PI_BSD_DOM2_PGS_REG, pgs);
    IO_WRITE(PI_BSD_DOM2_RLS_REG, rls);
}

static s32 fileSramTransfer(u32 offset, void *buffer, u32 nbytes, s32 direction)
{
    u8 *bytes = buffer;
    u32 lat, pwd, pgs, rls;
    u32 amount;
    u32 stat;

    if (buffer == NULL || offset > GE_SRAM_SIZE || nbytes > GE_SRAM_SIZE - offset)
        return -1;
    if (direction != OS_READ && direction != OS_WRITE)
        return -1;

    /* V35 R4: GoldenEye's osPiStartDma path is ROM-relative.  Its raw
     * backend programs PI_CART_ADDR_REG from (osRomBase | devAddr), so a
     * devAddr of 0x08000000 never reaches Domain-2 SRAM as intended.
     *
     * Serialize on libultra's PI access lock and program the physical
     * Domain-2 cartridge address directly instead.  PI_WR_LEN performs
     * cart -> RDRAM (OS_READ), while PI_RD_LEN performs RDRAM -> cart
     * (OS_WRITE). */
    __osPiGetAccess();
    fileSramSetPiTiming(&lat, &pwd, &pgs, &rls);

    while (nbytes != 0)
    {
        amount = nbytes > GE_SRAM_DMA_CHUNK ? GE_SRAM_DMA_CHUNK : nbytes;

        if (direction == OS_READ)
        {
            osInvalDCache(g_GeSramBounce.bytes, amount);
        }
        else
        {
            bcopy(bytes, g_GeSramBounce.bytes, amount);
            osWritebackDCache(g_GeSramBounce.bytes, amount);
        }

        stat = IO_READ(PI_STATUS_REG);
        while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))
            stat = IO_READ(PI_STATUS_REG);

        IO_WRITE(PI_DRAM_ADDR_REG, osVirtualToPhysical(g_GeSramBounce.bytes));
        IO_WRITE(PI_CART_ADDR_REG, GE_SRAM_BASE + offset);

        if (direction == OS_READ)
            IO_WRITE(PI_WR_LEN_REG, amount - 1);
        else
            IO_WRITE(PI_RD_LEN_REG, amount - 1);

        stat = IO_READ(PI_STATUS_REG);
        while (stat & (PI_STATUS_IO_BUSY | PI_STATUS_DMA_BUSY))
            stat = IO_READ(PI_STATUS_REG);

        if (direction == OS_READ)
            bcopy(g_GeSramBounce.bytes, bytes, amount);

        bytes += amount;
        offset += amount;
        nbytes -= amount;
    }

    fileSramRestorePiTiming(lat, pwd, pgs, rls);
    __osPiRelAccess();
    return 0;
}

static s32 fileSramProbe(void)
{
    static const u8 probeA[GE_SRAM_PROBE_SIZE] = {
        0x47, 0x45, 0x2b, 0x53, 0x52, 0x41, 0x4d, 0x31,
        0xa5, 0x5a, 0x3c, 0xc3, 0x96, 0x69, 0xf0, 0x0f
    };
    u8 saved[GE_SRAM_PROBE_SIZE];
    u8 verify[GE_SRAM_PROBE_SIZE];
    s32 read_ok = FALSE;
    s32 ok = TRUE;
    s32 i;

    if (g_GeSramProbeState >= 0)
        return g_GeSramProbeState;

    /* V35 R3: SRAM has no Joybus probe.  Prove the PI-domain backing store
     * exists before allowing legacy save code to consume it.  The final
     * 16 bytes are reserved permanently for this non-destructive probe. */
    if (fileSramTransfer(GE_SRAM_PROBE_OFFSET, saved, GE_SRAM_PROBE_SIZE, OS_READ) != 0)
        ok = FALSE;
    else
        read_ok = TRUE;

    if (ok && fileSramTransfer(GE_SRAM_PROBE_OFFSET, (void *)probeA, GE_SRAM_PROBE_SIZE, OS_WRITE) != 0)
        ok = FALSE;

    if (ok && fileSramTransfer(GE_SRAM_PROBE_OFFSET, verify, GE_SRAM_PROBE_SIZE, OS_READ) != 0)
        ok = FALSE;

    if (ok)
    {
        for (i = 0; i < GE_SRAM_PROBE_SIZE; i++)
        {
            if (verify[i] != probeA[i])
            {
                ok = FALSE;
                break;
            }
        }
    }

    if (read_ok && fileSramTransfer(GE_SRAM_PROBE_OFFSET, saved, GE_SRAM_PROBE_SIZE, OS_WRITE) != 0)
        ok = FALSE;

    g_GeSramProbeState = ok ? 1 : 0;
    return g_GeSramProbeState;
}
#endif /* GE_SAVE_SRAM */

#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)
/*
 * V46: compact, versioned GoldenEye Plus extended-settings journal.
 *
 * The layout is backend-neutral: retail-compatible bytes 0x0000..0x01ff
 * stay untouched, while two 64-byte banks live at 0x0200..0x027f.
 * On the default Plus build these banks are stored in 16 Kbit EEPROM; the
 * older 32 KiB SRAM transport remains available as an explicit opt-in.
 * The newest valid generation is authoritative; the legacy 512-byte image
 * remains a compatibility mirror.
 */
#define GE_SRAM_EXT_BANK_SIZE       0x40u
#define GE_SRAM_EXT_BANK_A_OFFSET   0x0200u
#define GE_SRAM_EXT_BANK_B_OFFSET   0x0240u
#define GE_SRAM_EXT_MAGIC           0x47455053u /* "GEPS" */
#define GE_SRAM_EXT_VERSION_LEGACY  1u
#define GE_SRAM_EXT_VERSION_V2      2u
#define GE_SRAM_EXT_VERSION         3u
#define GE_SRAM_EXT_RECORD_SIZE     8u

#define GE_SRAM_EXT_FLAG_VALID          0x80
#define GE_SRAM_EXT_FLAG_STAY_TP_DEATH  0x01
#define GE_SRAM_EXT_FLAG_MICROOPT       0x02
#define GE_SRAM_EXT_FLAG_TP_CROUCH_CAM  0x04
#define GE_SRAM_EXT_FLAG_DIRECTIONAL    0x08
#define GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY 0x10
#define GE_SRAM_EXT_RESERVED_AA_VALID    0x80
#define GE_SRAM_EXT_RESERVED_AA_ENABLED  0x01
#define GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR 0x20

/* Version-1-only crouch metadata.  These bits are consumed exactly once while
 * upgrading a v1 bank to v2, then stripped from the canonical v2 record. */
#define GE_SRAM_EXT_V1_CROUCH36_MARKER      0x02
#define GE_SRAM_EXT_V1_CROUCH_ABSOLUTE      0x04
#define GE_SRAM_EXT_V1_CROUCH46_MARKER      0x08
#define GE_SRAM_EXT_V1_CROUCH46_REPAIR      0x10
#define GE_SRAM_EXT_V1_CROUCH_MASK          0x1e

typedef struct GeSramExtFolderRecord
{
    s8 camera_distance_adjust;
    s8 camera_height_adjust;
    s8 camera_horizontal_adjust;
    s8 camera_downframe_adjust;
    s8 crouch_camera_height_adjust;
    u8 mod_options2;
    u8 flags;
    u8 reserved;
} GeSramExtFolderRecord;

typedef struct GeSramExtBank
{
    u32 magic;
    u8 version;
    u8 bank_size;
    u8 record_size;
    u8 folder_count;
    u16 generation;
    u16 reserved0;
    u32 checksum;
    GeSramExtFolderRecord folders[MAX_FOLDER_COUNT];
    u8 reserved_tail[16];
} GeSramExtBank;

typedef char ge_sram_ext_record_must_be_8[(sizeof(GeSramExtFolderRecord) == GE_SRAM_EXT_RECORD_SIZE) ? 1 : -1];
typedef char ge_sram_ext_bank_must_be_64[(sizeof(GeSramExtBank) == GE_SRAM_EXT_BANK_SIZE) ? 1 : -1];

static GeSramExtBank g_GeSramExtBank;
static s32 g_GeSramExtLoaded = FALSE;
static s32 g_GeSramExtActiveBank = -1;

static s32 fileSaveExtProbe(void)
{
#ifdef GE_SAVE_SRAM
    return fileSramProbe();
#else
    return joyGamePakProbe() == EEPROM_TYPE_16K;
#endif
}

static s32 fileSaveExtTransfer(u32 offset, void *buffer, u32 nbytes, s32 direction)
{
#ifdef GE_SAVE_SRAM
    return fileSramTransfer(offset, buffer, nbytes, direction);
#else
    u32 block;

    if (buffer == NULL || (offset & (EEPROM_BLOCK_SIZE - 1)) != 0
        || (nbytes & (EEPROM_BLOCK_SIZE - 1)) != 0
        || offset > 0x800u || nbytes > 0x800u - offset)
        return -1;

    block = offset / EEPROM_BLOCK_SIZE;
    if (block >= EEP16K_MAXBLOCKS || block + nbytes / EEPROM_BLOCK_SIZE > EEP16K_MAXBLOCKS)
        return -1;

    if (direction == OS_READ)
        return joyGamePakLongRead((u8)block, buffer, (s32)nbytes);
    if (direction == OS_WRITE)
        return joyGamePakLongWrite((u8)block, buffer, (s32)nbytes);
    return -1;
#endif
}

static u32 fileSramExtChecksum(const GeSramExtBank *source)
{
    GeSramExtBank bank = *source;
    const u8 *bytes;
    u32 crc = 0xffffffffu;
    u32 i;
    s32 bit;

    bank.checksum = 0;
    bytes = (const u8 *)&bank;

    for (i = 0; i < sizeof(bank); i++)
    {
        crc ^= bytes[i];
        for (bit = 0; bit < 8; bit++)
            crc = (crc >> 1) ^ ((0u - (crc & 1u)) & 0xedb88320u);
    }

    return ~crc;
}

static s32 fileSramExtBankValid(const GeSramExtBank *bank)
{
    if (bank->magic != GE_SRAM_EXT_MAGIC
        || (bank->version != GE_SRAM_EXT_VERSION_LEGACY
            && bank->version != GE_SRAM_EXT_VERSION_V2
            && bank->version != GE_SRAM_EXT_VERSION)
        || bank->bank_size != GE_SRAM_EXT_BANK_SIZE
        || bank->record_size != GE_SRAM_EXT_RECORD_SIZE
        || bank->folder_count != MAX_FOLDER_COUNT)
        return FALSE;

    return bank->checksum == fileSramExtChecksum(bank);
}

static void fileSramExtInitEmpty(void)
{
    bzero(&g_GeSramExtBank, sizeof(g_GeSramExtBank));
    g_GeSramExtBank.magic = GE_SRAM_EXT_MAGIC;
    g_GeSramExtBank.version = GE_SRAM_EXT_VERSION;
    g_GeSramExtBank.bank_size = GE_SRAM_EXT_BANK_SIZE;
    g_GeSramExtBank.record_size = GE_SRAM_EXT_RECORD_SIZE;
    g_GeSramExtBank.folder_count = MAX_FOLDER_COUNT;
    g_GeSramExtBank.generation = 0;
    g_GeSramExtBank.checksum = fileSramExtChecksum(&g_GeSramExtBank);
    g_GeSramExtActiveBank = -1;
}

static s32 fileSramExtEnsureLoaded(void)
{
    GeSramExtBank a;
    GeSramExtBank b;
    s32 valid_a;
    s32 valid_b;

    if (g_GeSramExtLoaded)
        return TRUE;

    if (!fileSaveExtProbe())
        return FALSE;

    valid_a = fileSaveExtTransfer(GE_SRAM_EXT_BANK_A_OFFSET, &a, sizeof(a), OS_READ) == 0
        && fileSramExtBankValid(&a);
    valid_b = fileSaveExtTransfer(GE_SRAM_EXT_BANK_B_OFFSET, &b, sizeof(b), OS_READ) == 0
        && fileSramExtBankValid(&b);

    if (valid_a && valid_b)
    {
        if ((s16)(b.generation - a.generation) > 0)
        {
            g_GeSramExtBank = b;
            g_GeSramExtActiveBank = 1;
        }
        else
        {
            g_GeSramExtBank = a;
            g_GeSramExtActiveBank = 0;
        }
    }
    else if (valid_a)
    {
        g_GeSramExtBank = a;
        g_GeSramExtActiveBank = 0;
    }
    else if (valid_b)
    {
        g_GeSramExtBank = b;
        g_GeSramExtActiveBank = 1;
    }
    else
    {
        fileSramExtInitEmpty();
    }

    g_GeSramExtLoaded = TRUE;
    return TRUE;
}

static s32 fileSramExtCommit(void)
{
    GeSramExtBank verify;
    u32 offset;
    s32 target;

    if (!fileSramExtEnsureLoaded())
        return FALSE;

    target = g_GeSramExtActiveBank == 0 ? 1 : 0;
    offset = target ? GE_SRAM_EXT_BANK_B_OFFSET : GE_SRAM_EXT_BANK_A_OFFSET;

    g_GeSramExtBank.generation++;
    g_GeSramExtBank.checksum = fileSramExtChecksum(&g_GeSramExtBank);

    if (fileSaveExtTransfer(offset, &g_GeSramExtBank, sizeof(g_GeSramExtBank), OS_WRITE) != 0)
        return FALSE;
    if (fileSaveExtTransfer(offset, &verify, sizeof(verify), OS_READ) != 0)
        return FALSE;
    if (!fileSramExtBankValid(&verify) || verify.generation != g_GeSramExtBank.generation)
        return FALSE;

    g_GeSramExtActiveBank = target;
    return TRUE;
}

/*
 * V74: schema-versioned camera/settings migration.
 *
 * v1 -> v2 canonicalized crouch height and AA metadata.
 * v2 -> v3 changes the distance byte from a raw signed adjustment to a compact
 * 5-unit adjustment so the runtime camera can span 100..600 around the new
 * authored default of 300 without growing the eight-byte folder record.
 *
 * V72 temporarily used mod_options3 bit 1 for TP Sight Translucency, but that
 * bit belongs to the legacy camera-pack signature.  v3 repairs any contaminated
 * value by restoring Sight Translucency to its authored default On; from then on
 * its independent extension flag is authoritative.
 */
static s32 fileSramExtEnsureCurrentVersion(void)
{
    u32 i;

    if (!fileSramExtEnsureLoaded())
        return FALSE;

    if (g_GeSramExtBank.version == GE_SRAM_EXT_VERSION)
        return TRUE;

    if (g_GeSramExtBank.version == GE_SRAM_EXT_VERSION_LEGACY)
    {
        for (i = 0; i < MAX_FOLDER_COUNT; i++)
        {
            GeSramExtFolderRecord *record = &g_GeSramExtBank.folders[i];

            if (record->flags & GE_SRAM_EXT_FLAG_VALID)
            {
                if (!(record->reserved & GE_SRAM_EXT_V1_CROUCH_ABSOLUTE)
                    || record->crouch_camera_height_adjust == 36)
                    record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;

                if (record->crouch_camera_height_adjust < 0)
                    record->crouch_camera_height_adjust = 0;
                if (record->crouch_camera_height_adjust > 96)
                    record->crouch_camera_height_adjust = 96;

                record->reserved &= (GE_SRAM_EXT_RESERVED_AA_VALID | GE_SRAM_EXT_RESERVED_AA_ENABLED);
            }
        }

        g_GeSramExtBank.version = GE_SRAM_EXT_VERSION_V2;
    }

    if (g_GeSramExtBank.version == GE_SRAM_EXT_VERSION_V2)
    {
        for (i = 0; i < MAX_FOLDER_COUNT; i++)
        {
            GeSramExtFolderRecord *record = &g_GeSramExtBank.folders[i];

            if (record->flags & GE_SRAM_EXT_FLAG_VALID)
            {
                s32 oldadjust = record->camera_distance_adjust;
                s32 newadjust;

                /* An untouched old default follows the newly-authored 300.
                 * Preserve genuinely customized v2 distances approximately. */
                if (oldadjust == 0)
                    newadjust = 0;
                else
                {
                    s32 actual = 240 + oldadjust;
                    s32 delta = actual - TP_CAM_DISTANCE_DEFAULT;
                    newadjust = delta >= 0 ? (delta + 2) / 5 : (delta - 2) / 5;
                    if (newadjust < (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5)
                        newadjust = (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5;
                    if (newadjust > (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5)
                        newadjust = (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5;
                }

                record->camera_distance_adjust = newadjust;
                record->flags &= ~GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY;
            }
        }

        g_GeSramExtBank.version = GE_SRAM_EXT_VERSION;
        return fileSramExtCommit();
    }

    return FALSE;
}

static u32 fileLegacyCameraPacked(const save_data *save)
{
    return ((u32)(save->unlocked_cheats_3 >> 4) & 0x0f)
        | (((u32)(save->flag_007 >> 1) & 0x7f) << 4)
        | ((u32)save->times[(SP_LEVEL_MAX - 1) * 4 - 1] << 11)
        | ((u32)save->mod_camera_tail << 19)
        | (((u32)(save->mod_options3 >> 1) & 0x07) << 27);
}

static s32 fileSramExtRecordFromLegacySave(const save_data *save, GeSramExtFolderRecord *record)
{
    u32 packed;
    u32 data;
    u32 distance_index;
    u32 height_index;
    u32 horizontal_index;
    u32 downframe_index;

    if (save == NULL || record == NULL)
        return FALSE;

    packed = fileLegacyCameraPacked(save);
    if (((packed >> 27) & 0x07) != MOD_CAMERA_PACK_SIGNATURE3)
        return FALSE;

    data = packed & 0x03ffffff;
    downframe_index = data % 97; data /= 97;
    horizontal_index = data % 81; data /= 81;
    height_index = data % 61; data /= 61;
    distance_index = data % 95;

    {
        s32 oldadjust = distance_index == 0 ? -127 : -126 + ((distance_index - 1) << 1);
        s32 delta;

        if (oldadjust == 0)
            record->camera_distance_adjust = 0;
        else
        {
            delta = (240 + oldadjust) - TP_CAM_DISTANCE_DEFAULT;
            record->camera_distance_adjust = delta >= 0 ? (delta + 2) / 5 : (delta - 2) / 5;
            if (record->camera_distance_adjust < (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5)
                record->camera_distance_adjust = (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5;
            if (record->camera_distance_adjust > (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5)
                record->camera_distance_adjust = (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5;
        }
    }
    record->camera_height_adjust = -48 + (height_index << 1);
    record->camera_horizontal_adjust = -60 + (horizontal_index << 1);
    record->camera_downframe_adjust = -24 + downframe_index;
    record->crouch_camera_height_adjust = 0;
    record->mod_options2 = save->mod_options2;
    record->flags = GE_SRAM_EXT_FLAG_VALID;
    /* V40: old extension records had reserved==0. Treat them as the authored
     * Anti-Aliasing default (On), while marking newly migrated records explicitly. */
    record->reserved = GE_SRAM_EXT_RESERVED_AA_VALID | GE_SRAM_EXT_RESERVED_AA_ENABLED;
    /* V42 R2 stores the authored crouch-camera height itself in the extension
     * byte.  Legacy EEPROM has no crouch-height field, so migration starts at
     * the current authored default rather than manufacturing an adjustment. */
    record->crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT;

    if (packed & MOD_CAMERA_STAY_TP_DEATH_BIT)
        record->flags |= GE_SRAM_EXT_FLAG_STAY_TP_DEATH;
    if (save->mod_options3 & MODOPT3_ENABLE_MICROOPT)
        record->flags |= GE_SRAM_EXT_FLAG_MICROOPT;
    if (save->mod_options3 & MODOPT3_TP_CROUCH_CAM)
        record->flags |= GE_SRAM_EXT_FLAG_TP_CROUCH_CAM;
    if (save->mod_options3 & MODOPT3_DIRECTIONAL_SHOULDER)
        record->flags |= GE_SRAM_EXT_FLAG_DIRECTIONAL;

    return TRUE;
}

static s32 fileSramExtUpdateFolder(u32 folder, const GeSramExtFolderRecord *record)
{
    if (folder >= MAX_FOLDER_COUNT || record == NULL || !fileSramExtEnsureCurrentVersion())
        return FALSE;

    if (!memcmp(&g_GeSramExtBank.folders[folder], record, sizeof(*record)))
        return TRUE;

    g_GeSramExtBank.folders[folder] = *record;
    return fileSramExtCommit();
}

static void fileSramExtSyncLegacySave(save_data *save)
{
    GeSramExtFolderRecord record;
    u32 folder;

    if (save == NULL || (save->completion_bitflags & SAVEFLAG_DORESET))
        return;

    folder = save->completion_bitflags & SAVEFLAG_FOLDER;
    if (folder >= MAX_FOLDER_COUNT || !fileSramExtEnsureCurrentVersion())
        return;

    if (!fileSramExtRecordFromLegacySave(save, &record))
        return;

    if (g_GeSramExtBank.folders[folder].flags & GE_SRAM_EXT_FLAG_VALID)
    {
        GeSramExtFolderRecord *current = &g_GeSramExtBank.folders[folder];

        /* V75: every TP camera tuner in the extension journal is authoritative.
         * The 512-byte compatibility mirror can be stale when an unrelated
         * legacy save is written; decoding it here used to resurrect the known
         * Height 4 / Horizontal 72 / Down Frame 96 poison values.  Preserve all
         * camera fields from the current extension record during generic mirror
         * synchronization. */
        record.camera_distance_adjust = current->camera_distance_adjust;
        record.camera_height_adjust = current->camera_height_adjust;
        record.camera_horizontal_adjust = current->camera_horizontal_adjust;
        record.camera_downframe_adjust = current->camera_downframe_adjust;
        record.crouch_camera_height_adjust = current->crouch_camera_height_adjust;
        record.reserved = current->reserved;
        record.flags = (record.flags & ~GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY)
            | (current->flags & GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY);
    }

    fileSramExtUpdateFolder(folder, &record);
}

static s32 fileSramExtLoadFolder(u32 folder, save_data *save)
{
    GeSramExtFolderRecord *record;
    s32 i;


    if (folder >= MAX_FOLDER_COUNT || save == NULL || !fileSramExtEnsureLoaded())
        return FALSE;

    if (!fileSramExtEnsureCurrentVersion())
        return FALSE;

    record = &g_GeSramExtBank.folders[folder];
    if (!(record->flags & GE_SRAM_EXT_FLAG_VALID))
        return FALSE;

    /* V75 one-time repair for the exact stale-camera signature produced by the
     * V74 generic legacy-mirror synchronization regression.  This is the same
     * historical poison state already documented by the V30B migration:
     * displayed Height 4, Horizontal 72, Down Frame 96.  Only that exact triple
     * is reset; legitimate custom camera values are otherwise retained. */
    if (!(record->reserved & GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR))
    {
        if (record->camera_height_adjust == 16
            && record->camera_horizontal_adjust == 96
            && record->camera_downframe_adjust == 72)
        {
            record->camera_height_adjust = 0;
            record->camera_horizontal_adjust = 0;
            record->camera_downframe_adjust = 0;
        }

        record->reserved |= GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR;
        fileSramExtCommit();
    }

    g_ModThirdPersonCameraDistanceAdjust = (s32)record->camera_distance_adjust * 5;
    g_ModThirdPersonCameraHeightAdjust = record->camera_height_adjust;
    g_ModThirdPersonCameraHorizontalAdjust = record->camera_horizontal_adjust;
    g_ModThirdPersonCameraDownFrameAdjust = record->camera_downframe_adjust;
    /* V42 R2 record byte is the absolute displayed/runtime crouch height. */
    if (record->crouch_camera_height_adjust < 0)
        record->crouch_camera_height_adjust = 0;
    if (record->crouch_camera_height_adjust > 96)
        record->crouch_camera_height_adjust = 96;
    g_ModThirdPersonCrouchCameraHeightAdjust = record->crouch_camera_height_adjust - TP_CROUCH_CAM_HEIGHT_DEFAULT;

    if (g_ModThirdPersonCameraDistanceAdjust < TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT)
        g_ModThirdPersonCameraDistanceAdjust = TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT;
    if (g_ModThirdPersonCameraDistanceAdjust > TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT)
        g_ModThirdPersonCameraDistanceAdjust = TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT;
    if (g_ModThirdPersonCameraHeightAdjust < -48) g_ModThirdPersonCameraHeightAdjust = -48;
    if (g_ModThirdPersonCameraHeightAdjust > 72) g_ModThirdPersonCameraHeightAdjust = 72;
    if (g_ModThirdPersonCameraHorizontalAdjust < -60) g_ModThirdPersonCameraHorizontalAdjust = -60;
    if (g_ModThirdPersonCameraHorizontalAdjust > 100) g_ModThirdPersonCameraHorizontalAdjust = 100;
    if (g_ModThirdPersonCameraDownFrameAdjust < -24) g_ModThirdPersonCameraDownFrameAdjust = -24;
    if (g_ModThirdPersonCameraDownFrameAdjust > 72) g_ModThirdPersonCameraDownFrameAdjust = 72;
    if (g_ModThirdPersonCrouchCameraHeightAdjust < -TP_CROUCH_CAM_HEIGHT_DEFAULT)
        g_ModThirdPersonCrouchCameraHeightAdjust = -TP_CROUCH_CAM_HEIGHT_DEFAULT;
    if (g_ModThirdPersonCrouchCameraHeightAdjust > 96 - TP_CROUCH_CAM_HEIGHT_DEFAULT)
        g_ModThirdPersonCrouchCameraHeightAdjust = 96 - TP_CROUCH_CAM_HEIGHT_DEFAULT;

    g_ModGameplayOptions2 = record->mod_options2;
    g_ModGameplayOptions3 = MODOPT3_SIGNATURE;
    if (record->flags & GE_SRAM_EXT_FLAG_MICROOPT) g_ModGameplayOptions3 |= MODOPT3_ENABLE_MICROOPT;
    if (record->flags & GE_SRAM_EXT_FLAG_TP_CROUCH_CAM) g_ModGameplayOptions3 |= MODOPT3_TP_CROUCH_CAM;
    if (record->flags & GE_SRAM_EXT_FLAG_DIRECTIONAL) g_ModGameplayOptions3 |= MODOPT3_DIRECTIONAL_SHOULDER;
    g_ModTpSightTranslucencyEnabled =
        (record->flags & GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY) == 0;

    g_ModStayInTpOnDeathDefault = (record->flags & GE_SRAM_EXT_FLAG_STAY_TP_DEATH) != 0;
    g_ModAntiAliasingEnabled = !(record->reserved & GE_SRAM_EXT_RESERVED_AA_VALID)
        || (record->reserved & GE_SRAM_EXT_RESERVED_AA_ENABLED);
    for (i = 0; i < MAX_PLAYER_COUNT; i++)
        g_PlayerStayInTpOnDeath[i] = g_ModStayInTpOnDeathDefault;

    /* Keep the first 512 bytes as a backwards-compatible mirror.  The extended-settings
     * journal is authoritative, but old EEPROM builds/tools still see a
     * coherent representation of every setting they know about. */
    save->mod_options2 = g_ModGameplayOptions2;
    save->mod_options3 = g_ModGameplayOptions3;
    fileStoreThirdPersonCameraSettings(save);
    return TRUE;
}

s32 fileLoadExtendedSettings(save_data *save)
{
    u32 folder;

    if (save == NULL || (save->completion_bitflags & SAVEFLAG_DORESET))
        return FALSE;

    folder = save->completion_bitflags & SAVEFLAG_FOLDER;
    if (folder >= MAX_FOLDER_COUNT)
        return FALSE;

    return fileSramExtLoadFolder(folder, save);
}

void fileStoreExtendedSettings(save_data *save)
{
    GeSramExtFolderRecord record;
    u32 folder;

    if (save == NULL || (save->completion_bitflags & SAVEFLAG_DORESET))
        return;

    folder = save->completion_bitflags & SAVEFLAG_FOLDER;
    if (folder >= MAX_FOLDER_COUNT)
        return;

        {
        s32 encoded = g_ModThirdPersonCameraDistanceAdjust / 5;
        if (encoded < (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5)
            encoded = (TP_CAM_DISTANCE_MIN - TP_CAM_DISTANCE_DEFAULT) / 5;
        if (encoded > (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5)
            encoded = (TP_CAM_DISTANCE_MAX - TP_CAM_DISTANCE_DEFAULT) / 5;
        record.camera_distance_adjust = encoded;
    }
    record.camera_height_adjust = g_ModThirdPersonCameraHeightAdjust;
    record.camera_horizontal_adjust = g_ModThirdPersonCameraHorizontalAdjust;
    record.camera_downframe_adjust = g_ModThirdPersonCameraDownFrameAdjust;
    /* Persist the exact user-facing height so changing a future authored
     * default cannot silently reinterpret an existing stored value. */
    record.crouch_camera_height_adjust = TP_CROUCH_CAM_HEIGHT_DEFAULT
        + g_ModThirdPersonCrouchCameraHeightAdjust;
    record.mod_options2 = g_ModGameplayOptions2;
    record.flags = GE_SRAM_EXT_FLAG_VALID;
    record.reserved = GE_SRAM_EXT_RESERVED_AA_VALID
        | GE_SRAM_EXT_RESERVED_V75_CAMERA_REPAIR
        | (g_ModAntiAliasingEnabled ? GE_SRAM_EXT_RESERVED_AA_ENABLED : 0);

    if (g_ModStayInTpOnDeathDefault) record.flags |= GE_SRAM_EXT_FLAG_STAY_TP_DEATH;
    if (g_ModGameplayOptions3 & MODOPT3_ENABLE_MICROOPT) record.flags |= GE_SRAM_EXT_FLAG_MICROOPT;
    if (g_ModGameplayOptions3 & MODOPT3_TP_CROUCH_CAM) record.flags |= GE_SRAM_EXT_FLAG_TP_CROUCH_CAM;
    if (g_ModGameplayOptions3 & MODOPT3_DIRECTIONAL_SHOULDER) record.flags |= GE_SRAM_EXT_FLAG_DIRECTIONAL;
    if (!g_ModTpSightTranslucencyEnabled)
        record.flags |= GE_SRAM_EXT_FLAG_DISABLE_TP_SIGHT_TRANSLUCENCY;

    fileSramExtUpdateFolder(folder, &record);
}

static void fileSramExtInvalidateFolder(u32 folder)
{
    GeSramExtFolderRecord record;

    if (folder >= MAX_FOLDER_COUNT || !fileSramExtEnsureCurrentVersion())
        return;

    bzero(&record, sizeof(record));
    fileSramExtUpdateFolder(folder, &record);
}
#endif /* GE_SAVE_SRAM || GE_SAVE_EEPROM16K */

#ifdef GE_SAVE_SRAM
static s32 fileGamePakLongRead(u8 address, u8 *buffer, s32 nbytes)
{
    u32 offset = (u32)address * EEPROM_BLOCK_SIZE;

    if (nbytes < 0 || offset > GE_SRAM_LEGACY_SIZE || (u32)nbytes > GE_SRAM_LEGACY_SIZE - offset)
        return -1;

    return fileSramTransfer(offset, buffer, (u32)nbytes, OS_READ);
}

static s32 fileGamePakLongWrite(u8 address, u8 *buffer, s32 nbytes)
{
    u32 offset = (u32)address * EEPROM_BLOCK_SIZE;

    if (nbytes < 0 || offset > GE_SRAM_LEGACY_SIZE || (u32)nbytes > GE_SRAM_LEGACY_SIZE - offset)
        return -1;

    return fileSramTransfer(offset, buffer, (u32)nbytes, OS_WRITE);
}
#else
#define fileGamePakLongRead joyGamePakLongRead
#define fileGamePakLongWrite joyGamePakLongWrite
#endif

#if !defined(GE_SAVE_SRAM) && !defined(GE_SAVE_EEPROM16K)
s32 fileLoadExtendedSettings(save_data *save) { (void)save; return FALSE; }
void fileStoreExtendedSettings(save_data *save) { (void)save; }
#endif

/**
 *
 *
 * @return s32
 */
s32 fileGamePakProbe(void)
{
#ifdef GE_SAVE_SRAM
  return fileSramProbe();
#elif defined(GE_SAVE_EEPROM16K)
  return joyGamePakProbe() == EEPROM_TYPE_16K;
#else
  return joyGamePakProbe();
#endif
}

/**
 * Resets the RamRom replay folder save
 *
 */
void fileResetRamRomSave(void)
{
    save_data new_save = BLANKSAVEDATA;

    saves[SAVESLOTRAMROM] = new_save;
}

/**
 *
 *
 * @param save
 */
void fileWriteSmallSave(smallSave *save)
{
    if (fileGamePakProbe())
    {
        fileGenerateCRC(&save->unk[0], &save->unk[24], save);
        fileGamePakLongWrite(0, save, sizeof(smallSave));
    }
}

/**
 *
 *
 * @param save
 */
void fileWriteSave(save_data *save)
{
    if (save >= &saves[SAVESLOT1] && save < &saves[SAVESLOTRAMROM])
    {
        if ( fileGamePakProbe())
        {
            fileGenerateCRC(&save->completion_bitflags, save + 1, save);
            fileGamePakLongWrite((((u32)((save - &saves[SAVESLOT1]) * 0x60) >> 3) + 4), save, sizeof(save_data)); // 0x60 = sizeof(save_data) be sure to manually update if save changes
#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)
            fileSramExtSyncLegacySave(save);
#endif
        }
    }
}

/**
 * reset save to default
 *
 * @param save
 */
void fileResetSave(save_data *save)
{
    save_data new_save  = BLANKSAVEDATA;

    *save = new_save;
    fileWriteSave(save);
}

/**
 * Get the folder of save
 *
 * @param save
 * @return u32
 */
u32 fileGetSaveFolder(save_data *save)
{
  return save->completion_bitflags & SAVEFLAG_FOLDER;
}

/**
 * Clear then set save folder flag
 *
 * @param save
 * @param folder
 */
void fileSetSaveFoldernum(save_data *save, u32 folder)
{
    save->completion_bitflags &= ~SAVEFLAG_FOLDER;
    save->completion_bitflags |= folder & SAVEFLAG_FOLDER;
}

/**
 * Get save flag 0x18
 *
 * @param folder
 * @return u32
 */
u32 fileGetSaveFlagSlot(save_data *folder)
{
  return (folder->completion_bitflags & SAVEFLAG_SLOT) >> 3;
}

/**
 * Resets save flag 0x18
 *
 * @param folder
 * @param arg1
 */
void fileResetSaveFlagSlot(save_data *folder, s32 slot)
{
    folder->completion_bitflags &= ~SAVEFLAG_SLOT;
    folder->completion_bitflags |= ((slot * 8) & SAVEFLAG_SLOT);
}

/**
 * Get the selected bond save flag
 *
 * @param folder
 * @return u32
 */
u32 fileGetSelectedBond(save_data *folder)
{
  return (folder->completion_bitflags & SAVEFLAG_BOND) >> 5;
}

/**
 * Set the selected bond save flag
 *
 * @param folder
 * @param bond
 */
void fileSetSelectedBond(save_data *folder, s32 bond)
{
    folder->completion_bitflags &= ~SAVEFLAG_BOND;
    folder->completion_bitflags |= ((bond << 5) & SAVEFLAG_BOND);
}

/**
 * Check if save has flag 0x80
 *
 * @param folder
 * @return TRUE/FALSE
 */
bool fileGetSaveFlagDoReset(save_data *folder)
{
  return ((folder->completion_bitflags & SAVEFLAG_DORESET) != FALSE);
}

/**
 * Toggle save flag 0x80
 * possibly wear levelling
 *
 * @param folder: folder to enable or disable flag
 * @param set: Enable flag if TRUE, Disable flag if FALSE
 */
void fileSetSaveFlagDoReset(save_data *folder, bool enable)
{
    if (enable)
    {
        folder->completion_bitflags |= SAVEFLAG_DORESET;
    }
    else
    {
        folder->completion_bitflags &= ~SAVEFLAG_DORESET;
    }
}

/**
 * Get completion time for stage at difficulty
 *
 * @param save
 * @param levelid
 * @param difficulty
 * @return best time for stage at difficulty
 */
s32 fileGetSaveStageDifficultyTime(save_data* save, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty)
{
    s32 offset;
    LEVEL_SOLO_SEQUENCE max_level;
    u32 time;
    s32 index;

    max_level = SP_LEVEL_MAX;
    if ((levelid >= SP_LEVEL_DAM) && (levelid < SP_LEVEL_MAX ) && (difficulty >= DIFFICULTY_AGENT) && (difficulty < DIFFICULTY_MAX))
    {
        if (difficulty == DIFFICULTY_007)
        {
            if ( fileIs007ModeUnlocked( fileGetSaveFolder(save)))
            {
                return 0x3FF; //max time
            }
            return 0;
        }

        offset = ((difficulty * max_level) + levelid) * 10; //startbit
        index = (offset >> 3);

        switch(7 - (offset & 7)) //bitmask
        {
            case 7: //no offset agent
                // first 10 bits 8 + 2                    1111 1111                                      1100 0000
                time = ((save->times[index] & 0xFF) << 2) | ((save->times[index + 1] & 0xc0) >> 6);
                break;
            case 5: //offset 2 secret agent
                // next 10 bits 6 + 4                     0011 1111                                      1111 0000
                time =  ((save->times[index] & 0x3f) << 4) | ((save->times[index + 1] & 0xf0) >> 4);
                break;
            case 3: //offset 4 00 agent
                // next 10 bits 4 + 6                     0000 1111                                      1111 1100
                time =  ((save->times[index] & 0xf) << 6) | ((save->times[index + 1] & 0xfc) >> 2);
                break;
            case 1: //offset 6 007
                // next 10 bits 2 + 8                     0000 0011                                      1111 1111
                time = ((save->times[index] & 0x3)  << 8) | ((save->times[index + 1] & 0xFFF));
                break;
            default:
                time = 0; // shouldnt reach
#if DEBUG
                osSyncPrintf("file.c: SHOULDN\'T GET HERE EVER [1]\n");
#endif
        }

        return time;
    }

    return 0;
}

/**
 * Set completion time for stage at difficulty
 *
 * @param save
 * @param levelid
 * @param difficulty
 * @param newtime
 */
void fileSetDifficultyStageTime(save_data *save, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty, s32 newtime)
{
    s32 offset;
    s32 index;
    LEVEL_SOLO_SEQUENCE max_level;

    max_level = SP_LEVEL_MAX;
    if ((levelid >= SP_LEVEL_DAM) && (levelid < SP_LEVEL_MAX ) && (difficulty >= DIFFICULTY_AGENT) && (difficulty < DIFFICULTY_007))
    {
        if (newtime == 0) {
            newtime = 0x4f;
        } else if (newtime > 0x3ff) {
            newtime = 0x3ff;
        }

        offset = ((difficulty * max_level) + levelid) * 10; //startbit
        index = (offset >> 3);

        switch(7 - (offset & 7)) //bitmask
        {
            case 7: //no offset 4 8 12 etc agent
                save->times[index] &= 0xff00;
                save->times[index + 1] &= 0xff3f;
                save->times[index] |= (newtime >> 2) & 0xff;
                save->times[index + 1] |= (newtime << 6) & 0xc0;
                break;
            case 5: //first offset 5 9 13 etc secret agent
                save->times[index] &= 0xffc0;
                save->times[index + 1] &= 0xff0f;
                save->times[index] |= ((newtime >> 4) & 0x3f);
                save->times[index + 1] |= (newtime << 4) & 0xf0;
                break;
            case 3: //second offset 6 10 14 etc 00 agent
                save->times[index] &= 0xfff0;
                save->times[index + 1] &= 0xff03;
                save->times[index] |= ((newtime >> 6) & 0xf);
                save->times[index + 1] |= (newtime << 2) & 0xfC;
                break;
            case 1: //third offset 7 11 15 etc 007
                save->times[index] &= 0xfffc;
                save->times[index + 1] &= 0xff00;
                save->times[index] |= ((newtime >> 8) & 3);
                save->times[index + 1] |= newtime & 0xfff;
                break;
            default:
#if DEBUG
                osSyncPrintf("file.c: SHOULDN\'T GET HERE EVER [2]\n");
#endif
                break;
        }
    }
}


/**
 * Check if stage is completed at difficulty for save
 *
 * @param folder
 * @param levelid
 * @param difficulty
 * @return is stage at diffiuclty completed
 */
bool fileGetSaveStageCompletedForDifficulty(save_data *folder, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty)
{
    if ((levelid >= SP_LEVEL_DAM) && (levelid < SP_LEVEL_MAX) && (difficulty >= DIFFICULTY_AGENT) && (difficulty <= DIFFICULTY_007))
    {
        return fileGetSaveStageDifficultyTime(folder, levelid, difficulty) != 0;
    }

    return FALSE;
}

/**
 * Updates time for stage at difficulty if better
 *
 * @param folder
 * @param levelid
 * @param difficulty
 * @param arg4
 */
void fileCheckSaveStageDifficultyTime(save_data *folder, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty, s32 newtime)
{
    if ((levelid >= SP_LEVEL_DAM) && (levelid < SP_LEVEL_MAX) && (difficulty >= DIFFICULTY_AGENT) && (difficulty <= DIFFICULTY_007))
    {
        s32 time = fileGetSaveStageDifficultyTime(folder, levelid, difficulty);

        if ((time == 0) || (newtime < time))
        {
            fileSetDifficultyStageTime(folder, levelid, difficulty, newtime);
        }
    }
}

/**
 * Check if cheat is unlocked
 *
 * @param save
 * @param cheat
 * @return bool
 */
bool fileGetIsCheatUnlocked(save_data *save, s32 cheat)
{
    s32 bits;

    if (cheat >= 0 && cheat < CHEAT_INPUT_BUFFER_SIZE)
    {
        bits = save->unlocked_cheats_1 | save->unlocked_cheats_3 << 0x18 | save->unlocked_cheats_3 << 0x10 | save->unlocked_cheats_2 << 8;
        return ((1 << cheat) & bits) != 0;
    }

    return FALSE;
}

/**
 *
 *
 * @param save
 * @param cheat
 */
void fileSetSaveCheatUnlocked(save_data *save, s32 cheat)
{
    u32 i;
    u32 temp;

    if (cheat >= 0 && cheat < CHEAT_INPUT_BUFFER_SIZE)
    {
        temp = 1 << (cheat);

        for(i = 0; temp > 0xff; i++)
        {
            temp = temp >> 8;
        }

        *(((u8 *)save + i + 0xe)) |= temp & 0xFFu; //save.unlocked_cheats_1[i] |= temp;
    }
}

/**
 * Get save in foldernum slot
 *
 * @param foldernum
 * @return save_data*
 */
save_data * fileGetSaveForFoldernum(u32 folder)
{
    int i;

    for (i = SAVESLOT1; i < SAVESLOTRAMROM; i++)
    {
        if ( fileGetSaveFlagDoReset(&saves[i]) == FALSE &&
                fileGetSaveFolder(&saves[i]) == folder)
        {
            return &saves[i];
        }
    }

    if (folder == RAMROM_FOLDERNUM)
    {
        return &saves[SAVESLOTRAMROM];
    }

    return NULL;
}

/**
 * See if any save has 0x80 flag
 *
 * @return s32
 */
s32 fileGetSaveFlagDoReset_any_folder(void)
{
    s32 i;

    for(i = SAVESLOT1; i < SAVESLOTRAMROM; i++)
    {
        if ( fileGetSaveFlagDoReset(&saves[i]))
        {
            return i;
        }
    }

    return -1;
}

/**
 * Resets save with 0x80 flag
 * Maybe clearing for copy or wear level
 *
 * @param folder
 */
void fileBuildWriteNewSave(u32 folder)
{
    s32 folder_with_flag;

    folder_with_flag = fileGetSaveFlagDoReset_any_folder();

    if (folder_with_flag >= 0)
    {
        save_data new_save = BLANKSAVEDATA;
        saves[folder_with_flag] = new_save;

        fileSetSaveFoldernum(&saves[folder_with_flag], folder);
        fileSetSaveFlagDoReset(&saves[folder_with_flag], FALSE);
        fileSetSelectedBond(&saves[folder_with_flag], folder);
        fileWriteSave(&saves[folder_with_flag]);
    }
}

#ifdef GE_UNLOCK_SAVE1
static void fileForceSave1FullyUnlocked(void)
{
    save_data *save = fileGetSaveForFoldernum(FOLDER1);
    s32 i;

    for (i = 0; i < (s32)sizeof(save->times); i++)
    {
        save->times[i] = 0xff;
    }

    save->flag_007 |= 1;
    save->unlocked_cheats_1 = 0xff;
    save->unlocked_cheats_2 = 0xff;
    save->unlocked_cheats_3 = (save->unlocked_cheats_3 & 0xf0) | 0x0f;
    fileWriteSave(save);
}
#endif

void fileValidateSaves(void)
{
    bool checksumOK;
    smallSave joyChecksum;
    s32 crc[2];
    s32 i;
    s32 *temp;

    if (fileGamePakProbe())
    {
        checksumOK = TRUE;

        // block read 32 bytes
        fileGamePakLongRead(0, &joyChecksum, sizeof(smallSave));

        // if customised file dont assume crc is ok
        if (joyChecksum.unk[0] != SAVEFLAGS_SET(FOLDER3, SAVESLOT1, BOND_CONNERY, FALSE))
        {
            checksumOK = FALSE;
        }

        fileGenerateCRC(&joyChecksum.unk[0], &joyChecksum.unk[24], &crc); //do checksum on 24 bytes of save data

        temp = &joyChecksum;

        if ((crc[0] != temp[0]) || (crc[1] != temp[1]))
        {
            checksumOK = FALSE;
        }

        // bad checksum, create a new save and replace damaged one.
        if (!checksumOK)
        {
            smallSave NewSave = {0, 0, SAVEFLAGS_SET(FOLDER3, SAVESLOT1, BOND_CONNERY, FALSE)};
            joyChecksum = NewSave;
            fileWriteSmallSave(&joyChecksum);
        }

        // Block read 5 saves starting at address 4th byte (? bug: address must be multiple of 8 - return is -1)
        fileGamePakLongRead(4, &saves, sizeof(save_data) * 5);

        for (i = SAVESLOT1; i != SAVESLOTRAMROM; i++) //only != matches
        {
            bool checksumOK2 = TRUE;

            fileGenerateCRC(&saves[i].completion_bitflags, &saves[i + 1], &crc); // do checksum on save data

            if (1){} // Hack to shift registers
            if (1){} // or something like if (FINAL){}

            if ((crc[0] != saves[i].chksum1) ||
                (crc[1] != saves[i].chksum2))
            {
                checksumOK2 = FALSE;
            }

            if (!checksumOK2)
            {
                fileResetSave(&saves[i]);
            }
        }

        for (i = FOLDER1; i < MAX_FOLDER_COUNT; i++)
        {
            s32 slot_2;
            s32 jif    = -1;
            s32 slot = -1;
            s32 j;

            // for each save
            for (j = SAVESLOT1; j < SAVESLOTRAMROM; j++)
            {
                // if save = folder and SAVEFLAG_DORESET set
                if (!fileGetSaveFlagDoReset(&saves[j]) &&
                    fileGetSaveFolder(&saves[j]) == i)
                {
                    if (jif < SAVESLOT1) // on first SAVEFLAG_DORESET do this
                    {
                        jif = j;
                        slot = fileGetSaveFlagSlot(&saves[j]);
                    }
                    else
                    {
                        slot_2 = fileGetSaveFlagSlot(&saves[j]);

                        if (slot_2 == (slot + 1) % 4)
                        {
                            fileResetSave(&saves[jif]);
                            jif = j;
                            slot = slot_2;
                        }
                        else
                        {
                            fileResetSave(&saves[j]);
                        }
                    }
                }
            }

            // SAVEFLAG_DORESET was not set
            if (jif < SAVESLOT1)
            {
                fileBuildWriteNewSave(i);
            }
        }

        for (i = FOLDER1; i < MAX_FOLDER_COUNT; i++)
        {
            save_data *save = fileGetSaveForFoldernum(i);

            if (save)
            {
                save_selected_bond[i] = fileGetSelectedBond(save);
            }
        }
#ifdef GE_UNLOCK_SAVE1
        fileForceSave1FullyUnlocked();
#endif
    }
}

/**
 * Check if folder is valid
 *
 * @param folder
 * @return bool
 */
bool fileIsFolderValid(s32 folder)
{
    if ((folder >= FOLDER1) && (folder < MAX_FOLDER_COUNT))
    {
        return TRUE;
    }

    if (folder == RAMROM_FOLDERNUM)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * wrapper func - uses save if found
 * file fileIsStageUnlockedAtDifficulty calls fileIsSavedStageUnlockedAtDifficulty
 *
 * @param foldernum
 * @param levelid
 * @param difficulty
 * @return 0, 1, or 3 (STAGESTATUS_LOCKED, STAGESTATUS_UNLOCKED, STAGESTATUS_COMPLETED)
 */
STAGESTATUS fileIsStageUnlockedAtDifficulty(s32 foldernum, LEVEL_SOLO_SEQUENCE levelid, DIFFICULTY difficulty)
{
    save_data* save;
    s32 i;

    if (( fileIsFolderValid(foldernum)) &&
        (levelid >= SP_LEVEL_DAM && levelid < SP_LEVEL_MAX) &&
        (difficulty >= DIFFICULTY_AGENT && difficulty < DIFFICULTY_MAX))
    {
        save = fileGetSaveForFoldernum(foldernum);

        if (save)
        {
            if ( fileGetSaveStageCompletedForDifficulty(save, levelid, difficulty))
            {
                return STAGESTATUS_COMPLETED; //found on first try, stage has been completed and a time saved.
            }

            if ((levelid == SP_LEVEL_AZTEC && difficulty < DIFFICULTY_SECRET) ||
                (levelid == SP_LEVEL_EGYPT && difficulty < DIFFICULTY_00))
            {
                return STAGESTATUS_LOCKED; //we cant possibly have a completed bonus stage below each set dificulty
            }

            //still cant find it, do a search (this is probably how a cheat can unlock stages without having to actualy do them all)
            for (i = difficulty; i < DIFFICULTY_MAX ; i++)
            {
                LEVEL_SOLO_SEQUENCE istage;
                for (istage = SP_LEVEL_DAM; istage < levelid; istage++)
                {
                    if (! fileGetSaveStageCompletedForDifficulty(save, istage, i))
                    {
                        break;
                    }
                }
                //if the first uncomplete stage is not less than current
                if (levelid <= istage)
                {
                    return STAGESTATUS_UNLOCKED;
                }
            }

            // if we still cant find it
            if ((difficulty < DIFFICULTY_007) && (levelid < SP_LEVEL_AZTEC))
            {
                for (i = difficulty; i < DIFFICULTY_MAX; i++)
                {
                    if ( fileGetSaveStageCompletedForDifficulty(save, levelid - 1, i))
                    {
                        return STAGESTATUS_UNLOCKED;
                    }
                }
            }

            if (difficulty < DIFFICULTY_007)
            {
                for (i = SP_LEVEL_DAM; i < SP_LEVEL_AZTEC; i++)
                {
                    if (! fileGetSaveStageCompletedForDifficulty(save, i, DIFFICULTY_AGENT))
                    {
                        break;
                    }
                }
                //this cant actually fire an it?
                if (i >= SP_LEVEL_AZTEC)
                {
                    for (i = DIFFICULTY_AGENT; i < difficulty; i++)
                    {
                        if (! fileGetSaveStageCompletedForDifficulty(save, levelid, i))
                        {
                            break;
                        }
                    }

                    if (difficulty <= i)
                    {
                        return STAGESTATUS_UNLOCKED;
                    }
                }
            }// difficulty < DIFFICULTY_007
        }// save

        // no save, current level is dam, its unlocked.
        if (levelid == SP_LEVEL_DAM)
        {
            return STAGESTATUS_UNLOCKED;
        }

        // no save, cheat enabled, its unlocked.
        if (get_debug_enable_agent_levels_flag() && difficulty == DIFFICULTY_AGENT)
        {
            return STAGESTATUS_UNLOCKED;
        }

        // no save, cheat enabled, its unlocked. (basically a repeat of above)
        if (get_debug_enable_all_levels_flag())
        {
            return STAGESTATUS_UNLOCKED;
        }
    }
    // After all that the stage is not unlocked
    return STAGESTATUS_LOCKED;
}

/**
 *
 *
 * @param save1
 * @param save2
 */
void fileOverwriteSaveSlotWithNewSave(save_data *save1, save_data *save2)
{
    s32 folder_with_flag;
    s32 slot;

    slot = 0;
    folder_with_flag = fileGetSaveFlagDoReset_any_folder();

    if (folder_with_flag >= 0)
    {
        saves[folder_with_flag] = *save2;

        if (save1)
        {
            slot = (s32)( fileGetSaveFlagSlot(save1) + 1) % 4;
        }

        fileSetSaveFlagDoReset(&saves[folder_with_flag], FALSE);
        fileResetSaveFlagSlot(&saves[folder_with_flag], slot);
        fileWriteSave(&saves[folder_with_flag]);

        if (save1)
        {
            fileResetSave(save1);
        }
    }
}

/**
 *
 *
 * @param foldernum
 * @param stage
 * @param difficulty
 * @param maxtime
 */
void fileUnlockStageInFolderAtDifficulty(s32 foldernum, LEVEL_SOLO_SEQUENCE stage, DIFFICULTY difficulty, s32 newtime)
{
    if ((foldernum >= 0) && (foldernum < MAX_FOLDER_COUNT) &&
        (stage >= SP_LEVEL_DAM) && (stage < SP_LEVEL_MAX) &&
        (difficulty >= DIFFICULTY_AGENT) && (difficulty < DIFFICULTY_MAX))
    {
        save_data new_save = BLANKSAVEDATA;

        save_data *save = fileGetSaveForFoldernum(foldernum);
        s32 i;
        if (save) {
            new_save = *save;
        } else {
            fileSetSaveFoldernum(&new_save, foldernum);
        }

        for (i = difficulty; i >= DIFFICULTY_AGENT; i--)
        {
            if (i == difficulty)
            {
                fileCheckSaveStageDifficultyTime(&new_save, stage, i, newtime);
            }
            else
            {
                fileCheckSaveStageDifficultyTime(&new_save, stage, i, 99999999);
            }
        }

        fileOverwriteSaveSlotWithNewSave(&save[0], &new_save);
    }
}

/**
 *
 *
 * @param foldernum
 * @param cheat
 */
void fileSaveFolderUnlockCheat(s32 foldernum, s32 cheat)
{
    if ((foldernum >= FOLDER1) && (foldernum < MAX_FOLDER_COUNT) && (cheat >= 0) && (cheat < CHEAT_INPUT_BUFFER_SIZE))
    {
        save_data *save = fileGetSaveForFoldernum(foldernum);

        if (save && fileGetIsCheatUnlocked(save, cheat))
        {
           return;
        }

        {
            save_data new_save = BLANKSAVEDATA;

            if (save)
            {
                new_save = *save;
            }
            else
            {
                fileSetSaveFoldernum(&new_save, foldernum);
            }

            fileSetSaveCheatUnlocked(&new_save, cheat);
            fileOverwriteSaveSlotWithNewSave(save, &new_save);
        }
    }
}




void fileGetHighestStageDifficultyCompletedForFolder(s32 foldernum, LEVEL_SOLO_SEQUENCE *levelid, DIFFICULTY *difficulty)
{
    save_data *folder;
    LEVEL_SOLO_SEQUENCE stageid;
    DIFFICULTY difficultyid;

    folder = fileGetSaveForFoldernum(foldernum);

    if (folder)
    {
        for (difficultyid = DIFFICULTY_007; difficultyid >= DIFFICULTY_AGENT; difficultyid--)
        {
            for (stageid = SP_LEVEL_EGYPT; stageid >= SP_LEVEL_DAM; stageid--)
            {
                if ( fileGetSaveStageCompletedForDifficulty(folder, stageid, difficultyid))
                {
                    *levelid = stageid;
                    *difficulty = difficultyid;
                    return;
                }
            }
        }
    }
    *levelid = SP_LEVEL_DAM - 1;
    *difficulty = DIFFICULTY_MULTI;
}

/**
 * Get the highest stage unlocked in folder
 *
 * @param foldernum
 * @return LEVEL_SOLO_SEQUENCE
 */
LEVEL_SOLO_SEQUENCE fileGetHighestStageUnlockedForFolder(s32 foldernum)
{
    LEVEL_SOLO_SEQUENCE levelid;
    DIFFICULTY difficulty;

    if ( fileGetSaveForFoldernum(foldernum) != NULL)
    {
        for (levelid = SP_LEVEL_EGYPT; levelid >= SP_LEVEL_DAM; levelid--)
        {
            for (difficulty = DIFFICULTY_AGENT; difficulty < DIFFICULTY_MAX; difficulty++)
            {
                if ( fileIsStageUnlockedAtDifficulty(foldernum, levelid, difficulty))
                {
                    return levelid;
                }
            }
        }
    }
    return SP_LEVEL_DAM;
}

/**
 * Get the highest stage unlocked in any folder
 *
 * @return levelid
 */
LEVEL_SOLO_SEQUENCE fileGetHighestStageUnlockedAnyFolder(void)
{
    int folder;
    LEVEL_SOLO_SEQUENCE isfound;
    LEVEL_SOLO_SEQUENCE highest = SP_LEVEL_DAM;

    for (folder = FOLDER1; folder < MAX_FOLDER_COUNT; folder++)
    {
        isfound = fileGetHighestStageUnlockedForFolder(folder);
        if (highest < isfound)
        {
            highest = isfound;
        }
    }

    return highest;
}

/**
 * Check if cradle has been completed at any difficulty
 *
 * @param foldernum
 * @return bool
 */
bool fileIsCradleCompletedForFolder(s32 folder)
{
    return (fileIsStageUnlockedAtDifficulty(folder, SP_LEVEL_CRADLE, DIFFICULTY_AGENT) == STAGESTATUS_COMPLETED) ||
           (fileIsStageUnlockedAtDifficulty(folder, SP_LEVEL_CRADLE, DIFFICULTY_SECRET) == STAGESTATUS_COMPLETED) ||
           (fileIsStageUnlockedAtDifficulty(folder, SP_LEVEL_CRADLE, DIFFICULTY_00) == STAGESTATUS_COMPLETED);
}

/**
 * Check if aztec has been completed at secret or 00 difficulty
 *
 * @param folder
 * @return bool
 */
bool fileIsAztecCompletedOnSecretOr00ForFolder(s32 folder)
{
    return (fileIsStageUnlockedAtDifficulty(folder, SP_LEVEL_AZTEC, DIFFICULTY_SECRET) == STAGESTATUS_COMPLETED) ||
           (fileIsStageUnlockedAtDifficulty(folder, SP_LEVEL_AZTEC, DIFFICULTY_00) == STAGESTATUS_COMPLETED);
}

/**
 * Check if egypt is completed at 00 difficulty
 *
 * @param foldernum
 * @return bool
 */
bool fileIsEgyptCompletedOn00ForFolder(int foldernum)
{
    return fileIsStageUnlockedAtDifficulty(foldernum, SP_LEVEL_EGYPT, DIFFICULTY_00) == STAGESTATUS_COMPLETED;
}

/**
 * Check if cradle has been completed in any folder
 *
 * @return bool
 */
bool fileIsCradleCompletedAnyFolder(void)
{
    s32 folder;

    for (folder = FOLDER1; folder < MAX_FOLDER_COUNT; folder++)
    {
        if ( fileIsCradleCompletedForFolder(folder))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if aztec has been completed in any folder at secret or 00 difficulty
 *
 * @return bool
 */
bool check_aztec_completed_any_folder_secret_00(void)
{
    s32 folder;

    for (folder = FOLDER1; folder < MAX_FOLDER_COUNT; folder++)
    {
        if ( fileIsAztecCompletedOnSecretOr00ForFolder(folder))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Check if Egypt has been completed in any folder at secret or 00 difficulty
 *
 * @return bool
 */
bool fileIsEgyptCompletedOn00AnyFolder(void)
{
    s32 folder;

    for (folder = FOLDER1; folder < MAX_FOLDER_COUNT; folder++)
    {
        if ( fileIsEgyptCompletedOn00ForFolder(folder))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * Get bond for folder
 *
 * @param folder
 * @return u8
 */
u8 fileGetBondForFolder(u32 folder)
{
#ifdef ALL_BONDS
    //likely code based on behavior
    if ((folder >= FOLDER1) && (folder < MAX_FOLDER_COUNT))
    {
        return save_selected_bond[folder];
    }
#endif

    return BOND_BROSNAN;
}

/**
 * Set the selected bond to folder object
 *
 * @param folder
 * @param bond
 */
void fileSetSelectedBondTofolder(s32 folder, s32 bond)
{
    if (folder < FOLDER1 || folder > FOLDER4)
    {
        return;
    }
#ifdef ALL_BONDS
    save_selected_bond[folder] = bond;
#else
    save_selected_bond[folder] = BOND_BROSNAN;
#endif
}

/**
 *
 *
 * @param unused
 */
void sub_GAME_7F01EBF4(u32 unused)
{
    return;
}

/**
 *
 *
 * @param unused
 */
void sub_GAME_7F01EBFC(u32 unused)
{
    return;
}

/**
 * Delete save at foldernum
 *
 * @param foldernum
 */
void fileDeleteSaveForFolder(s32 foldernum)
{
    save_data *save;
    LEVEL_SOLO_SEQUENCE levelid;
    DIFFICULTY difficulty;

    if (foldernum >= FOLDER1 && foldernum < MAX_FOLDER_COUNT)
    {
        save = fileGetSaveForFoldernum(foldernum);
        if (save)
        {
            fileGetHighestStageDifficultyCompletedForFolder(foldernum, &levelid, &difficulty);
            if ((levelid >= SP_LEVEL_DAM) && (difficulty >= DIFFICULTY_AGENT))
            {
                save_data new_save = BLANKSAVEDATA;
                *save = new_save;
                fileSetSaveFoldernum(save, foldernum);
                fileSetSaveFlagDoReset(save, FALSE);
                fileSetSelectedBond(save, foldernum);
                fileSetSelectedBondTofolder(foldernum, foldernum);
#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)
                fileSramExtInvalidateFolder(foldernum);
#endif
                fileWriteSave(save);
            }
        }
    }
}

/**
 *
 *
 * Resetting times??
 * @param folder
 */
void fileInitializeAllTimes(u32 folder)
{
    save_data *save;
    LEVEL_SOLO_SEQUENCE levelid;
    DIFFICULTY difficulty;

    save = fileGetSaveForFoldernum(folder);

    for (levelid = SP_LEVEL_DAM; levelid < SP_LEVEL_MAX; levelid++)
    {
        for(difficulty = DIFFICULTY_AGENT; difficulty < DIFFICULTY_007; difficulty++)
        {
            fileCheckSaveStageDifficultyTime(save, levelid, difficulty, 99999999);
        }
    }
}

/**
 * Copy save
 *
 * Copies selected save to the first free slot
 * if no free slot, do nothing
 * @param foldernum Current folder number
 */
void fileCopyFolderToFirstFree(s32 foldernum)
{
    save_data* save;
    LEVEL_SOLO_SEQUENCE levelid;
    DIFFICULTY difficulty;
    s32 other;

    if ((foldernum >= FOLDER1) && (foldernum < MAX_FOLDER_COUNT))
    {
        save = fileGetSaveForFoldernum(foldernum);
        if (save)
        {
            fileGetHighestStageDifficultyCompletedForFolder(foldernum, &levelid, &difficulty);
            if (levelid >= SP_LEVEL_DAM)
            {
                if (difficulty >= DIFFICULTY_AGENT) {
                    for(other = FOLDER1;other != MAX_FOLDER_COUNT; other++)
                    {
                            if (( fileGetSaveForFoldernum(other) == NULL) ||
                                ( fileGetHighestStageDifficultyCompletedForFolder(other, &levelid, &difficulty),
                                (levelid < SP_LEVEL_DAM) && (difficulty < DIFFICULTY_AGENT)))
                            {
                                break;
                            }
                    }

                    if ((s32)other < MAX_FOLDER_COUNT)
                    {
                        save_data new_save = BLANKSAVEDATA;
                        save_data *temp_s2 = fileGetSaveForFoldernum(other);
                        new_save = *save;
                        fileSetSaveFoldernum(&new_save, other);
                        fileSetSelectedBondTofolder(other, fileGetBondForFolder(foldernum));
                        fileOverwriteSaveSlotWithNewSave(temp_s2, &new_save);
                    }
                }
            }
        }
    }
}

/**
 *
 *
 * @param save
 */
void fileSaveSettingsForFolder(save_data *save)
{
    u32 temp;
    u16 bits;

    bits = 0;
    save->music_vol = get_mTrack2Vol() >> 7;
    save->sfx_vol = (call_sndGetSfxSlotFirstNaturalVolume() >> 7);

    if (get_cur_player_look_vertical_inverted())
    {
        bits = OPTION_INVERTLOOK;
    }

    if (cur_player_get_autoaim())
    {
        bits |= OPTION_AUTOAIM;
    }

    if (cur_player_get_aim_control())
    {
        bits |= OPTION_AIMCONTROL;
    }

    if (cur_player_get_sight_onscreen_control())
    {
        bits |= OPTION_SIGHTONSCREEN;
    }

    if (cur_player_get_lookahead())
    {
        bits |= OPTION_LOOKAHEAD;
    }

    if (cur_player_get_ammo_onscreen_setting())
    {
        bits |= OPTION_DISPLAYAMMO;
    }

    if (cur_player_get_screen_setting() == SCREEN_SIZE_WIDESCREEN)
    {
        bits |= OPTION_SCREENWIDE;
    }
    else if (cur_player_get_screen_setting() == SCREEN_SIZE_CINEMA)
    {
        bits |= OPTION_SCREENCINEMA;
    }

    if (get_screen_ratio() != SCREEN_RATIO_NORMAL)
    {
        if (get_screen_ratio() & 1)
        {
            bits |= OPTION_SCREENRATIO;
        }
        if (get_screen_ratio() & 2)
        {
            bits |= OPTION_SCREENRATIO2;
        }
    }

    if (cur_player_get_headroll_setting()) bits |= OPTION_HEADROLL;
    if (cur_player_get_crosshair_setting()) bits |= OPTION_CROSSHAIR;
    bits |= OPTION_R21_MIGRATED;

    temp = ((u16) (cur_player_get_control_type() << 8)) & OPTION_CONTROLTYPE;
    save->options = bits | temp;
#ifdef GE_MODDED_CHEATS
    /* V30C: Do not repack TP camera values from this generic settings-save
     * path.  This function can run before the selected folder camera payload
     * has been loaded, which allowed stale RAM adjustments (including values
     * restored by emulator save states) to be stamped into EEPROM with the
     * newest signature before migration got a chance to reset them.
     *
     * TP camera values are already persisted explicitly at their own Watch /
     * frontend edit sites, and migration writes them directly when a folder is
     * loaded. */
#endif
}

/**
 * Loads settings from save file
 *
 * @param folder
 */

void fileLoadSettingsForFolder(u32 folder)
{
    save_data *save;
    u16 padding;
    u16 options;

    save = fileGetSaveForFoldernum(folder);
    if (save)
    {
        set_mTrack2Vol((save->music_vol << 7) | (save->music_vol >> 1));
        sub_GAME_7F0A91A0((save->sfx_vol << 7) | (save->sfx_vol >> 1));

        options = save->options;

        /* R21 save migration: all legacy saves predate Head Roll/Crosshair.
         * Preserve every old option bit, default Head Roll to On and
         * Crosshair to Off, then mark the in-memory save as migrated. */
        if (!(options & OPTION_R21_MIGRATED))
        {
            options |= OPTION_HEADROLL | OPTION_R21_MIGRATED;
            options &= ~OPTION_CROSSHAIR;
            save->options = options;
        }
#ifdef GE_MODDED_CHEATS
        /* V33: V32 used 0xa0 as a three-bit options marker.  Bit 5 is now
         * Directional Shoulder View Toggle, so migrate that exact legacy
         * marker by clearing the reclaimed bit and installing the new marker.
         * This makes the new option default Off without disturbing Crouch Cam,
         * Micro-optimizations, or the camera-pack bits in positions 1..3. */
        if ((save->mod_options3 & MODOPT3_LEGACY_SIGNATURE_MASK) == MODOPT3_LEGACY_SIGNATURE_V32)
        {
            save->mod_options3 = (save->mod_options3 & 0x1f) | MODOPT3_SIGNATURE;
            fileWriteSave(save);
        }
        else if ((save->mod_options3 & MODOPT3_SIGNATURE_MASK) != MODOPT3_SIGNATURE)
        {
            if (!(save->mod_options2 & MODOPT2_R21_MIGRATED))
            {
                options &= ~OPTION_INVERTLOOK;
                save->options = options;
                save->mod_options2 = DEFAULT_MOD_OPTIONS2;
            }
            else if (!(save->mod_options2 & MODOPT2_REVERSE_DEFAULT))
            {
                options &= ~OPTION_INVERTLOOK;
                save->options = options;
                save->mod_options2 |= MODOPT2_REVERSE_DEFAULT;
            }
            save->mod_options2 &= ~MODOPT2_DISABLE_NOISE_DITHER;
            save->mod_options3 = DEFAULT_MOD_OPTIONS3;
            fileWriteSave(save);
        }
#if defined(GE_SAVE_SRAM) || defined(GE_SAVE_EEPROM16K)
        if (!fileSramExtLoadFolder(folder, save))
        {
            g_ModGameplayOptions2 = save->mod_options2;
            g_ModGameplayOptions3 = save->mod_options3;
            g_ModTpSightTranslucencyEnabled = TRUE;
            g_ModThirdPersonCrouchCameraHeightAdjust = 0;
            fileLoadThirdPersonCameraSettings(save);
            fileStoreExtendedSettings(save);
        }
#else
        g_ModGameplayOptions2 = save->mod_options2;
        g_ModGameplayOptions3 = save->mod_options3;
        g_ModTpSightTranslucencyEnabled = TRUE;
        fileLoadThirdPersonCameraSettings(save);
#endif
#endif

        if (getPlayerCount() == 1)
        {
            cur_player_set_control_type(((s32) (options & OPTION_CONTROLTYPE) >> 8) & 0xFFFF);
        }
        else
        {
            cur_player_set_control_type(CONTROLLER_CONFIG_HONEY);
        }

        set_cur_player_look_vertical_inverted((options & OPTION_INVERTLOOK) != FALSE);
        cur_player_set_autoaim((options & OPTION_AUTOAIM) != FALSE);
        cur_player_set_aim_control((options & OPTION_AIMCONTROL) != FALSE);
        cur_player_set_sight_onscreen_control((options & OPTION_SIGHTONSCREEN) != FALSE);
        cur_player_set_lookahead((options & OPTION_LOOKAHEAD) != FALSE);
        cur_player_set_ammo_onscreen_setting((options & OPTION_DISPLAYAMMO) != FALSE);

        if (options & OPTION_SCREENCINEMA)
        {
            cur_player_set_screen_setting(SCREEN_SIZE_CINEMA);
        }
        else if (options & OPTION_SCREENWIDE)
        {
            cur_player_set_screen_setting(SCREEN_SIZE_WIDESCREEN);
        }
        else
        {
            cur_player_set_screen_setting(SCREEN_SIZE_FULLSCREEN);
        }

        set_screen_ratio((SCREEN_RATIO_OPTION)
            (((options & OPTION_SCREENRATIO) ? 1 : 0)
            | ((options & OPTION_SCREENRATIO2) ? 2 : 0)));
        cur_player_set_headroll_setting((options & OPTION_HEADROLL) != FALSE);
        cur_player_set_crosshair_setting((options & OPTION_CROSSHAIR) != FALSE);
    }
}

/**
 * Resets the folder save to default
 *
 * @param folder
 */
void fileClearSavefileForFolder(s32 folder)
{
    if (folder >= FOLDER1 && folder < MAX_FOLDER_COUNT)
    {
        save_data *save = fileGetSaveForFoldernum(folder);

        save_data save_to_copy = BLANKSAVEDATA;
        save_data new_save;
        if (save)
        {
            save_to_copy = *save;
        }
        else
        {
            fileSetSaveFoldernum(&save_to_copy, folder);
        }

        new_save = save_to_copy;

        fileSaveSettingsForFolder(&new_save);

        if (memcmp(&new_save, &save_to_copy, sizeof(save_data)))
        {
            fileOverwriteSaveSlotWithNewSave(save, &new_save);
        }
    }
}

/**
 * Not confident in the naming of this one...
 *
 * @param folder
 */
void fileUpdateSelectedBondInSave(s32 folder)
{

    if (folder >= FOLDER1 && folder < MAX_FOLDER_COUNT)
    {
        save_data *save = fileGetSaveForFoldernum(folder);
        save_data new_save = BLANKSAVEDATA;

        if (save)
        {
            new_save = *save;
        }
        else
        {
            fileSetSaveFoldernum(&new_save, folder);
        }

        if (save_selected_bond[folder] != fileGetSelectedBond(&new_save))
        {
            fileSetSelectedBond(&new_save, save_selected_bond[folder]);
            fileOverwriteSaveSlotWithNewSave(save, &new_save);
        }
    }
}

/**
 * Copy folder save
 *
 * @param foldernum
 * @param out_save
 */
void fileCopySave(s32 folder, save_data *out_save)
{

    save_data *in_save = fileGetSaveForFoldernum(folder);

    if (in_save)
    {
        *out_save = *in_save;
    }
    else
    {
        save_data new_save = BLANKSAVEDATA;
        *out_save = new_save;
    }
}

/**
 * Copy save to RarRom replay save
 *
 * @param folder
 * @param save
 */
void fileCopyDemoSaveToRamRomSave(u32 folder, save_data *save)
{
    if (folder == RAMROM_FOLDERNUM)
    {
        saves[SAVESLOTRAMROM] = *save;
    }
}

/**
 * Check if 007 mode is unlocked
 *
 * @param folder
 * @return bool
 */
s32 fileIs007ModeUnlocked(u32 folder)
{
    LEVEL_SOLO_SEQUENCE levelid;
    save_data* save;

    save = fileGetSaveForFoldernum(folder);

    if (save != NULL)
    {
        if ((save->flag_007 & 1))
        {
            return TRUE;
        }

        for (levelid = SP_LEVEL_DAM; levelid < SP_LEVEL_MAX; levelid++)
        {
            if (! fileGetSaveStageCompletedForDifficulty(save, levelid, DIFFICULTY_00))
            {
                break;
            }
        }

        if (levelid == SP_LEVEL_MAX)
        {
            return TRUE;
        }
    }

    return FALSE;
}
