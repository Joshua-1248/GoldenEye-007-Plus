#include <ultra64.h>
#include "include/limits.h"
#include <bondconstants.h>
#include <bondtypes.h>
#include <bondgame.h>
#include <music.h>
#include <snd.h>
#include "bondview.h"
#include "bondinv.h"
#include "gun.h"
#include "chrobjdata.h"
#include "game/propobj.h"
#include "game/objective_status.h"
#include "quaternion.h"
#include "image_bank.h"
#include "bondwalk2.h"
#include "othermodemicrocode.h"
#include "player.h"
#include "lv.h"
#include "random.h"
#include "math_asinfacosf.h"
#include "loadobjectmodel.h"
#include "objecthandler.h"
#include "image.h"
#include "tex.h"
#include "debugmenu_handler.h"
#include "fr.h"
#include "assets/obseg/text/LgunE.h"
#include "textrelated.h"
#include "chrai.h"
#ifdef GE_MODDED_CHEATS
#include "cheat.h"
#endif
#include "model.h"
#include "options.h"
#include "mpmenu.h"
#include "joy.h"
#include "matrixmath.h"
#include "bondinv.h"
#include "stan.h"
#include "gbi_extension.h"


#ifdef REFRESH_PAL
#define THROWN_ITEM_REFRESH_RATE 50
#define DUAL_WIELD_TRIGGER_SWAP_TICKS 24
#define DUAL_WIELD_SINGLE_TRIGGER_SWAP_TICKS 36
#define WATCH_SOUND_DURATION_TICKS 250
#define GUN_SPRING_DAMP 0.9402999877929688f
#define GUN_SPRING_SCALE 0.05970001220703125f
#else
#define THROWN_ITEM_REFRESH_RATE 60
#define DUAL_WIELD_TRIGGER_SWAP_TICKS 20
#define DUAL_WIELD_SINGLE_TRIGGER_SWAP_TICKS 30
#define WATCH_SOUND_DURATION_TICKS 300
#define GUN_SPRING_DAMP 0.95f
#define GUN_SPRING_SCALE 0.050000012f
#endif

extern f32 g_TankShellSpeed;

extern coord3d D_80035C40;
extern coord3d D_80035C4C;
extern coord3d D_80035C58;
extern coord3d D_80035C64;
extern coord3d D_80035C70;
extern coord3d D_80035C7C;
extern coord3d D_80035C88;
extern Vtx D_80035C98;
extern coord3d D_80035CA8;
extern coord3d D_80035CB4;
extern u32 D_80035CC0;
extern u32 D_80035D00;
extern u32 D_80035D04[];
extern u32 D_80035EA4;
extern u32 watchControllerButtonBases[];
extern GunModelFileRecord gitem_structs[];
extern struct gun_trigger_state g_ZeroTriggerState;
extern Lights1 g_WeaponEnvmapLight;
extern ALSoundState *g_ImpactSfxStates[NUM_IMPACT_SFX_STATES];
extern struct RicochetSoundsSmall ricochet_sounds_small;
extern struct PunchSounds punch_sounds;
extern struct BulletFleshSounds bullet_flesh_sounds;
extern struct LaserRichochetSounds laser_ricochet_sounds;
extern struct RicochetSoundsLarge ricochet_sounds_large;
extern struct EarWhistleSounds ear_whistle_sounds;
extern struct sfx2 watchlaser_fire_sounds;
extern struct sfx3 knife_throw_sounds;
extern u32 D_80032458;
extern u32 D_80034CA4[];
extern u32 D_80034E0C[];
extern Weapon1PTransformKeyframe throwKnifeDrawBackKeyframes[];
extern Weapon1PTransformKeyframe throwKnifeReleaseKeyframes[];
extern Weapon1PTransformKeyframe grenadeThrowKeyframes[];
extern Weapon1PTransformKeyframe timedMineThrowKeyframes[];
extern Weapon1PTransformKeyframe proxMineThrowKeyframes[];
extern Weapon1PTransformKeyframe remoteMineThrowKeyframes[];
extern Weapon1PTransformKeyframe fistMeleeKeyframes1[];
extern Weapon1PTransformKeyframe fistMeleeKeyframes2[];
extern Weapon1PTransformKeyframe sniperMeleeKeyframes1[];
extern Weapon1PTransformKeyframe sniperMeleeKeyframes2[];
extern Weapon1PTransformKeyframe taserFireKeyFrames[];
extern Weapon1PTransformKeyframe taserRaiseKeyframes[];
extern struct ModelSkeleton skeleton_gun_kf7;

typedef struct ModelHeader {
    s16 unk00;
    s16 Type;
    struct ChrRecord *chr;
    ModelFileHeader *obj;
    RenderPosView *render_pos;
    union ModelRwData **datas;
    f32 scale;
    struct Model *attachedto;
    ModelNode *attachedto_objinst;
} ModelHeader;

void gunCreateBeamForHand(enum GUNHAND hand);
void bullet_path_from_screen_center(coord3d *arg0, coord3d *arg1, enum GUNHAND arg2);
void gunInitProjectileFromPlayer(ObjectRecord *obj, coord3d *targetpos, Mtxf *arg2, coord3d *velocity, Mtxf *arg4);
s32 gunSample1PTransform(Weapon1PTransformKeyframe *keyframes, f32 time, Mtxf *matrix, GUNHAND hand);
void analyzeGEKey(void);
void give_weapon_case_items(void);
struct ModelFileHeader *get_ptr_weapon_model_header_line(ITEM_IDS weapon);
s32 get_ammo_in_hands_weapon(enum GUNHAND hand);
s32 get_ammo_type_for_weapon(ITEM_IDS weapon);
f32 gunSetHorizontalOffset(GUNHAND hand);
f32 get_value_if_watch_is_on_hand_or_not(GUNHAND hand);
void sub_GAME_7F05DA8C(GUNHAND hand, ITEM_IDS weaponnum_watchmenu);
void sub_GAME_7F05E808(GUNHAND hand);
void sub_GAME_7F05EA94(Model *model, s32 val);
void sub_GAME_7F0649D8(enum GUNHAND hand);
void sub_GAME_7F068508(GUNHAND handnum, f32 floor_y_pos);
Vtx *dynAllocateVertices(s32 count);
Mtx *dynAllocateMatrix(void);
void divide3DCoordinates(coord3d *in, f32 divisor, coord3d *out);

#if !defined(VERSION_EU)
void sub_GAME_7F05FB00(enum GUNHAND hand)
{
    struct hand* hand_ptr;
    ObjectRecord* hand_obj_record;

    hand_ptr = &g_CurrentPlayer->hands[hand];
    hand_obj_record = hand_ptr->rocket;

    if (hand_obj_record != NULL)
    {
        objFreePermanently(hand_obj_record, 1);
        hand_ptr->rocket = NULL;
    }
}


extern f32 D_80053DDC;

/*
* Address: 0x7F05FB64
*/
void gunFireTankShell(s32 handnum)
{
    WeaponObjRecord *obj;
    struct hand *hand;
    Mtxf identitymtx;
    coord3d velocity;
    ObjectRecord *tankobj;
    coord3d unscaledvelocity;
    Mtxf shellmtx;
    coord3d screenpos;
    coord3d aimdir;
    PropRecord *playerprop;
    coord3d *prevplayerpos;
    ITEM_IDS weaponid;
    coord3d spawnpos;
    PropRecord *tankprop;

    hand = &g_CurrentPlayer->hands[handnum];

    playerprop = getCurrentPlayerProp();
    prevplayerpos = getCurrentPlayerPrevPos();
    weaponid = getCurrentPlayerWeaponId(handnum);

    matrix_4x4_set_identity(&identitymtx);

    if (weaponid == ITEM_TANKSHELLS) 
    {
        tankprop = get_ptr_for_players_tank();

        if (1);

        if ((tankprop != NULL) && (tankprop->flags & TANK_RUN_STATE_RUNNING)) 
        {
            bondviewSet3dCoord7F07CEB0(&aimdir);
        } 
        else 
        {
            sub_GAME_7F068190(&screenpos, &aimdir);
            mtx4RotateVecInPlace(currentPlayerGetViewToWorldMtxf(), &aimdir);
        }

        velocity.x = aimdir.x * g_TankShellSpeed;
        velocity.y = aimdir.y * g_TankShellSpeed;
        velocity.z = aimdir.z * g_TankShellSpeed;

        if (g_ClockTimer > 0) {
            velocity.x += (playerprop->pos.x - prevplayerpos->x) / g_GlobalTimerDelta;
            velocity.y += (playerprop->pos.y - prevplayerpos->y) / g_GlobalTimerDelta;
            velocity.z += (playerprop->pos.z - prevplayerpos->z) / g_GlobalTimerDelta;
        }

        if ((tankprop != NULL) && (tankprop->flags & TANK_RUN_STATE_RUNNING)) 
        {
            tankobj = tankprop->obj;
            spawnpos.x = tankobj->model->render_pos[4].pos.m[3][0];
            spawnpos.y = tankobj->model->render_pos[4].pos.m[3][1];
            spawnpos.z = tankobj->model->render_pos[4].pos.m[3][2];

            mtx4TransformVecInPlace(currentPlayerGetViewToWorldMtxf(), &spawnpos);
        } 
        else 
        {
            spawnpos.x = playerprop->pos.x;
            spawnpos.y = playerprop->pos.y;
            spawnpos.z = playerprop->pos.z;
        }

        if ((g_CurrentPlayer && g_CurrentPlayer));

        setSixExplosionAndSmokeEntries();
    } 
    else 
    {
        bullet_path_from_screen_center(&screenpos, &aimdir, handnum);
        mtx4RotateVecInPlace(currentPlayerGetViewToWorldMtxf(), &aimdir);

        spawnpos.x = hand->field_B58.x;
        spawnpos.y = hand->field_B58.y;
        spawnpos.z = hand->field_B58.z;

        if (1);

        unscaledvelocity.x = aimdir.x * D_80053DDC;
        unscaledvelocity.y = aimdir.y * D_80053DDC;
        unscaledvelocity.z = aimdir.z * D_80053DDC;

        velocity.x = unscaledvelocity.x * g_GlobalTimerDelta;
        velocity.y = unscaledvelocity.y * g_GlobalTimerDelta;
        velocity.z = unscaledvelocity.z * g_GlobalTimerDelta;

        if (g_ClockTimer > 0) 
        {
            velocity.x += (playerprop->pos.x - prevplayerpos->x) / g_GlobalTimerDelta;
            velocity.y += (playerprop->pos.y - prevplayerpos->y) / g_GlobalTimerDelta;
            velocity.z += (playerprop->pos.z - prevplayerpos->z) / g_GlobalTimerDelta;
        }
    }

    matrix_4x4_copy(&g_CurrentPlayer->hands[handnum].throw_item_pos_related, &shellmtx);

    shellmtx.m[3][0] = 0.0f;
    shellmtx.m[3][1] = 0.0f;
    shellmtx.m[3][2] = 0.0f;

    if (hand->rocket != NULL) 
    {
        obj = (WeaponObjRecord *) hand->rocket;
        hand->firedrocket = 1;
    } 
    else 
    {
        obj = (WeaponObjRecord *) create_new_item_instance_of_model(PROP_CHRROCKET, ITEM_ROCKETROUND);
    }

    if (obj == NULL) 
    {
        return;
    }

    obj->timer = -1;
    obj->runtime_bitflags &= ~RUNTIMEBITFLAG_OWNER;
    obj->runtime_bitflags |= get_cur_playernum() << RUNTIMEBITSHIFT_OWNER;

    gunInitProjectileFromPlayer(obj, &spawnpos, &shellmtx, &velocity, (s32 *) &identitymtx);

    if (obj->runtime_bitflags & RUNTIMEBITFLAG_00000080)
    {
        obj->projectile->flags |= PROJECTILEFLAG_LAUNCHING;

        if (weaponid != ITEM_TANKSHELLS)
        {
            obj->projectile->flags |= PROJECTILEFLAG_00000020;
            obj->projectile->unkB0 = obj->runtime_pos.y;
            obj->projectile->unkB4 = obj->projectile->speed.y;
            obj->projectile->unk10.x = unscaledvelocity.x;
            obj->projectile->unk10.y = unscaledvelocity.y;
            obj->projectile->unk10.z = unscaledvelocity.z;
            obj->projectile->refreshrate = THROWN_ITEM_REFRESH_RATE;

            if (obj->projectile->sounds[0] == NULL)
            {
                sndPlaySfx(g_musicSfxBufferPtr, 1, &obj->projectile->sounds[0]);
            } 
            else if (obj->projectile->sounds[1] == NULL)
            {
                sndPlaySfx(g_musicSfxBufferPtr, 1, &obj->projectile->sounds[1]);
            }
        }
    }
}


/**
 * D_80053DDC belongs to gunFireTankShell, but must sit here
 * immediately ahead of this function's pool for that function to match.
 */
#endif

void gunUpdateAndFire(GUNHAND handnum)
{
    Mtxf *rwmtx;
    Mtxf gunmtx;
    Mtxf flashmtx;
    Mtxf flash2mtx;
    Mtxf tmpmtx;
    ModelFileHeader *mdlhdr;
    coord3d gunofs = D_80035C40;
    Mtxf rotmtx;
    Mtxf aimmtx;
    struct hand *hand;
    s32 *flashvisptr;
    f32 *flashdata;
    ModelNode *node;
    s32 j;
    ITEM_IDS item;
    WeaponStats *itemstats;
    f32 rndf;
    u32 rnd;
    f32 stackpad_scale;
    coord3d blendedpos;
    coord3d blendedlook;
    coord3d blendedup;
    s32 i;
    coord3d trigrot;
    coord3d taserrot;
    coord3d fistrot;
    f32 *cylinderdata;
    u8 stackpad1[4];
    ModelNode **hammerdata;
    Model *model;
    coord3d flashpos;
    f32 flashscale;
    f32 flashext;
    f32 *nodeptr;
    f32 *nodepos;
    f32 *swdata;
    s32 sw6mtxidx;
    f32 *hingedata;
    f32 *sw7data;
    s32 sw7mtxidx;
    s32 shellidx;
    u8 stackpad2[4];

    flashvisptr = NULL;
    flashdata = NULL;
    hand = &g_CurrentPlayer->hands[handnum];
    item = get_item_in_hand_or_watch_menu(handnum);
    itemstats = get_ptr_item_statistics(item);

    /**
     * When switching from a single weapon to dual wielding, both gun models interpolate a little to the sides of the screen
     * over a period of 4 seconds. And when switching back to a single weapon that weapon moves back more towards the
     * center of the screen.
     */
    if (handnum == GUNRIGHT)
    {
        if (bondwalkItemCheckBitflags(get_item_in_hand_or_watch_menu(1), WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) != 0)
        {
            hand->field_A34 += (2.0f * g_GlobalTimerDelta) / 240.0f;

            if (hand->field_A34 > 2.0f)
            {
                hand->field_A34 = 2.0f;
            }
        }
        else
        {
            hand->field_A34 -= (2.0f * g_GlobalTimerDelta) / 240.0f;

            if (hand->field_A34 < 0.0f)
            {
                hand->field_A34 = 0.0f;
            }
        }
    }
    else if (bondwalkItemCheckBitflags(get_item_in_hand_or_watch_menu(0), WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) != 0)
    {
        hand->field_A34 -= (2.0f * g_GlobalTimerDelta) / 240.0f;

        if (hand->field_A34 < (-2.0f))
        {
            hand->field_A34 = -2.0f;
        }
    }
    else
    {
        hand->field_A34 += (2.0f * g_GlobalTimerDelta) / 240.0f;

        if (hand->field_A34 > 0.0f)
        {
            hand->field_A34 = 0.0f;
        }
    }

    /**
     * Gun sway system. This moves the held weapons in figure-eight pattern which becomes bigger depending how fast the player is moving.
     */
    blendedpos = D_80035C4C;
    blendedlook = D_80035C58;
    blendedup = D_80035C64;

    i = hand->curblendpos;

    coord3dCatmullRomInterp(&hand->blendpos[(i + 3) % 4], &hand->blendpos[i], &hand->blendpos[(i + 1) % 4], &hand->blendpos[(i + 2) % 4], hand->dampt, &blendedpos);
    coord3dCatmullRomInterp(&hand->blendlook[(i + 3) % 4], &hand->blendlook[i], &hand->blendlook[(i + 1) % 4], &hand->blendlook[(i + 2) % 4], hand->dampt, &blendedlook);
    coord3dCatmullRomInterp(&hand->blendup[(i + 3) % 4], &hand->blendup[i], &hand->blendup[(i + 1) % 4], &hand->blendup[(i + 2) % 4], hand->dampt, &blendedup);

    blendedpos.x *= g_CurrentPlayer->gunposamplitude;
    blendedpos.y *= g_CurrentPlayer->gunposamplitude;
    blendedpos.z *= g_CurrentPlayer->gunposamplitude;
    blendedpos.x += hand->weapon_theta_displacement;
    blendedpos.y += hand->weapon_verta_displacement;
    blendedpos.x += sub_GAME_7F05DCB8(handnum);

    j = 0;

    if (g_ClockTimer > 0)
    {
        do
        {
            j += 1;
            hand->spring_pos_x = (GUN_SPRING_DAMP * hand->spring_pos_x) + ((f32 *) (&blendedpos))[0];
            hand->spring_pos_y = (GUN_SPRING_DAMP * hand->spring_pos_y) + ((f32 *) (&blendedpos))[1];
            hand->spring_pos_z = (GUN_SPRING_DAMP * hand->spring_pos_z) + ((f32 *) (&blendedpos))[2];
            hand->spring_look_x = (GUN_SPRING_DAMP * hand->spring_look_x) + ((f32 *) (&blendedlook))[0];
            hand->spring_look_y = (GUN_SPRING_DAMP * hand->spring_look_y) + ((f32 *) (&blendedlook))[1];
            hand->spring_look_z = (GUN_SPRING_DAMP * hand->spring_look_z) + ((f32 *) (&blendedlook))[2];
            hand->spring_up_x = (GUN_SPRING_DAMP * hand->spring_up_x) + ((f32 *) (&blendedup))[0];
            hand->spring_up_y = (GUN_SPRING_DAMP * hand->spring_up_y) + ((f32 *) (&blendedup))[1];
            hand->spring_up_z = (GUN_SPRING_DAMP * hand->spring_up_z) + ((f32 *) (&blendedup))[2];
        }
        while (j < g_ClockTimer);
    }

    hand->sway_pos_x = hand->spring_pos_x * GUN_SPRING_SCALE;
    hand->sway_pos_y = hand->spring_pos_y * GUN_SPRING_SCALE;
    hand->sway_pos_z = hand->spring_pos_z * GUN_SPRING_SCALE;
    hand->sway_look_x = hand->spring_look_x * GUN_SPRING_SCALE;
    hand->sway_look_y = hand->spring_look_y * GUN_SPRING_SCALE;
    hand->sway_look_z = hand->spring_look_z * GUN_SPRING_SCALE;
    hand->sway_up_x = hand->spring_up_x * GUN_SPRING_SCALE;
    hand->sway_up_y = hand->spring_up_y * GUN_SPRING_SCALE;
    hand->sway_up_z = hand->spring_up_z * GUN_SPRING_SCALE;

    // Offset the weapon to the right or left side of the screen depending on which hand it's in.
    if (handnum == GUNRIGHT)
    {
        gunofs.x = (gunSetHorizontalOffset(handnum) + hand->sway_pos_x) + (*hand).gunofs2_x;
    }
    else
    {
        gunofs.x = (gunSetHorizontalOffset(handnum) + hand->sway_pos_x) - hand->gunofs2_x;
    }

    gunofs.y = hand->gunofs2_y + (itemstats->PosY + hand->sway_pos_y);
    gunofs.z = hand->gunofs2_z + (itemstats->PosZ + hand->sway_pos_z);

    if (((item == ITEM_ROCKETLAUNCH) || (item == ITEM_TRIGGER)) || (item == ITEM_WATCHLASER))
    {
        gunofs.y += g_CurrentPlayer->ducking_height_offset / (-100.0f);
        gunofs.z += (3.0f * g_CurrentPlayer->ducking_height_offset) / (-100.0f);

        if ((item == ITEM_ROCKETLAUNCH) && (((cur_player_get_screen_setting() == SCREEN_SIZE_WIDESCREEN) || (cur_player_get_screen_setting() == SCREEN_SIZE_CINEMA)) || (get_screen_ratio() != SCREEN_RATIO_NORMAL)))
        {
            gunofs.y -= 3.0f;
        }
    }
    else if (item == ITEM_TASER)
    {
        gunofs.y += (2.5f * g_CurrentPlayer->ducking_height_offset) / (-100.0f);
        gunofs.z += (7.5f * g_CurrentPlayer->ducking_height_offset) / (-100.0f);
    }
    else
    {
        gunofs.y += (5.0f * g_CurrentPlayer->ducking_height_offset) / (-100.0f);
        gunofs.z += (15.0f * g_CurrentPlayer->ducking_height_offset) / (-100.0f);
    }

    if ((hand->weapon_firing_status != 0) && (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_00000020) != 0))
    {
        if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_00000040) != 0)
        {
            rnd = randomGetNext();
            rndf = (f32) ((u32) rnd);
            gunofs.x += 0.3f - ((rndf * (1.0f / M_U32_MAX_VALUE_F)) * 0.6f);
        }

        rnd = randomGetNext();
        rndf = (f32) ((u32) rnd);
        gunofs.y += 0.3f - ((rndf * (1.0f / M_U32_MAX_VALUE_F)) * 0.6f);
        rnd = randomGetNext();
        rndf = (f32) ((u32) rnd);
        gunofs.z += 0.3f - ((rndf * (1.0f / M_U32_MAX_VALUE_F)) * 0.6f);
    }

    gunofs.x += (((g_CurrentPlayer->field_FFC.x - getPlayer_c_screenleft()) - (getPlayer_c_screenwidth() * 0.5f)) * itemstats->PlayZ) / (getPlayer_c_screenwidth() * 0.5f);

    if ((g_CurrentPlayer->field_FFC.y - getPlayer_c_screentop()) > (getPlayer_c_screenheight() * 0.5f))
    {
        gunofs.y -= (((g_CurrentPlayer->field_FFC.y - getPlayer_c_screentop()) - (getPlayer_c_screenheight() * 0.5f)) * itemstats->PlayY) / (getPlayer_c_screenheight() * 0.5f);
    }
    else
    {
        gunofs.y -= (((g_CurrentPlayer->field_FFC.y - getPlayer_c_screentop()) - (getPlayer_c_screenheight() * 0.5f)) * itemstats->PlayX) / (getPlayer_c_screenheight() * 0.5f);
    }

    sub_GAME_7F05C614();
    matrix_4x4_set_identity(&rotmtx);

    if ((item == ITEM_TRIGGER) || (item == ITEM_WATCHLASER))
    {
            trigrot = D_80035C70;
            matrix_4x4_set_rotation_around_xyz(&trigrot, &tmpmtx);
            matrix_4x4_multiply_homogeneous_in_place(&tmpmtx, &rotmtx);
    }
    else if (item == ITEM_TASER)
    {
        taserrot = D_80035C7C;
        matrix_4x4_set_rotation_around_xyz(&taserrot, &tmpmtx);
        matrix_4x4_multiply_homogeneous_in_place(&tmpmtx, &rotmtx);
    }
    else if ((item == ITEM_FIST) && (g_CurrentPlayer->cur_item_weapon_getname == ITEM_SNIPERRIFLE))
    {
        fistrot = D_80035C88;
        matrix_4x4_set_rotation_around_xyz(&fistrot, &tmpmtx);
        matrix_4x4_multiply_homogeneous_in_place(&tmpmtx, &rotmtx);
        gunofs.x += -2.5f;
        gunofs.y += 27.8f;
        gunofs.z += 2.0f;
    }

    if (hand->field_92C != 0)
    {
        gunofs.x += hand->field_8EC.m[3][0];
        gunofs.y += hand->field_8EC.m[3][1];
        gunofs.z += hand->field_8EC.m[3][2];
        matrix_4x4_multiply_homogeneous_in_place(&hand->field_8EC, &rotmtx);
        rotmtx.m[3][0] = 0.0f;
        rotmtx.m[3][1] = 0.0f;
        rotmtx.m[3][2] = 0.0f;
    }
    else
    {
        hand->field_8E8 = 0.0f;
        hand->field_8DC = (-0.0f) + 0.0f;
        hand->field_8E0 = 0.0f;
        hand->field_8E4 = (-0.0f) + 0.0f;
    }

    matrix_4x4_set_basis_and_position_target(&tmpmtx, 0.0f, 0.0f, 0.0f, hand->sway_look_x, hand->sway_look_y, hand->sway_look_z, hand->sway_up_x, hand->sway_up_y, hand->sway_up_z);
    matrix_4x4_multiply_homogeneous_in_place(&tmpmtx, &rotmtx);
    matrix_4x4_align(&tmpmtx, 0.0f, gunofs.x - hand->field_A38, gunofs.y - hand->field_A3C, gunofs.z - hand->field_A40);
    matrix_4x4_multiply_homogeneous_in_place(&tmpmtx, &rotmtx);
    matrix_4x4_copy(&rotmtx, &gunmtx);
    matrix_4x4_set_position(&gunofs, &gunmtx);
    matrix_4x4_copy(&gunmtx, &hand->gunmtx_camspace);
    matrix_4x4_copy(&hand->throw_item_pos_related, &hand->throw_item_pos_related_prev);
    matrix_4x4_multiply_homogeneous(currentPlayerGetViewToWorldMtxf(), &hand->gunmtx_camspace, &hand->throw_item_pos_related);
    hand->field_87F = 1;

    if (((((((get_ptr_weapon_model_header_line(item) == 0) || (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) == 0)) || (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_HIDE_FIRST_PERSON_HAND) != 0)) 
    || (hand->weapon_action_state == GUN_ANIM_STATE_SWITCH_SWAP)) || (hand->weapon_action_state == GUN_ANIM_STATE_SWITCH_HOLD)) || (Gun_hand_without_item(handnum) == 0)) || (get_itemtype_in_hand(handnum) == 0))
    {
        hand->field_87F = 0;
    }

    if ((hand->weapon_ammo_in_magazine <= 0) && (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_SINGLE_USE_RELOAD) != 0))
    {
        hand->field_87F = 0;
    }

    if (hand->field_87F != 0)
    {
        mdlhdr = &g_CurrentPlayer->copy_of_body_obj_header[handnum];
        rwmtx = (Mtxf *) dynAllocate(mdlhdr->numMatrices * ((s32) (sizeof(Mtxf))));
        j = 0;

        if (mdlhdr->numMatrices > 0)
        {
            do
            {
                matrix_4x4_set_identity((Mtxf *) (((u8 *) rwmtx) + (j * ((s32) (sizeof(Mtxf))))));
                j += 1;
            }
            while (j < mdlhdr->numMatrices);
        }

        modelCalculateRwDataLen(mdlhdr);
#ifdef DEBUG
        /** 
         * The model's runtime data is written into hand->modeldatas, which is a
         * fixed run of 32 words (modeldatas .. field_C04, ending at volley).
         */
        if (mdlhdr->numRecords >= 32)
        {
                osSyncPrintf("Increase GUNSAVESIZE to %d!!! ", mdlhdr->numRecords);
        }
#endif
        model = (Model *) (&hand->field_B68);

        if (mdlhdr->Switches);

        modelInit(model, mdlhdr, (s32 *) (&hand->modeldatas));
        sub_GAME_7F05E978(model, 1);
        sub_GAME_7F05EA94(model, hand->field_87E);
        node = mdlhdr->Switches[1];

        if (node != NULL)
        {
            if (&node->Data->Switch);

            flashvisptr = ((s32 *) (&hand->modeldatas)) + node->Data->Switch.RwDataIndex;
        }

        if (mdlhdr->Switches[3] != NULL)
        {
            flashdata = (f32 *) mdlhdr->Switches[3]->Data;
        }

        hand->mtxlist = rwmtx;

        if ((bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_MIRROR_DUAL) != 0) && (handnum == GUNLEFT))
        {
            matrix_column_1_scalar_multiply(-1.0f, gunmtx.m[0]);
        }

        matrix_scalar_multiply(IDO_POINT_ONE, gunmtx.m[0]);
        matrix_4x4_copy(&gunmtx, rwmtx);

        if (mdlhdr->Skeleton == (&skeleton_gun_revolver))
        {
            swdata = (f32 *) mdlhdr->Switches[4];

            if (swdata != NULL)
            {
                rndf = 0.0f;
                cylinderdata = (f32 *) ((ModelNode *) swdata)->Data;

                if (item == ITEM_RUGER)
                {
                    if (hand->weapon_action_state == 1)
                    {
#if defined(VERSION_EU)
                        rndf = (((hand->field_890 - (hand->weapon_ammo_in_magazine * 5)) + 0x19) * M_TAU_F) / 30.0f;
#else
                        rndf = (((hand->field_890 - (hand->weapon_ammo_in_magazine * 6)) + 0x1E) * M_TAU_F) / 36.0f;
#endif
                    }
                    else
                    {
                        rndf = ((6 - hand->weapon_ammo_in_magazine) * M_TAU_F) / 6.0f;
                    }
                }
                else if (hand->weapon_action_state == 1)
                {
#if defined(VERSION_EU)
                if (hand->field_890 < 5)
                {
                    rndf = (hand->field_890 * M_TAU_F) / 30.0f;
                }
#else
                if (hand->field_890 < 6)
                {
                    rndf = (hand->field_890 * M_TAU_F) / 36.0f;
                }
#endif
            }

                matrix_4x4_set_rotation_around_z(rndf, &tmpmtx);
                matrix_4x4_set_position((coord3d *) cylinderdata, &tmpmtx);
                matrix_4x4_multiply(&gunmtx, &tmpmtx, &rwmtx[3]);
            }

            swdata = (f32 *) mdlhdr->Switches[5];

            if (swdata != NULL)
            {
                hammerdata = (ModelNode **) ((ModelNode *) swdata)->Data;

                if (hand->weapon_action_state == 1)
                {
#if defined(VERSION_EU)
                    if (hand->field_890 < 2)
                    {
                        rndf = (2.0f * ((-((f32) hand->field_890)) * DegToRad(30.0f))) / 5.0f;
                    }
                    else
                    {
                        rndf = (2.0f * ((-((f32) (5 - hand->field_890))) * DegToRad(30.0f))) / 5.0f;
                    }
#else
                    if (hand->field_890 < 3)
                    {
                        rndf = (2.0f * ((-((f32) hand->field_890)) * DegToRad(30.0f))) / 6.0f;
                    }
                    else
                    {
                        rndf = (2.0f * ((-((f32) (6 - hand->field_890))) * DegToRad(30.0f))) / 6.0f;
                    }
#endif
                    matrix_4x4_set_rotation_around_x(rndf, &tmpmtx);
                    matrix_4x4_set_position((coord3d *) hammerdata, &tmpmtx);
                }
                else
                {
                    matrix_4x4_set_identity_and_position((coord3d *) hammerdata, &tmpmtx);
                }

                matrix_4x4_multiply(&gunmtx, &tmpmtx, &rwmtx[4]);
            }
        }

        if (flashvisptr != NULL)
        {
            *flashvisptr = 0;
        }

        if (flashdata != NULL)
        {
            rnd = randomGetNext();
            rndf = (f32) ((u32) rnd);
            flashscale = ((rndf * (1.0f / M_U32_MAX_VALUE_F)) * 0.25f) + 1.0f;
            flashext = itemstats->MuzzleFlashExtension;

            if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_00000001) != 0)
            {
                rnd = randomGetNext();
                rndf = (f32) ((u32) rnd);
                matrix_4x4_set_rotation_around_z((rndf * (1.0f / M_U32_MAX_VALUE_F)) * M_TAU_F, &flashmtx);
                matrix_4x4_set_position((coord3d *) flashdata, &flashmtx);
            }
            else
            {
                matrix_4x4_set_identity_and_position((coord3d *) flashdata, &flashmtx);
            }

            matrix_scalar_multiply(flashscale, flashmtx.m[0]);
            matrix_column_3_scalar_multiply(flashext, flashmtx.m[0]);
            matrix_4x4_multiply_in_place(&gunmtx, &flashmtx);
            matrix_4x4_copy(&flashmtx, &rwmtx[1]);

            hand->field_B58.x = flashmtx.m[3][0];
            hand->field_B58.y = flashmtx.m[3][1];
            hand->field_B58.z = flashmtx.m[3][2];
            mtx4TransformVecInPlace(currentPlayerGetViewToWorldMtxf(), &hand->field_B58);
            hand->field_B64 = -flashmtx.m[3][2];

            if (hand->field_87D != 0)
            {
                if (flashvisptr != NULL)
                {
                    *flashvisptr = 1;
                }

                nodeptr = (f32 *) mdlhdr->Switches[2];

                if (nodeptr != NULL)
                {
                    nodepos = (f32 *) ((ModelNode *) nodeptr)->Data;
                    flashpos.x = (((((f32 *) nodepos)[0] * flashmtx.m[0][0]) + (((f32 *) nodepos)[1] * flashmtx.m[1][0])) + (((f32 *) nodepos)[2] * flashmtx.m[2][0])) + flashmtx.m[3][0];
                    flashpos.y = (((((f32 *) nodepos)[0] * flashmtx.m[0][1]) + (((f32 *) nodepos)[1] * flashmtx.m[1][1])) + (((f32 *) nodepos)[2] * flashmtx.m[2][1])) + flashmtx.m[3][1];
                    flashpos.z = (((((f32 *) nodepos)[0] * flashmtx.m[0][2]) + (((f32 *) nodepos)[1] * flashmtx.m[1][2])) + (((f32 *) nodepos)[2] * flashmtx.m[2][2])) + flashmtx.m[3][2];

                    matrix_4x4_align(&flash2mtx, (randomGetNext() * (1.0f / M_U32_MAX_VALUE_F)) * M_TAU_F, -flashpos.x, -flashpos.y, -flashpos.z);
                    matrix_scalar_multiply(IDO_POINT_ONE * flashscale, flash2mtx.m[0]);
                    matrix_4x4_set_rotation_axis_angle(&aimmtx, 0, gunofs.x - hand->field_A38, gunofs.y - hand->field_A3C, gunofs.z - hand->field_A40);
                    matrix_4x4_multiply_in_place(&aimmtx, &flash2mtx);
                    matrix_row_3_scalar_multiply(flashext, flash2mtx.m[0]);
                    matrix_4x4_multiply_in_place(&rotmtx, &flash2mtx);
                    matrix_4x4_set_position(&flashpos, &flash2mtx);
                    matrix_4x4_copy(&flash2mtx, &rwmtx[2]);
                }

                if (mdlhdr->Skeleton == (&skeleton_gun_kf7))
                {
                    nodeptr = (f32 *) mdlhdr->Switches[4];

                    if (nodeptr != NULL)
                    {
                        nodepos = (f32 *) ((ModelNode *) nodeptr)->Data;
                        flashpos.x = (((((f32 *) nodepos)[0] * flashmtx.m[0][0]) + (((f32 *) nodepos)[1] * flashmtx.m[1][0])) + (((f32 *) nodepos)[2] * flashmtx.m[2][0])) + flashmtx.m[3][0];
                        flashpos.y = (((((f32 *) nodepos)[0] * flashmtx.m[0][1]) + (((f32 *) nodepos)[1] * flashmtx.m[1][1])) + (((f32 *) nodepos)[2] * flashmtx.m[2][1])) + flashmtx.m[3][1];
                        flashpos.z = (((((f32 *) nodepos)[0] * flashmtx.m[0][2]) + (((f32 *) nodepos)[1] * flashmtx.m[1][2])) + (((f32 *) nodepos)[2] * flashmtx.m[2][2])) + flashmtx.m[3][2];
                        ((f32 *) stackpad2)[-8] = IDO_POINT_ONE * flashscale;
                        matrix_4x4_align(&flash2mtx, (randomGetNext() * (1.0f / M_U32_MAX_VALUE_F)) * M_TAU_F, -flashpos.x, -flashpos.y, -flashpos.z);
                        matrix_scalar_multiply(((f32 *) stackpad2)[-8], flash2mtx.m[0]);
                        matrix_4x4_set_rotation_axis_angle(&aimmtx, 0, gunofs.x - hand->field_A38, gunofs.y - hand->field_A3C, gunofs.z - hand->field_A40);
                        matrix_4x4_multiply_in_place(&aimmtx, &flash2mtx);
                        matrix_row_3_scalar_multiply(flashext, flash2mtx.m[0]);
                        matrix_4x4_multiply_in_place(&rotmtx, &flash2mtx);
                        matrix_4x4_set_position(&flashpos, &flash2mtx);
                        matrix_4x4_copy(&flash2mtx, &rwmtx[3]);
                    }
                }
            }
        }
        else
        {
            hand->field_B58.x = hand->throw_item_pos_related.m[3][0];
            hand->field_B58.y = hand->throw_item_pos_related.m[3][1];
            hand->field_B58.z = hand->throw_item_pos_related.m[3][2];
            hand->field_B64 = -hand->gunmtx_camspace.m[3][2];
        }

        node = mdlhdr->Switches[6];

        if (node != NULL)
        {
            swdata = (f32 *) node->Data;
            sw6mtxidx = modelFindNodeMtxIndex(node, 0);
            sub_GAME_7F05E6B4(handnum, hand->weapon_hold_time);

            if ((mdlhdr->numSwitches >= 0x1D) && (mdlhdr->Switches[28] != NULL))
            {
                hingedata = (f32 *) mdlhdr->Switches[28]->Data;
                guRotateF(tmpmtx.m, (((hand->field_A84 + M_TAU_F) - get_value_if_watch_is_on_hand_or_not(handnum)) * 360.0f) / M_TAU_F, hingedata[0] - hingedata[3], hingedata[1] - hingedata[4], hingedata[2] - hingedata[5]);
                matrix_4x4_set_position((coord3d *) swdata, &tmpmtx);
            }
            else
            {
                matrix_4x4_set_position_and_rotation_around_y(swdata, hand->field_A84, &tmpmtx);
            }

            matrix_4x4_multiply_homogeneous(&gunmtx, &tmpmtx, &rwmtx[sw6mtxidx]);
        }

        if (mdlhdr->numSwitches >= 0x1E)
        {
            bondviewSelectCuff(model, mdlhdr, 0x1D);
        }

        node = mdlhdr->Switches[7];

        if (node != NULL)
        {
            sw7data = (f32 *) node->Data;
            sw7mtxidx = modelFindNodeMtxIndex(node, 0);
            sub_GAME_7F05E83C(handnum);
            matrix_4x4_set_identity_and_position((coord3d *) sw7data, &tmpmtx);
            tmpmtx.m[3][2] -= hand->field_A88;
            matrix_4x4_multiply(&gunmtx, &tmpmtx, &rwmtx[sw7mtxidx]);
        }

        shellidx = 0;

        /**
         * For the shotguns, show or hide the shells in the shell holder based on how much ammunition is in the player's reserve.
         */
        if (mdlhdr->numSwitches >= 0x13)
        {
            do
            {
                if (mdlhdr->Switches[18 + shellidx] != 0)
                {
                    nodeptr = (f32 *) modelGetNodeRwData(model, mdlhdr->Switches[18 + shellidx]);

                    if (nodeptr != NULL)
                    {
                        *((s32 *) nodeptr) = hand->numvisibleshells >= (5 - shellidx);
                    }
                }

                if (mdlhdr->Switches[23 + shellidx] != 0)
                {
                    nodeptr = (f32 *) modelGetNodeRwData(model, mdlhdr->Switches[23 + shellidx]);

                    if (nodeptr != NULL)
                    {
                        *((s32 *) nodeptr) = hand->numvisibleshells >= (5 - shellidx);
                    }
                }

                shellidx += 1;
            }
            while (shellidx != 5);
        }

        modelUpdateNodeRelations(model);

        if (hand->weapon_firing_status != 0)
        {
            switch (item)
            {
            case ITEM_WPPK:
            case ITEM_WPPKSIL:
            case ITEM_TT33:
            case ITEM_SKORPION:
            case ITEM_AK47:
            case ITEM_UZI:
            case ITEM_MP5K:
            case ITEM_MP5KSIL:
            case ITEM_SPECTRE:
            case ITEM_M16:
            case ITEM_FNP90:
            case ITEM_SNIPERRIFLE:
            case ITEM_RUGER:
            case ITEM_GOLDENGUN:
            case ITEM_SILVERWPPK:
            case ITEM_GOLDWPPK:
                gunCreateBeamForHand(handnum);
                hand->field_8A0 = hand->field_8A0 + 1;
                break;
            case ITEM_LASER:
            case ITEM_WATCHLASER:
#if defined(VERSION_JP) || defined(VERSION_EU)
                hand->field_8A0 = hand->field_8A0 + 1;
#endif
                gunCreateBeamForHand(handnum);
                break;
            }
        }
    }

    if (item == ITEM_ROCKETLAUNCH)
    {
        gunUpdateAttachedRocket(handnum);
    }

    if (hand->weapon_firing_status != 0)
    {
        sub_GAME_7F068508(handnum, bondviewGetPlayerStanHeight(g_CurrentPlayer));

        if (item == ITEM_GRENADELAUNCH)
        {
            gunSpawnGLGrenade(handnum);
            return;
        }

        if (item == ITEM_GRENADE)
        {
            generate_player_thrown_grenade(handnum);
            return;
        }

        if (item == ITEM_ROCKETLAUNCH)
        {
            gunFireTankShell(handnum);
            return;
        }

        if (item == ITEM_THROWKNIFE)
        {
            generate_player_thrown_knife(handnum);
            return;
        }

        if ((((((((item == ITEM_REMOTEMINE) || (item == ITEM_PROXIMITYMINE)) || (item == ITEM_TIMEDMINE)) || (item == ITEM_BOMBCASE)) || (item == ITEM_BUG)) || (item == ITEM_MICROCAMERA)) || (item == ITEM_GOLDENEYEKEY)) || (item == ITEM_PLASTIQUE))
        {
            generate_player_thrown_object(handnum);
            return;
        }

        if (item == ITEM_FLAREPISTOL)
        {
            gunSpawnGLGrenade(handnum);
            return;
        }

        if (item == ITEM_PITONGUN)
        {
            gunSpawnGLGrenade(handnum);
        }
    }
}


void gunUpdateAndFireBothHands(void)
{
    gunUpdateAndFire(GUNRIGHT);
    gunUpdateAndFire(GUNLEFT);
}


/**
 * @param arg0:
 * @param item:
 * @param arg2:
 * @param arg3:
 *
 * Address 0x7F061948.
 *
 * This function adjusts the length of the bullet beam that's rendered on screen.
 * This function is used for both player and guard beams.
 *
 * The watch laser has a very short beam, in accordance with its range.
 * The laser also has a shortened one, but it appears this is to avoid graphical glitches.
 * Other weapons have their bullet beam capped at 10000 max length, otherwise if the player
 * fires into the void, there may be graphical glitches with the beam.
 *
*/
void CapBeamLengthAndDecideIfRendered(struct BeamRecord *arg0, ITEM_IDS item, coord3d *arg2, coord3d *arg3)
{
    f32 phi_f12_2;

    //arg0->pos.x = arg2->x;
    //arg0->pos.y = arg2->y;
    //arg0->pos.z = arg2->z;

    //arg0->delta.x = arg3->x - arg2->x;
    //arg0->delta.y = arg3->y - arg2->y;
    //arg0->delta.z = arg3->z - arg2->z;

    //phi_f12_2 = sqrtf((arg0->delta.f[0] * arg0->delta.f[0]) + (arg0->delta.f[1] * arg0->delta.f[1]) + (arg0->delta.f[2] * arg0->delta.f[2]));

    //arg0->delta.x *= 1.0f / phi_f12_2;
    //arg0->delta.y *= 1.0f / phi_f12_2;
    //arg0->delta.z *= 1.0f / phi_f12_2;


    arg0->pos.f[0] = arg2->f[0];
    arg0->pos.f[1] = arg2->f[1];
    arg0->pos.f[2] = arg2->f[2];

    arg0->delta.f[0] = arg3->x - arg2->x;
    arg0->delta.f[1] = arg3->f[1] - arg2->f[1];
    arg0->delta.f[2] = arg3->f[2] - arg2->f[2];

    phi_f12_2 = sqrtf((arg0->delta.f[0] * arg0->delta.f[0]) + (arg0->delta.f[1] * arg0->delta.f[1]) + (arg0->delta.f[2] * arg0->delta.f[2]));

    arg0->delta.f[0] *= 1.0f / phi_f12_2;
    arg0->delta.f[1] *= 1.0f / phi_f12_2;
    arg0->delta.f[2] *= 1.0f / phi_f12_2;

    if (item == ITEM_WATCHLASER)
    {
        if (phi_f12_2 > 300.0f)
        {
            phi_f12_2 = 300.0f;
        }
    }
    else
    {
        if (phi_f12_2 > 10000.0f)
        {
            phi_f12_2 = 10000.0f;
        }
    }

    arg0->unk00 = 0;
    arg0->item_id = (s8) item;
    arg0->unk1c = phi_f12_2;

    if (phi_f12_2 < 500.0f)
    {
        phi_f12_2 = 500.0f;
    }

    if (item == ITEM_LASER)
    {
        arg0->unk20 = 0.25f * phi_f12_2;
        arg0->unk24 = 0.6f * phi_f12_2;

        if (arg0->unk24 > 3000.0f)
        {
            arg0->unk24 = 3000.0f;
        }

        // Laser beams are rendered more often than other normal weapons
        arg0->unk28 = (-0.1f - ((f32) (u32)randomGetNext() * (1.0f / UINT_MAX) * 0.3f)) * phi_f12_2;
    }
    else if (item == ITEM_WATCHLASER)
    {
        arg0->unk24 = phi_f12_2;
        arg0->unk20 = 2.0f * phi_f12_2;

        if (phi_f12_2 > 3000.0f)
        {
            arg0->unk24 = 3000.0f;
        }

        // Always render the beam for the watch laser
        arg0->unk28 = 0.0f;
    }
    else
    {
        arg0->unk20 = 0.2f * phi_f12_2;
        arg0->unk24 = arg0->unk20;

        if (arg0->unk20 > 3000.0f)
        {
            arg0->unk24 = 3000.0f;
        }

        // Decide if a beam should be rendered for normal weapon bullets
        arg0->unk28 = ((2.0f * ((f32) (u32)randomGetNext() * (1.0f / UINT_MAX))) - 1.0f) * arg0->unk20;
    }

    if (arg0->unk1c <= arg0->unk28)
    {
        // No beam will be rendered
        arg0->unk00 = -1;
    }
}


void gunCreateBeamForHand(enum GUNHAND hand)
{
    coord3d *field_2A18;
    Mtxf *player_matrix;
    struct hand *hand_ptr;
    f32 val;
    struct ChrRecord *chr;
    f32 diff1_z;
    f32 diff1_y;
    f32 diff1_x;
    f32 diff2_z;
    f32 diff2_y;
    f32 diff2_x;
    BeamRecord *weapon_beam;

    hand_ptr = &g_CurrentPlayer->hands[hand];
    player_matrix = camGetWorldToScreenMtxf();

    val = -((((hand_ptr->item_related.x * player_matrix->m[0][2]) + (hand_ptr->item_related.y * player_matrix->m[1][2])) + (hand_ptr->item_related.z * player_matrix->m[2][2])) + player_matrix->m[3][2]);

    if (val < hand_ptr->field_B64)
    { 
        return; 
    }

    weapon_beam = &hand_ptr->weapon_beam;

    CapBeamLengthAndDecideIfRendered(weapon_beam, getCurrentPlayerWeaponId(hand), &hand_ptr->field_B58, &hand_ptr->item_related);

    if ((g_CurrentPlayer->prop->chr == NULL) || (getPlayerCount() < 2)) 
    { 
        return; 
    }

    chr = g_CurrentPlayer->prop->chr;

    diff1_x = hand_ptr->item_related.x - g_CurrentPlayer->field_2A18[hand].x;
    diff1_y = hand_ptr->item_related.y - g_CurrentPlayer->field_2A18[hand].y;
    diff1_z = hand_ptr->item_related.z - g_CurrentPlayer->field_2A18[hand].z;
    guNormalize(&diff1_x, &diff1_y, &diff1_z);

    diff2_x = hand_ptr->item_related.x - hand_ptr->field_B58.x;
    diff2_y = hand_ptr->item_related.y - hand_ptr->field_B58.y;
    diff2_z = hand_ptr->item_related.z - hand_ptr->field_B58.z;
    guNormalize(&diff2_x, &diff2_y, &diff2_z);

    val = acosf(
        + (diff2_z * diff1_z)
        + ((diff1_x * diff2_x)
        + (diff1_y * diff2_y)));

    if (val > 0.08726647f) 
    { 
        return; 
    }

    CapBeamLengthAndDecideIfRendered(&chr->beams[hand], getCurrentPlayerWeaponId(hand), &g_CurrentPlayer->field_2A18[hand], &hand_ptr->item_related);
}


Gfx *sub_GAME_7F061E18(Gfx *gdl, BeamRecord *flash, s32 arg2)
{
    f32 posz;
    Mtx *mtx;
    Mtxf mtxf;
    Vtx templatevtx;
    coord3d pos;
    coord3d *playerpos;
    f32 radius;
    f32 pad_after_radius[2];
    f32 startoffset;
    coord3d up;
    coord3d right;
    coord3d flareoffset;
    coord3d extraorigin;
    f32 extra_scale;
    struct sImageTableEntry *image;
    f32 d;
    Mtxf *worldtoscreen;
    coord3d tmp;
    coord3d divout;
    f32 divin[2];
    f32 f14;
    f32 f16;
    f32 f18;
    f32 dist_to_start;
    Vtx *vtx;
    f32 dist;
    Gfx *cmd;

    if (flash->unk00 >= 0)
    {
        templatevtx = D_80035C98;
        playerpos = bondviewGetCurrentPlayersPosition();
        startoffset = flash->unk28;
        dist = flash->unk24;
        flareoffset = D_80035CA8;
        extraorigin = D_80035CB4;
        extra_scale = 1.4142f; // ~√2
        image = flareimage3;
        worldtoscreen = camGetWorldToScreenMtxf();

        if (flash->item_id == ITEM_LASER)
        {
            radius = 50.0f;
            image = flareimage4;
        }
        else if (flash->item_id == ITEM_WATCHLASER)
        {
            radius = 10.0f;
            image = flareimage4;
            templatevtx.v.cn[3] = (randomGetNext() % 50) + 150;

            if ((randomGetNext() % 5) == 0)
            {
                templatevtx.v.cn[0] = (templatevtx.v.cn[1] = 255 - (randomGetNext() % 100));
            }
        }
        else
        {
            radius = 30.0f;
        }

        pos.f[0] = flash->pos.f[0];
        pos.f[1] = flash->pos.f[1];
        pos.f[2] = flash->pos.f[2];

        if (startoffset > 0.0f)
        {
            pos.f[0] += startoffset * flash->delta.f[0];
            pos.f[1] += startoffset * flash->delta.f[1];
            pos.f[2] += startoffset * flash->delta.f[2];
        }
        else
        {
            dist += startoffset;
            startoffset = 0.0f;
        }

        if (flash->unk1c < (startoffset + dist))
        {
            dist = flash->unk1c - startoffset;
        }

        right.f[0] = (flash->delta.f[1] * (playerpos->f[2] - (pos.f[2] + (dist * flash->delta.f[2])))) - (flash->delta.f[2] * (playerpos->f[1] - (pos.f[1] + (dist * flash->delta.f[1]))));
        right.f[1] = (flash->delta.f[2] * (playerpos->f[0] - (pos.f[0] + (dist * flash->delta.f[0])))) - (flash->delta.f[0] * (playerpos->f[2] - (pos.f[2] + (dist * flash->delta.f[2]))));
        right.f[2] = (flash->delta.f[0] * (playerpos->f[1] - (pos.f[1] + (dist * flash->delta.f[1])))) - (flash->delta.f[1] * (playerpos->f[0] - (pos.f[0] + (dist * flash->delta.f[0]))));

        if (((right.f[0] != 0.0f) || (right.f[1] != 0.0f)) || (right.f[2] != 0.0f))
        {
            guNormalize(&right.f[0], &right.f[1], &right.f[2]);
            right.f[0] *= radius;
            right.f[1] *= radius;
            right.f[2] *= radius;
        }
        else
        {
            right.f[0] = 0.0f;
            right.f[1] = radius;
            right.f[2] = 0.0f;
        }

        up.f[0] = (flash->delta.f[1] * right.f[2]) - (flash->delta.f[2] * right.f[1]);
        up.f[1] = (flash->delta.f[2] * right.f[0]) - (flash->delta.f[0] * right.f[2]);
        up.f[2] = (flash->delta.f[0] * right.f[1]) - (flash->delta.f[1] * right.f[0]);

        guNormalize(&up.f[0], &up.f[1], &up.f[2]);

        up.f[0] *= radius;
        up.f[1] *= radius;
        up.f[2] *= radius;

        if (flash->item_id == ITEM_LASER)
        {
            vtx = dynAllocateVertices(8);
        }
        else
        {
            vtx = dynAllocateVertices(4);
        }

        mtx = dynAllocateMatrix();

        matrix_4x4_set_identity_and_position(&pos, &mtxf);
        matrix_scalar_multiply(0.1f, (f32 *)&mtxf);
        matrix_4x4_multiply_homogeneous_in_place(worldtoscreen, &mtxf);
        matrix_4x4_f32_to_s32(&mtxf, (Mtxf *)mtx);

        vtx[0] = templatevtx;
        vtx[1] = templatevtx;
        vtx[2] = templatevtx;
        vtx[3] = templatevtx;

        if (flash->item_id == ITEM_LASER)
        {
            vtx[4] = templatevtx;
            vtx[5] = templatevtx;
            vtx[6] = templatevtx;
            vtx[7] = templatevtx;
        }

        if (flash->item_id == ITEM_WATCHLASER)
        {
            tmp.f[0] = pos.f[0] + flash->delta.f[0] * dist;
            tmp.f[1] = pos.f[1] + flash->delta.f[1] * dist;
            tmp.f[2] = pos.f[2] + flash->delta.f[2] * dist;

            mtx4TransformVecInPlace(worldtoscreen, &tmp);

            divin[0] = divin[1] = radius / 10.0f;

            dist_to_start = -tmp.f[2];
            divide3DCoordinates((coord3d *)divin, dist_to_start, &divout);

            if (divout.f[0] < 2.0f)
            {
                tmp.f[0] *= divout.f[0] * 0.5f;
                tmp.f[1] *= divout.f[0] * 0.5f;
                tmp.f[2] *= divout.f[0] * 0.5f;
            }

            mtx4TransformVecInPlace(currentPlayerGetViewToWorldMtxf(), &tmp);

            tmp.f[0] -= pos.f[0];
            tmp.f[1] -= pos.f[1];
            tmp.f[2] -= pos.f[2];

            flareoffset.x = tmp.f[0] * 10.0f;
            flareoffset.y = tmp.f[1] * 10.0f;
            flareoffset.z = tmp.f[2] * 10.0f;
        }
        else
        {
            flareoffset.x = flash->delta.f[0] * (dist * 10.0f);
            flareoffset.y = flash->delta.f[1] * (dist * 10.0f);
            flareoffset.z = flash->delta.f[2] * (dist * 10.0f);
        }

        vtx[0].v.ob[0] = right.f[0];
        vtx[0].v.ob[1] = right.f[1];
        vtx[0].v.ob[2] = right.f[2];
        vtx[0].v.tc[0] = image->width << 5;
        vtx[0].v.tc[1] = 0;

        vtx[1].v.ob[0] = -right.f[0];
        vtx[1].v.ob[1] = -right.f[1];
        vtx[1].v.ob[2] = -right.f[2];
        vtx[1].v.tc[0] = 0;
        vtx[1].v.tc[1] = 0;

        vtx[2].v.ob[0] = flareoffset.f[0] + (right.f[0] * 0.9f);
        vtx[2].v.ob[1] = flareoffset.f[1] + (right.f[1] * 0.9f);
        vtx[2].v.ob[2] = flareoffset.f[2] + (right.f[2] * 0.9f);
        vtx[2].v.tc[0] = image->width << 5;
        vtx[2].v.tc[1] = image->height << 5;

        vtx[3].v.ob[0] = flareoffset.f[0] - (right.f[0] * 0.9f);
        vtx[3].v.ob[1] = flareoffset.f[1] - (right.f[1] * 0.9f);
        vtx[3].v.ob[2] = flareoffset.f[2] - (right.f[2] * 0.9f);
        vtx[3].v.tc[0] = 0;
        vtx[3].v.tc[1] = image->height << 5;

        if (flash->item_id == ITEM_LASER)
        {
            f14 = playerpos->f[0] - pos.f[0];
            f16 = playerpos->f[1] - pos.f[1];
            f18 = playerpos->f[2] - pos.f[2];

            dist_to_start = (f14 * f14) + (f16 * f16) + (f18 * f18);

            f14 = playerpos->f[0] - (pos.f[0] + (flash->delta.f[0] * dist));
            f16 = playerpos->f[1] - (pos.f[1] + (flash->delta.f[1] * dist));
            f18 = playerpos->f[2] - (pos.f[2] + (flash->delta.f[2] * dist));

            d = (f14 * f14) + (f16 * f16) + (f18 * f18);

            if (d < dist_to_start)
            {
                extraorigin.f[0] = flareoffset.f[0];
                extraorigin.f[1] = flareoffset.f[1];
                extraorigin.f[2] = flareoffset.f[2];
                extra_scale *= 0.9f;
            }

            vtx[4].v.ob[0] = extraorigin.f[0] + (up.f[0] * extra_scale);
            vtx[4].v.ob[1] = extraorigin.f[1] + (up.f[1] * extra_scale);
            vtx[4].v.ob[2] = extraorigin.f[2] + (up.f[2] * extra_scale);
            vtx[4].v.tc[0] = flareimage5->width << 5;
            vtx[4].v.tc[1] = flareimage5->height << 5;

            vtx[5].v.ob[0] = extraorigin.f[0] - (up.f[0] * extra_scale);
            vtx[5].v.ob[1] = extraorigin.f[1] - (up.f[1] * extra_scale);
            vtx[5].v.ob[2] = extraorigin.f[2] - (up.f[2] * extra_scale);
            vtx[5].v.tc[0] = 0;
            vtx[5].v.tc[1] = 0;

            vtx[6].v.ob[0] = extraorigin.f[0] + (right.f[0] * extra_scale);
            vtx[6].v.ob[1] = extraorigin.f[1] + (right.f[1] * extra_scale);
            vtx[6].v.ob[2] = extraorigin.f[2] + (right.f[2] * extra_scale);
            vtx[6].v.tc[0] = 0;
            vtx[6].v.tc[1] = flareimage5->height << 5;

            vtx[7].v.ob[0] = extraorigin.f[0] - (right.f[0] * extra_scale);
            vtx[7].v.ob[1] = extraorigin.f[1] - (right.f[1] * extra_scale);
            vtx[7].v.ob[2] = extraorigin.f[2] - (right.f[2] * extra_scale);
            vtx[7].v.tc[0] = flareimage5->width << 5;
            vtx[7].v.tc[1] = 0;
        }

        gSPClearGeometryMode(gdl++, G_CULL_BACK);

        // Ideally this should be something like gSPMatrix(gdl++ or cmd++, osVirtualToPhysical(mtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH) but I can't get these or similar forms to match
        {
            cmd = gdl++;
            cmd->words.w0 = _SHIFTL(G_MTX, 24, 8)
                | _SHIFTL(G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH, 16, 8)
                | _SHIFTL(sizeof(Mtx), 0, 16);
            cmd->words.w1 = osVirtualToPhysical(mtx);
        }

        if (flash->item_id == ITEM_LASER)
        {
            texSelect(&gdl, flareimage5, 4, arg2, 2);

            gSPVertex(gdl++, osVirtualToPhysical(vtx), 8, 0);

            gSP4Triangles(gdl++, 4, 5, 6, 4, 5, 7, 0, 0, 0, 0, 0, 0);

            texSelect(&gdl, image, 4, arg2, 2);

            gSP4Triangles(gdl++, 0, 2, 3, 0, 3, 1, 0, 0, 0, 0, 0, 0);
        }
        else
        {
            texSelect(&gdl, image, 4, arg2, 2);

            gSPVertex(gdl++, osVirtualToPhysical(vtx), 4, 0);

            gSP4Triangles(gdl++, 0, 2, 3, 0, 3, 1, 0, 0, 0, 0, 0, 0);
        }
    }

    return gdl;
}


/*
* Address: 0x7F062B00
*/
void gunAdvanceBeamTimer(BeamRecord* beam)
{
    if (beam->unk00 >= 0)
    {
        if (g_ClockTimer < 3)
        {
#ifdef VERSION_US
            beam->unk28 += beam->unk20 * g_GlobalTimerDelta;
#else
            beam->unk28 += beam->unk20 * g_JP_GlobalTimerDelta;
#endif
        }
        else
        {
            beam->unk28 += beam->unk20 * (2.0f + ((f32) randomGetNext() * 2.3283064e-10f * 0.5f));
        }

        if (beam->unk1c <= beam->unk28)
        {
            beam->unk00 = -1;
            return;
        }

        beam->unk00++;
    }
}

// Address: 0x7F062BE4
void gunRenderFirstPersonGunModels(Gfx **gdlptr)
{
    Gfx *gdl = *gdlptr;
    ModelRenderData renderdata;
    s32 handnum;
    Model *model;
 
    renderdata = *(ModelRenderData *)&D_80035CC0;
 
    for (handnum = 0; handnum != 2; handnum++) 
    {
        struct hand *handptr = &g_CurrentPlayer->hands[handnum];
        s32 item = get_item_in_hand_or_watch_menu(handnum);
 
        if (handptr->field_87F == 0) 
        {
            continue;
        }
 
        if (item != ITEM_WATCHLASER) 
        {
            gdl = sub_GAME_7F061E18(gdl, &handptr->weapon_beam, 0);
        }
 
        if (item == ITEM_GOLDENGUN || item == ITEM_RUGER || item == ITEM_KNIFE || item == ITEM_THROWKNIFE || item == ITEM_SILVERWPPK || item == ITEM_GOLDWPPK) 
        {
            gSPSetLights1(gdl++, g_WeaponEnvmapLight);
            gSPLookAt(gdl++, sub_GAME_7F078474());
        }
 
        gSPPerspNormalize(gdl++, matrix_4x4_calc_depth_scale(0.0f, 300.0f));
 
        if ((*(Model *)&handptr->field_B68).obj->numSwitches >= 0x11 && (*(Model *)&handptr->field_B68).obj->Switches[16] != NULL)
        {
            union ModelRwData *rwdata;
            model = (Model *)&handptr->field_B68;
            rwdata = modelGetNodeRwData(model, (*(Model *)&handptr->field_B68).obj->Switches[17]);
 
            if (rwdata != NULL) 
            {
                rwdata->Raw.unk00 = 1;
            }
 
            if (item == ITEM_ROCKETLAUNCH) 
            {
                save_img_index_to_obj_ani_slot(&g_UnknownAnimController, crosshairimage);
                gdl = process_monitor_animation_microcode(model, (*(Model *)&handptr->field_B68).obj->Switches[16], &g_UnknownAnimController, gdl, 0, 4);
            } 
            else 
            {
                gdl = process_monitor_animation_microcode(model, (*(Model *)&handptr->field_B68).obj->Switches[16], &g_TaserAnimController, gdl, 0, 1);
            }
        }
 
        renderdata.gdl = gdl;
        renderdata.PropType = 4;
        renderdata.envcolour.word = g_CurrentPlayer->tileColor.a
                                  | ((u32)g_CurrentPlayer->tileColor.r << 24)
                                  | ((u32)g_CurrentPlayer->tileColor.g << 16)
                                  | ((u32)g_CurrentPlayer->tileColor.b << 8);
        renderdata.zbufferenabled = 0;
 
        matrix_4x4_7F058C64();
 
        if (item == ITEM_ROCKETLAUNCH && handptr->rocket != NULL) 
        {
            model = handptr->rocket->model;
 
            subdraw(&renderdata, model);
            bondviewTransformManyPosToViewMatrix(model->render_pos, model->obj->numMatrices);
 
            if (handptr->firedrocket != 0) 
            {
                handptr->rocket = NULL;
            }
        }
 
        if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_MIRROR_DUAL) != 0) 
        {
            gSPClearGeometryMode(renderdata.gdl++, G_CULL_BOTH);
 
            if (handnum == 0) 
            {
                renderdata.cullmode = 3;
            } 
            else 
            {
                renderdata.cullmode = 2;
            }
        }
 
        subdraw(&renderdata, (Model *)&handptr->field_B68);
        gdl = renderdata.gdl;
 
        if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_MIRROR_DUAL) != 0) 
        {
            gSPClearGeometryMode(gdl++, G_CULL_BOTH);
        }
 
        bondviewTransformManyPosToViewMatrix((*(Model *)&handptr->field_B68).render_pos, (*(Model *)&handptr->field_B68).obj->numMatrices);
        matrix_4x4_7F058C88();
 
        gSPPerspNormalize(gdl++, viGetPerspNorm());
 
        if (item == ITEM_WATCHLASER) 
        {
            gdl = sub_GAME_7F061E18(gdl, &handptr->weapon_beam, 0);
        }
    }
 
    *gdlptr = gdl;
}


Gfx *set_enviro_fog_for_items_in_solo_watch_menu(Gfx *gdl, ITEM_IDS itemid, Mtxf *mtx, s32 arg3, s32 arg4)
{
    ModelRenderData renderdata = *((ModelRenderData *) (&D_80035D00));
    ModelHeader model;
    u8 spb8[0x80];
    s32 padb4;
    Mtxf sp74;
    Mtxf *matrices;
    union ModelRwData *rwdata;
    s32 i;
    s32 j;
    ModelFileHeader *bodymodel;

    if ((itemid == ITEM_TRIGGER) || (itemid == ITEM_WATCHLASER))
    {
        itemid = ITEM_WATCHMAGNETATTRACT;
    }

    sub_GAME_7F05DA8C(GUNRIGHT, itemid);

    if ((!Gun_hand_without_item(GUNRIGHT)) || (!get_itemtype_in_hand(GUNRIGHT)))
    {
        return gdl;
    }

    bodymodel = &g_CurrentPlayer->copy_of_body_obj_header[GUNRIGHT];

    if (!get_ptr_weapon_model_header_line(itemid))
    {
        goto earlyreturn;
    }

    if (bondwalkItemCheckBitflags(itemid, WEAPONSTATBITFLAG_HIDE_FIRST_PERSON_MENU))
    {
        goto earlyreturn;
    }

    matrices = dynAllocate(bodymodel->numMatrices << 6);
    
    for (i = 0; i < bodymodel->numMatrices; i++)
    {
        matrix_4x4_set_identity(&matrices[i]);
    }

    i = 0;
    ((Model *) &model)->render_pos = matrices;
    modelCalculateRwDataLen(bodymodel);
    modelInit((Model *) &model, bodymodel, spb8);
    sub_GAME_7F05E978((Model *) &model, 0);
    sub_GAME_7F05EA94((Model *) &model, 1);

    if (bodymodel->Switches[1] != NULL)
    {
        rwdata = modelGetNodeRwData((Model *) &model, bodymodel->Switches[1]);

        if (rwdata != 0)
        {
            rwdata->Raw.unk00 = 0;
        }
    }

    matrix_4x4_copy(mtx, matrices);
  
    if (bodymodel->Skeleton == (&skeleton_gun_revolver))
    {
        if (bodymodel->Switches[4] != NULL)
        {
            matrix_4x4_set_identity_and_position((coord3d *) bodymodel->Switches[4]->Data, &sp74);
            matrix_4x4_multiply(mtx, &sp74, &matrices[3]);
        }
        if (bodymodel->Switches[5] != NULL)
        {
            matrix_4x4_set_identity_and_position((coord3d *) bodymodel->Switches[5]->Data, &sp74);
            matrix_4x4_multiply(mtx, &sp74, &matrices[4]);
        }
    }

    if (bodymodel->Switches[6] != NULL)
    {
        coord3d *pos;
        s32 index;

        pos = (coord3d *) bodymodel->Switches[6]->Data;
        index = modelFindNodeMtxIndex(bodymodel->Switches[6], 0);
        matrix_4x4_set_identity_and_position(pos, &sp74);
        matrix_4x4_multiply(mtx, &sp74, &matrices[index]);
    }

    if (bodymodel->Switches[7] != NULL)
    {
        coord3d *pos;
        s32 index;
        u8 pad[8];

        pos = (coord3d *) bodymodel->Switches[7]->Data;
        index = modelFindNodeMtxIndex(bodymodel->Switches[7], 0);
        matrix_4x4_set_identity_and_position(pos, &sp74);
        matrix_4x4_multiply(mtx, &sp74, &matrices[index]);
    }

    if (bodymodel->numSwitches >= 19)
    {
        for (j = 0; j != 20; j += 4)
        {
            if ((*((ModelNode **) ((((u8 *) bodymodel->Switches) + j) + 0x48))) != NULL)
            {
                rwdata = modelGetNodeRwData((Model *) &model, *((ModelNode **) ((((u8 *) bodymodel->Switches) + j) + 0x48)));

                if (rwdata != NULL)
                {
                    rwdata->Raw.unk00 = 1;
                }
            }

            if ((*((ModelNode **) ((((u8 *) bodymodel->Switches) + j) + 0x5c))) != NULL)
            {
                rwdata = modelGetNodeRwData((Model *) &model, *((ModelNode **) ((((u8 *) bodymodel->Switches) + j) + 0x5c)));

                if (rwdata != NULL)
                {
                    rwdata->Raw.unk00 = 1;
                }
            }
        }
    }

    modelUpdateNodeRelations((Model *) &model);

    if ((((((itemid == ITEM_GOLDENGUN) || (itemid == ITEM_RUGER)) || (itemid == ITEM_KNIFE)) || (itemid == ITEM_THROWKNIFE)) || (itemid == ITEM_SILVERWPPK)) || (itemid == ITEM_GOLDWPPK))
    {
        gSPSetLights1(gdl++, g_WeaponEnvmapLight);
        gSPLookAt(gdl++, sub_GAME_7F078474());
    }

    if (bodymodel->numSwitches >= 17)
    {
        if (bodymodel->Switches[16] != NULL)
        {
            rwdata = modelGetNodeRwData((Model *) &model, bodymodel->Switches[17]);

            if (rwdata != 0)
            {
                rwdata->Raw.unk00 = 0;
            }
        }
    }

    renderdata.gdl = gdl;

    if (arg3 >= 0xff)
    {
        renderdata.PropType = PROP_TYPE_WEAPON;
        renderdata.envcolour.word = arg4;
    }
    else
    {
        renderdata.PropType = PROP_TYPE_PLAYER;
        renderdata.envcolour.word = arg3;
        renderdata.fogcolour.word = arg4;
    }

    renderdata.zbufferenabled = FALSE;
    subdraw(&renderdata, (Model *) &model);
    gdl = renderdata.gdl;
    matrix_4x4_7F058C64();
    j = 0;

    if (bodymodel->numMatrices > 0)
    {
        do
        {
            matrix_4x4_copy((Mtxf *) (((u8 *) ((Model *) &model)->render_pos) + j), &sp74);
            matrix_4x4_f32_to_s32(&sp74, (Mtxf *) ((i << 6) + (u8 *) ((Model *) &model)->render_pos));
            i++;
            j += 0x40;
        }
        while (i < bodymodel->numMatrices);
    }

    matrix_4x4_7F058C88();

earlyreturn:
    return gdl;

}


void sub_GAME_7F0634D8(s32 arg0, s32 arg1, s32 arg2, s32 arg3)
{
    set_enviro_fog_for_items_in_solo_watch_menu(arg0, arg1, arg2, arg3, -256);
}


// Unreferenced
void sub_GAME_7F0634FC(s32 arg0, s32 arg1, s32 arg2)
{
    sub_GAME_7F0634D8(arg0, arg1, arg2, 0xFF);
}


void sub_GAME_7F06351C(struct coord3d* arg0, Mtxf* arg1, Mtxf* arg2, Mtxf* arg3, struct coord3d* arg4, Mtxf* arg5, Mtxf* arg6)
{
    Mtxf sp20;

    matrix_4x4_set_identity_and_position(arg0, arg6);
    matrix_4x4_multiply_in_place(arg1, arg6);
    matrix_4x4_multiply_in_place(arg2, arg6);
    matrix_4x4_multiply_in_place(arg3, arg6);
    matrix_4x4_set_identity_and_position(arg4, &sp20);
    matrix_4x4_multiply_in_place(&sp20, arg6);
    matrix_4x4_multiply_in_place(arg5, arg6);
}


/**
 * Address: 7F06359C
 */
Gfx* watchRenderController(Gfx* gdl, Mtxf* basemtx, s32 envcolour, bool animatebuttons, WatchContButtonPositions* buttonpositions, s8* contpadnum)
{
    ModelRenderData renderdata;
    struct ModelHeader modelstack;
    s32 i;
    s32 j;
    s32 offset;
    f32 angle;
    u32 rwdata[26];
    u32 pad2;
    Mtxf sp41c;
    Mtxf sp3dc;
    struct ModelFileHeader* objheader;
    struct coord3d* position;
    Mtxf* matrices;
    ModelNode* node;
    Mtxf sp38c;
    Mtxf sp34c;
    Mtxf sp30c;
    Mtxf sp2cc;
    Mtxf sp28c;
    Mtxf sp24c;
    Mtxf sp20c;
    Mtxf sp1cc;
    struct coord3d coord_node2;
    struct coord3d coord_node11_pos;
    struct coord3d coord_node11_base;
    struct coord3d coord_node4_pos;
    struct coord3d coord_node4_base;
    struct coord3d coord_node5_pos;
    struct coord3d coord_node5_base;
    struct coord3d coord_node6_pos;
    struct coord3d coord_node6_base;
    struct coord3d coord_node7_pos;
    struct coord3d coord_node7_base;
    struct coord3d coord_node9_pos;
    struct coord3d coord_node9_base;
    struct coord3d coord_node8_pos;
    struct coord3d coord_node8_base;
    struct coord3d coord_node10_pos;
    struct coord3d coord_node10_base;
    struct coord3d coord_node3_pos;
    Mtxf spb4;
    struct coord3d coord_node3_base;
    struct coord3d coord_node1_pos;
    struct coord3d coord_node1_base;
    struct coord3d coord_node12_pos;
    struct coord3d coord_node12_base;

    renderdata = *(ModelRenderData *)((u8 *)D_80035D04 + 0x3c);

    sub_GAME_7F05DA8C(GUNRIGHT, 0x55);

    if (!(Gun_hand_without_item(GUNRIGHT) && get_itemtype_in_hand(GUNRIGHT)))
    {
        return gdl;
    }

    objheader = g_CurrentPlayer->copy_of_body_obj_header;
    matrices = dynAllocate(objheader->numMatrices * (sizeof(Mtxf)));
    modelCalculateRwDataLen(objheader);

    if (objheader);

    modelInit(&modelstack, objheader, rwdata);
    modelstack.render_pos = (RenderPosView*) matrices;
    matrix_4x4_copy(basemtx, &matrices[0]);
    
    for (i = 1; i < 13; i++)
    {
        position = objheader->Switches[i]->Data;

        // Update joy stick position and rotation.
        if (i == 2)
        {
            angle = ((((-(f32) joyGetStickX(*contpadnum)) * M_TAU_F) * 0.6f)) / 360.0f;
            matrix_4x4_set_rotation_around_z(angle, &sp41c);
            angle = ((((-(f32) joyGetStickY(*contpadnum)) * M_TAU_F) * 0.6f)) / 360.0f;
            matrix_4x4_set_rotation_around_x(angle, &sp3dc);
            matrix_4x4_multiply_in_place(&sp3dc, &sp41c);
            matrix_4x4_set_position(position, &sp41c);
        }
        else
        {
            matrix_4x4_set_identity_and_position(position, &sp41c);
        }

        matrix_4x4_multiply(basemtx, &sp41c, &matrices[i]);
    }

    modelUpdateNodeRelations(&modelstack);
    renderdata.gdl = gdl;

    if (envcolour >= 0xff)
    {
        renderdata.PropType = PROP_TYPE_OBJ;
    }
    else
    {
        renderdata.PropType = PROP_TYPE_PLAYER;
        renderdata.envcolour.word = envcolour;
        renderdata.fogcolour.word = 0xFFFFFF00;
    }

    renderdata.zbufferenabled = TRUE;
    subdraw(&renderdata, &modelstack);
    gdl = renderdata.gdl;
    matrix_4x4_7F058C64();

    for (i = 0; i < objheader->numMatrices; i++)
    {
        matrix_4x4_copy((u32)modelstack.render_pos + i * sizeof(Mtxf), &sp41c);
        matrix_4x4_f32_to_s32(&sp41c, &modelstack.render_pos[i]);
    }

    matrix_4x4_7F058C88();

    if (animatebuttons)
    {
        node = objheader->Switches[13];

        if (node)
        {
            *((s32*) modelGetNodeRwData(&modelstack, node)) = 0;
        }

        matrices = dynAllocate(objheader->numMatrices * (sizeof(Mtxf)));
        modelstack.render_pos = (RenderPosView*) matrices;

        // Update the positions and/or rotations of the free floating buttons on the sides of the screen.
        for (i = 0; i < 13; i++)
        {
            matrix_4x4_set_lookat_target(&sp20c, -5.0f, 2000.0f, -168.0f, -5.0f, 0.0f, -168.0f, 0.0f, 0.0f, -1.0f);
            matrix_4x4_set_lookat_target(&sp38c, -5.0f, 2000.0f, -168.0f, -5.0f, 0.0f, -168.0f, 0.0f, 0.0f, -1.0f);
            matrix_4x4_set_identity(&sp24c);
            matrix_4x4_set_identity(&sp1cc);
            matrix_4x4_copy(&sp1cc, &sp2cc);
            
            // Joy stick
            if (i == 2)
            {
                coord_node2 = buttonpositions->joystick;
                matrix_4x4_set_identity_and_position(&coord_node2, &sp34c);
                angle = ((((-(f32) joyGetStickX(*contpadnum)) * M_TAU_F) * 0.6f)) / 360.0f;
                matrix_4x4_set_rotation_around_z(angle, &sp41c);
                angle = ((((-(f32) joyGetStickY(*contpadnum)) * M_TAU_F) * 0.6f)) / 360.0f;
                matrix_4x4_set_rotation_around_x(angle, &sp3dc);
                matrix_4x4_multiply_in_place(&sp3dc, &sp41c);
                matrix_4x4_multiply_in_place(&sp34c, &sp41c);
                matrix_4x4_multiply(&sp20c, &sp41c, &sp30c);
                matrix_4x4_copy(&sp30c, &matrices[i]);
            }
            // R
            else if (i == 11)
            {
                coord_node11_base = ((struct coord3d *)watchControllerButtonBases)[5];
                coord_node11_pos = buttonpositions->r;

                if (joyGetButtons(*contpadnum, R_TRIG))
                {
                    matrix_4x4_set_rotation_around_y(-0.17453294f, &sp24c);
                }

                matrix_4x4_set_rotation_around_x(1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node11_base, &sp24c, &sp28c, &sp1cc, &coord_node11_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // C-Up
            else if (i == 4)
            {
                coord_node4_base = ((struct coord3d *)watchControllerButtonBases)[6];
                coord_node4_pos = buttonpositions->cUp;

                if (joyGetButtons(*contpadnum, U_CBUTTONS))
                {
                    coord_node4_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node4_base, &sp24c, &sp28c, &sp1cc, &coord_node4_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // C-Down
            else if (i == 5)
            {
                coord_node5_base = ((struct coord3d *)watchControllerButtonBases)[7];
                coord_node5_pos = buttonpositions->cDown;

                if (joyGetButtons(*contpadnum, D_CBUTTONS))
                {
                    coord_node5_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node5_base, &sp24c, &sp28c, &sp1cc, &coord_node5_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // C-Left
            else if (i == 6)
            {
                coord_node6_base = ((struct coord3d *)watchControllerButtonBases)[8];
                coord_node6_pos = buttonpositions->cLeft;

                if (joyGetButtons(*contpadnum, L_CBUTTONS))
                {
                    coord_node6_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node6_base, &sp24c, &sp28c, &sp1cc, &coord_node6_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // C-Right
            else if (i == 7)
            {
                coord_node7_base = ((struct coord3d *)watchControllerButtonBases)[9];
                coord_node7_pos = buttonpositions->cRight;

                if (joyGetButtons(*contpadnum, R_CBUTTONS))
                {
                    coord_node7_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node7_base, &sp24c, &sp28c, &sp1cc, &coord_node7_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // B
            else if (i == 9)
            {
                coord_node9_base = ((struct coord3d *)watchControllerButtonBases)[10];
                coord_node9_pos = buttonpositions->b;

                if (joyGetButtons(*contpadnum, B_BUTTON))
                {
                    coord_node9_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976, &sp28c);
                sub_GAME_7F06351C(&coord_node9_base, &sp24c, &sp28c, &sp1cc, &coord_node9_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // A
            else if (i == 8)
            {
                coord_node8_base = ((struct coord3d *)watchControllerButtonBases)[11];
                coord_node8_pos = buttonpositions->a;

                if (joyGetButtons(*contpadnum, A_BUTTON))
                {
                    coord_node8_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node8_base, &sp24c, &sp28c, &sp1cc, &coord_node8_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // L
            else if (i == 10)
            {
                coord_node10_base = ((struct coord3d *)watchControllerButtonBases)[12];
                coord_node10_pos = buttonpositions->l;

                if (joyGetButtons(*contpadnum, L_TRIG))
                {
                    matrix_4x4_set_rotation_around_y(0.17453294f, &sp24c);
                }

                matrix_4x4_set_rotation_around_x(1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node10_base, &sp24c, &sp28c, &sp1cc, &coord_node10_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // D-pad
            else if (i == 3)
            {
                coord_node3_base = ((struct coord3d *)watchControllerButtonBases)[13];
                coord_node3_pos = buttonpositions->dPad;
                matrix_4x4_set_identity(&spb4);

                if (joyGetButtons(*contpadnum, U_JPAD))
                {
                    matrix_4x4_set_rotation_around_x(-0.17453294f, &sp24c);
                }
                else if (joyGetButtons(*contpadnum, D_JPAD))
                {
                    matrix_4x4_set_rotation_around_x(0.17453294f, &sp24c);
                }

                if (joyGetButtons(*contpadnum, L_JPAD))
                {
                    matrix_4x4_set_rotation_around_z(0.17453294f, &spb4);
                }
                else if (joyGetButtons(*contpadnum, R_JPAD))
                {
                    matrix_4x4_set_rotation_around_z(-0.17453294f, &spb4);
                }

                matrix_4x4_multiply_in_place(&spb4, &sp24c);
                matrix_4x4_set_rotation_around_x(-0.89759791f, &sp28c);
                sub_GAME_7F06351C(&coord_node3_base, &sp24c, &sp28c, &sp1cc, &coord_node3_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // Start
            else if (i == 1)
            {
                coord_node1_base = ((struct coord3d *)watchControllerButtonBases)[14];
                coord_node1_pos = buttonpositions->start;

                if (joyGetButtons(*contpadnum, START_BUTTON))
                {
                    coord_node1_base.y += -10.0f;
                }

                matrix_4x4_set_rotation_around_x(-1.0471976f, &sp28c);
                sub_GAME_7F06351C(&coord_node1_base, &sp24c, &sp28c, &sp1cc, &coord_node1_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            // Z
            else if (i == 12)
            {
                coord_node12_base = ((struct coord3d *)watchControllerButtonBases)[15];
                coord_node12_pos = buttonpositions->z;

                if (joyGetButtons(*contpadnum, Z_TRIG))
                {
                    matrix_4x4_set_rotation_around_x(-0.17453294f, &sp24c);
                }

                matrix_4x4_set_rotation_around_z(M_PI_F, &sp28c);
                sub_GAME_7F06351C(&coord_node12_base, &sp24c, &sp28c, &sp1cc, &coord_node12_pos, &sp20c, &sp38c);
                matrix_4x4_copy(&sp38c, &matrices[i]);
            }
            else
            {
                matrix_4x4_copy(basemtx, &matrices[i]);
            }
        }

        modelUpdateNodeRelations(&modelstack);
        renderdata.gdl = gdl;
        subdraw(&renderdata, &modelstack);
        gdl = renderdata.gdl;
        matrix_4x4_7F058C64();

        for (i = 0; i < objheader->numMatrices; i++)
        {
            matrix_4x4_copy((u32)modelstack.render_pos + i * sizeof(Mtxf), &sp41c);
            matrix_4x4_f32_to_s32(&sp41c, &modelstack.render_pos[i]);
        }

        matrix_4x4_7F058C88();
    }

    return gdl;
}


/**
 * Address: 7F064364
 */
Gfx *watchRenderControllerOpaque(Gfx *gdl, Mtxf *basemtx, bool animatebuttons, WatchContButtonPositions *buttonpositions, s8 *contpadnum)
{
    return watchRenderController(gdl, basemtx, 0xff, animatebuttons, buttonpositions, contpadnum);
}


/**
 * Address: 7F0643A0
 */
ALSoundState* gunGetFreeSfxState(void)
{
    s32 i;

    for (i = 0; i < NUM_IMPACT_SFX_STATES; i++) 
    {
        if (g_ImpactSfxStates[i] == NULL) 
        {
            return &g_ImpactSfxStates[i];
        }
    }
    
    return NULL;
}


void recall_joy2_hits_edit_detail_edit_flag(enum ITEM_IDS item, PropRecord* prop, s32 texture_index)
{
    s32 sp6C;
    u32 rnd1;
    u32 rnd2;
    ALSoundState* sound_state;
    struct RicochetSoundsSmall ricochet_sounds_small_copy;
    struct PunchSounds punch_sounds_copy;
    struct BulletFleshSounds bullet_flesh_sounds_copy;
    u32 sfx_index;

    sp6C = sub_GAME_7F0539E4(&prop->pos);

    rnd1 = randomGetNext();
    rnd2 = randomGetNext();

    D_800483C4 = texture_index;

    if (get_debug_joy2hitsedit_flag() == 0)
    {
        get_debug_joy2detailedit_flag();
    }

    if ((item == ITEM_REMOTEMINE)
        || (item == ITEM_PROXIMITYMINE)
        || (item == ITEM_TIMEDMINE)
        || (item == ITEM_BOMBCASE)
        || (item == ITEM_BUG)
        || (item == ITEM_MICROCAMERA)
        || (item == ITEM_PLASTIQUE)
        || (item == ITEM_WATCHLASER)
        || (item == ITEM_WATCHMAGNETATTRACT))
    {
        return;
    }

#ifdef BUGFIX_R1
    if (g_ClockTimer <= 0) { return; }
#endif

    sound_state = gunGetFreeSfxState();
    if (sound_state != NULL)
    {
        if ((prop->type != PROP_TYPE_CHR) && (prop->type != PROP_TYPE_VIEWER))
        {
            if (item == ITEM_LASER)
            {
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, RICO_LASER1_SFX, sound_state);
            }
            else
            {
                ricochet_sounds_small_copy = ricochet_sounds_small;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, ricochet_sounds_small_copy.arr[rnd1 % 20], sound_state);
            }

            if (sound_state->link.next != NULL)
            {
                sndCreatePostEvent((ALSoundState* ) sound_state->link.next, 8, sp6C);
            }
        }
        else
        {
            if (item == ITEM_KNIFE)
            {
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, HIT_BULLET_SNOW_SFX, sound_state);
            }
            else if (item == ITEM_FIST)
            {
                punch_sounds_copy = punch_sounds;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, punch_sounds_copy.arr[rnd1 % 3], sound_state);
            }
            else
            {
                bullet_flesh_sounds_copy = bullet_flesh_sounds;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, bullet_flesh_sounds_copy.arr[rnd1 % 2], sound_state);
            }

            if (sound_state->link.next != NULL) {
                sndCreatePostEvent((ALSoundState* ) sound_state->link.next, 8, sp6C);
            }
        }
    }

    sound_state = gunGetFreeSfxState();
    if ((sound_state != NULL) && (texture_index >= 0))
    {
        if (g_HitTypeSounds[g_Textures[texture_index].hitSound] != NULL)
        {
            if (g_HitTypeSounds[g_Textures[texture_index].hitSound]->sfx_len > 0)
            {
                sfx_index = rnd2 % g_HitTypeSounds[g_Textures[texture_index].hitSound]->sfx_len;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, g_HitTypeSounds[g_Textures[texture_index].hitSound]->sfx[sfx_index], sound_state);
            }

            if (sound_state->link.next != NULL)
            {
                chrobjSndCreatePostEventDefault((ALSoundState* ) sound_state->link.next, &prop->pos);
            }
        }
    }
#ifdef DEBUG
    osSyncPrintf("Shot prop: hittype %d\n", g_Textures[texture_index].hitSound);
#endif
#ifdef ENABLE_LOG
    osSyncPrintf("Shot prop:  %S\n", HIT_TYPE_ToString[g_Textures[texture_index].hitSound]);
#endif
}


void sub_GAME_7F064720(coord3d* pos)
{
    ALSoundState* sound;
    ALLink* link;

#ifdef BUGFIX_R1
    if (g_ClockTimer <= 0) { return; }
#endif

    sound = gunGetFreeSfxState();

    if (sound != NULL)
    {
        sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, HIT_BULLET_GLASS_SFX, sound);

        link = sound->link.next;
        if (link != NULL)
        {
            chrobjSndCreatePostEventDefault((ALSoundState* ) link, pos);
        }
    }
}


void recall_joy2_hits_edit_flag(enum ITEM_IDS item, coord3d* arg1, s32 texture_index)
{
    ALSoundState *sound_state;
    u32 rnd1;
    u32 rnd2;
    struct LaserRichochetSounds laser_copied;
    struct RicochetSoundsLarge rico_copied;
    u32 sfx_index;
    struct image_sound *img_sound;

    rnd1 = randomGetNext();
    rnd2 = randomGetNext();

    D_800483C4 = texture_index;
    get_debug_joy2hitsedit_flag();

#ifdef BUGFIX_R1
    if (g_ClockTimer <= 0) { return; }
#endif

    sound_state = gunGetFreeSfxState();
    if (sound_state != NULL)
    {
        if (item != ITEM_WATCHLASER)
        {
            if (item == ITEM_LASER)
            {
                laser_copied = laser_ricochet_sounds;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, laser_copied.arr[rnd1 % 2], sound_state);
            }
            else
            {
                rico_copied = ricochet_sounds_large;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, rico_copied.arr[rnd1 % 36], sound_state);
            }
        }

        if (sound_state->link.next != NULL)
        {
            chrobjSndCreatePostEventDefault((ALSoundState* ) sound_state->link.next, arg1);
        }
    }

    sound_state = gunGetFreeSfxState();
    if ((sound_state != NULL) && (texture_index >= 0))
    {
        img_sound = g_HitTypeSounds[g_Textures[texture_index].hitSound];
        if (img_sound->sfx_len > 0)
        {
            if (img_sound != NULL)
            {
                sfx_index = rnd2 % img_sound->sfx_len;
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, img_sound->sfx[sfx_index], sound_state);
            }

            if (sound_state->link.next != NULL)
            {
                chrobjSndCreatePostEventDefault((ALSoundState* ) sound_state->link.next, arg1);
            }
        }
    }
}


void sub_GAME_7F064934(ITEM_IDS item)
{
    struct EarWhistleSounds copied;

#ifdef BUGFIX_R1
    if (g_ClockTimer <= 0) { return; }
#endif
    if ((item != ITEM_LASER) && (item != ITEM_WATCHLASER))
    {
        copied = ear_whistle_sounds;
        sndPlaySfx((struct ALBankAlt_s*) g_musicSfxBufferPtr, copied.arr[randomGetNext() % 5], 0);
    }
}


f32 sub_GAME_7F0649AC(s32 param_1)
{
  f32 fVar1;

  fVar1 = -60.0f;
  if (param_1 == 0x19) {
    fVar1 -= 20.0f;
  }
  return fVar1;
}



void sub_GAME_7F0649D8(enum GUNHAND hand)
{
    struct hand* hand_ptr;
    enum ITEM_IDS item_id;
    s32 ammo_in_magazine;
    s32 ammo_in_hands;
    WeaponStats* item_stats;
    s32 magsize;
    s32 ammo_total;

    hand_ptr = &g_CurrentPlayer->hands[hand];
    item_id = getCurrentPlayerWeaponId(hand);
    ammo_in_magazine = hand_ptr->weapon_ammo_in_magazine;
    ammo_in_hands = get_ammo_in_hands_weapon(hand);
    item_stats = get_ptr_item_statistics(item_id);
    ammo_total = ammo_in_hands + ammo_in_magazine;

    hand_ptr->weapon_ammo_in_magazine = (ammo_total >= item_stats->MagSize)
        ? item_stats->MagSize
        : ammo_total;

    g_CurrentPlayer->ammoheldarr[item_stats->AmmoType] = (bondwalkItemCheckBitflags(item_id, WEAPONSTATBITFLAG_AMMO_CLIP_LIMIT) != 0)
        ? 0
        : (g_CurrentPlayer->ammoheldarr[item_stats->AmmoType] - hand_ptr->weapon_ammo_in_magazine) + ammo_in_magazine;

    if (item_id == ITEM_ROCKETLAUNCH)
    {
        currentPlayerCreateRocket(hand);
        return;
    }

    if ((item_id == ITEM_SHOTGUN) || (item_id == ITEM_AUTOSHOT))
    {
        ammo_in_hands = get_ammo_in_hands_weapon(hand);
        if (ammo_in_hands >= 5)
        {
            hand_ptr->numvisibleshells = 5;
            return;
        }
        hand_ptr->numvisibleshells = ammo_in_hands;
    }
}

#if defined(VERSION_US) || defined(VERSION_JP)
    #define WEAPON_1P_ANIM_TIME(x) ((f32)(x))
    #define WHEN_1_CASE_GRENADELAUNCH_FLD890 6
    #define WHEN_1_CASE_GRENADE_FLD890 0xf0
    #define WHEN_D_FLD890 0x14
    #define WHEN_5_SP188_INIT 0x10
    #define WHEN_5_SP188_MULTI 0xc
    #define WHEN_5_FLD8B0_SP 0x11
    #define WHEN_5_FLD8B0_MULTI 0xd
    #define WHEN_8_SP178_INIT 0x17
    #define WHEN_8_SP178_MULTI 0xc
    #define WHEN_A_FLD890 0x10
    #define WHEN_A_FLD8B0 0x11
    #define WHEN_C_FLD890 0x17
    #define WHEN_E_FLD890 0x10
    #define WHEN_10_FLD890 0x17
    #define WHEN_11_FLD890_1 0x10
    #define WHEN_11_FLD890_2 0x18
    #define WHEN_1E_FLD890 0x1e
#endif
#if defined(VERSION_EU)
    #define WEAPON_1P_ANIM_TIME(x) ((f32)(x)) * 60.0f / 50.0f
    #define WHEN_1_CASE_GRENADELAUNCH_FLD890 5
    #define WHEN_1_CASE_GRENADE_FLD890 0xc8
    #define WHEN_D_FLD890 0x10
    #define WHEN_5_SP188_INIT 0xd
    #define WHEN_5_SP188_MULTI 0xa
    #define WHEN_5_FLD8B0_SP 0xe
    #define WHEN_5_FLD8B0_MULTI 0xa
    #define WHEN_8_SP178_INIT 0x13
    #define WHEN_8_SP178_MULTI 0xa
    #define WHEN_A_FLD890 0xd
    #define WHEN_A_FLD8B0 0xe
    #define WHEN_C_FLD890 0x13
    #define WHEN_E_FLD890 0xd
    #define WHEN_10_FLD890 0x13
    #define WHEN_11_FLD890_1 0xd
    #define WHEN_11_FLD890_2 0x14
    #define WHEN_1E_FLD890 0x19
#endif


/**
 * Address: 7F064B28
 */
void gunTickHandState(enum GUNHAND hand, s32 triggerOn)
{
#if defined(VERSION_US)
    s32 stack1;
    s32 stack2;
    s32 sp1C4;
    s32 stack3;
    struct hand *sp1BC;
    s32 stack4;
    s32 sp1B4;
    struct sfx2 sp1B0;
    s32 stack5;
    struct WeaponStats *weapon_stats;
    s32 sp1A4;
    s32 sp1A0;
    f32 sp19C;
    f32 sp198;
    s32 stack7;
    f32 sp190;
    f32 sp18C;
    s32 sp188;
    f32 sp184;
    s32 stack8;
    s32 stack9;
    s32 sp178;
    f32 sp174;
    s32 stack10;
    s32 stack11;
    Mtxf sp12C;
    f32 sp128;
    s32 stack12;
    Mtxf spE4;
    f32 tempf;
    struct hand *handptr;
    Mtxf sp9C;
    f32 sp98;
    f32 sp94;
    enum ITEM_IDS temp_v0_3;
    f32 sp8C;
    f32 sp88;
    enum ITEM_IDS var_s1;
    struct sfx3 sp7C;
    struct PropRecord *temp_v0_8;
    Weapon1PTransformKeyframe *sp74;
    f32 temp_f0_2;
    u32 var_a0_2;
    f32 temp_v1_9;
    struct hand *temp_v1_5;
    f32 un_f32_num = 0.0f;
    f32 un_f32_div_1 = 16.0f;
    f32 un_f32_div_2 = 23.0f;
    s32 stack14;
    s32 stack15;
#endif
#if defined(VERSION_JP)
    s32 stack1;
    s32 stack2;
    s32 sp1C4;
    s32 stack3;
    struct hand *sp1BC;
    s32 stack4;
    s32 sp1B4;
    struct sfx2 sp1B0;
    s32 stack5;
    struct WeaponStats *weapon_stats;
    s32 sp1A4;
    s32 sp1A0;
    f32 sp19C;
    s32 stat_2;
    s32 stat_3;
    s32 stat_4;
    f32 sp198;
    s32 stack14;
    f32 sp190;
    f32 sp18C;
    s32 sp188;
    f32 sp184;
    s32 stack7;
    s32 stack8;
    s32 sp178;
    f32 sp174;
    s32 stack9;
    s32 stack10;
    Mtxf sp12C;
    f32 sp128;
    s32 stack11;
    Mtxf spE4;
    s32 stack12;
    f32 tempf;
    Mtxf sp9C;
    f32 sp98;
    f32 sp94;
    struct hand *handptr;
    f32 sp8C;
    f32 sp88;
    enum ITEM_IDS temp_v0_3;
    struct sfx3 sp7C;
    enum ITEM_IDS var_s1;
    Weapon1PTransformKeyframe *sp74;
    struct PropRecord *temp_v0_8;
    f32 temp_f0_2;
    u32 var_a0_2;
    f32 temp_v1_9;
    struct hand *temp_v1_5;
    f32 un_f32_num = 0.0f;
    f32 un_f32_div_1 = 16.0f;
    f32 un_f32_div_2 = 23.0f;
    s32 stack15;
#endif
#if defined(VERSION_EU)
    s32 stack1;
    s32 stack2;
    s32 sp1C4;
    s32 stack3;
    struct hand *sp1BC;
    s32 stack4;
    s32 sp1B4;
    struct sfx2 sp1B0;
    s32 stack5;
    struct WeaponStats *weapon_stats;
    s32 sp1A4;
    s32 sp1A0;
    f32 sp19C;
    s32 stat_2;
    s32 stat_3;
    s32 stat_4;
    f32 sp198;
    s32 stack14;
    f32 sp190;
    f32 sp18C;
    s32 sp188;
    f32 sp184;
    s32 stack7;
    s32 stack8;
    s32 sp178;
    f32 sp174;
    s32 stack9;
    s32 stack10;
    Mtxf sp12C;
    f32 sp128;
    s32 stack11;
    Mtxf spE4;
    s32 stack12;
    f32 tempf;
    Mtxf sp9C;
    f32 sp98;
    f32 sp94;
    struct hand *handptr;
    f32 sp8C;
    f32 sp88;
    enum ITEM_IDS temp_v0_3;
    struct sfx3 sp7C;
    enum ITEM_IDS var_s1;
    Weapon1PTransformKeyframe *sp74;
    struct PropRecord *temp_v0_8;
    f32 temp_f0_2;
    u32 var_a0_2;
    f32 temp_v1_9;
    struct hand *temp_v1_5;
    f32 un_f32_num = 0.0f;
    f32 un_f32_div_1 = 13.0f;
    f32 un_f32_div_2 = 19.0f;
    s32 stack15;
#endif

    handptr = &g_CurrentPlayer->hands[hand];
    var_s1 = get_item_in_hand_or_watch_menu(hand);
    sp1C4 = get_ammo_type_for_weapon(var_s1);

    handptr->field_884 = handptr->weapon_hold_time;
    handptr->weapon_hold_time = triggerOn;

    if (triggerOn == 0)
    {
        handptr->field_888 = 1;
    }

    handptr->weapon_firing_status = 0;
    handptr->field_87D = 0;

    if (g_ClockTimer > 0)
    {
        handptr->field_890 += g_ClockTimer;
        handptr->field_88C += 1;
    }

#ifdef GE_MODDED_CHEATS
    /* R21 RC9 Rapid Fire: reproduce the proven Project64 code at the native
     * weapon layer.  The script writes 0xff to byte +3 of field_890 on N64
     * (the integer's low byte), so OR the timer with 0xff every hand tick.
     * This makes recoil/pump/bolt cadence gates expire immediately without
     * inventing weapon-state transitions or altering damage/ammo rules. */
    if (cheatIsActive(CHEAT_RAPID_FIRE))
    {
        handptr->field_890 |= 0xff;
    }
#endif

    handptr->field_92C = 0;

    if (handptr->weapon_action_state == GUN_ANIM_STATE_IDLE)
    {
#if defined(VERSION_JP) || defined(VERSION_EU)
        if ((var_s1 == ITEM_LASER) && (handptr->field_888 != 0))
        {
            handptr->field_8A0 = 0;
        }
#endif
        if (
            (handptr->weapon_hold_time != 0)
            && (var_s1 != ITEM_UNARMED)
            && (((bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_CLICKY) != 0)) || (handptr->weapon_ammo_in_magazine > 0))
#if defined(VERSION_JP) || defined(VERSION_EU)
            && ((var_s1 != ITEM_LASER) || (handptr->field_8A0 < 0xC8))
#endif
        )
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_TRIGGER_PRESS;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            handptr->field_888 = 0;
        }
        else
        {
            if (handptr->weapon_current_animation != 0)
            {
                handptr->weapon_action_state = handptr->weapon_current_animation;
                handptr->field_890 = 0;
                handptr->field_88C = 0;
            }
        }

        handptr->weapon_current_animation = 0;

        if ((handptr->weapon_action_state == GUN_ANIM_STATE_IDLE)
            && (handptr->weapon_ammo_in_magazine == 0)
            && (sp1C4 != 0))
        {
            if ((lvlGetControlsLockedFlag() == 0) && (g_CurrentPlayer->mpmenuon == 0))
            {
                /**
                 * D_80032458 is always 0 so this branch can never execute.
                 */
                if ((D_80032458 != 0) && (sp1C4 == 1) && (g_CurrentPlayer->ammoheldarr[sp1C4] <= 0))
                {
                    g_CurrentPlayer->ammoheldarr[sp1C4] = 1;
                }

                if (get_ammo_in_hands_weapon(hand) > 0)
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RELOAD_START;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                else
                {
                    if (g_CurrentPlayer->trigger_released != 0)
                    {
                        temp_v0_3 = get_item_in_hand_or_watch_menu(1 - hand);

                        sp1BC = (g_CurrentPlayer->hands - hand) + 1;

                        if ((sp1BC->weapon_action_state == GUN_ANIM_STATE_IDLE)
                            && (sp1BC->weapon_current_animation == 0)
                            && (
                                (temp_v0_3 == ITEM_UNARMED)
                                || ((sp1BC->weapon_ammo_in_magazine == 0)
                                    && ((get_ammo_type_for_weapon(temp_v0_3) != 0))
                                    && ((get_ammo_in_hands_weapon(1 - hand) <= 0)))))
                        {
                            autoadvance_on_deplete_all_ammo();

                            handptr->field_88C = 0;
                            handptr->field_890 = 0;
                            handptr->weapon_action_state = handptr->weapon_current_animation;
                            handptr->weapon_current_animation = 0;

                            sp1BC->field_88C = 0;
                            sp1BC->field_890 = 0;
                            sp1BC->weapon_action_state = sp1BC->weapon_current_animation;
                            sp1BC->weapon_current_animation = 0;
                        }
                    }
                }
            }
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_TRIGGER_PRESS)
    {
        switch (var_s1)
        {
        case ITEM_RUGER:
        case ITEM_GRENADELAUNCH:
            if (handptr->field_890 >= WHEN_1_CASE_GRENADELAUNCH_FLD890)
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_FIRE;
                handptr->field_890 = 0;
                handptr->field_88C = 0;
            }
            break;
        case ITEM_CAMERA:
            if (handptr->field_88C == 0)
            {
                currentPlayerSetFadeColour(0, 0, 0, 1.0f);
            }
            else if (handptr->field_890 > 0)
            {
                currentPlayerAdjustFade(8.0f, 0, 0, 0, 0.0f);
                handptr->weapon_action_state = GUN_ANIM_STATE_FIRE;
                handptr->field_890 = 0;
                handptr->field_88C = 0;
            }
            break;
        case ITEM_WPPK:
        case ITEM_WPPKSIL:
        case ITEM_TT33:
        case ITEM_SKORPION:
        case ITEM_AK47:
        case ITEM_UZI:
        case ITEM_MP5K:
        case ITEM_MP5KSIL:
        case ITEM_SPECTRE:
        case ITEM_M16:
        case ITEM_FNP90:
        case ITEM_SHOTGUN:
        case ITEM_AUTOSHOT:
        case ITEM_SNIPERRIFLE:
        case ITEM_GOLDENGUN:
        case ITEM_SILVERWPPK:
        case ITEM_GOLDWPPK:
        case ITEM_LASER:
        case ITEM_WATCHLASER:
        case ITEM_ROCKETLAUNCH:
        case ITEM_TRIGGER:
        case ITEM_TANKSHELLS:
        case ITEM_FLAREPISTOL:
        case ITEM_PITONGUN:
        case ITEM_WATCHMAGNETATTRACT:
            handptr->weapon_action_state = GUN_ANIM_STATE_FIRE;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_TIMEDMINE:
        case ITEM_PROXIMITYMINE:
        case ITEM_REMOTEMINE:
        case ITEM_BOMBCASE:
        case ITEM_PLASTIQUE:
        case ITEM_BUG:
        case ITEM_MICROCAMERA:
        case ITEM_GOLDENEYEKEY:
            handptr->weapon_action_state = GUN_ANIM_STATE_MINE_PLACE;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_KNIFE:
            if (!(randomGetNext() & 1))
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH2_BEGIN;
            }
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_GRENADE:
            if ((handptr->field_888 != 0) || (handptr->field_890 >= WHEN_1_CASE_GRENADE_FLD890))
            {
                g_CurrentPlayer->last_z_trigger_timer = handptr->field_890;
                handptr->weapon_action_state = GUN_ANIM_STATE_GRENADE_THROW;
                handptr->field_88C = 0;
                handptr->field_890 = 0;
            }
            break;
        case ITEM_FIST:
            if (!(randomGetNext() & 1))
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_PUNCH1_STRIKE;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_PUNCH2_STRIKE;
            }
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_THROWKNIFE:
            handptr->weapon_action_state = GUN_ANIM_STATE_THROWKNIFE_DRAW;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_TASER:
            tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
            if (gunSample1PTransform(taserFireKeyFrames, tempf, &handptr->field_8EC, hand) != 0)
            {
                handptr->field_92C = 1;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_FIRE;
                handptr->field_890 = 0;
                handptr->field_88C = 0;
            }
            break;
        case ITEM_BUNGEE:
        case ITEM_DOORDECODER:
        case ITEM_BOMBDEFUSER:
        case ITEM_LOCKEXPLODER:
        case ITEM_DOOREXPLODER:
        case ITEM_WEAPONCASE:
        case ITEM_SAFECRACKERCASE:
        case ITEM_KEYANALYSERCASE:
        case ITEM_BUGDETECTOR:
        case ITEM_EXPLOSIVEFLOPPY:
        case ITEM_POLARIZEDGLASSES:
        case ITEM_DARKGLASSES:
        case ITEM_CREDITCARD:
        case ITEM_GASKEYRING:
        case ITEM_DATATHIEF:
        case ITEM_WATCHIDENTIFIER:
        case ITEM_WATCHCOMMUNICATOR:
        case ITEM_WATCHGEIGERCOUNTER:
        case ITEM_WATCHMAGNETREPEL:
        case ITEM_DATTAPE:
        case ITEM_KEYCARD:
        case ITEM_KEYYALE:
        case ITEM_KEYBOLT:
            handptr->weapon_action_state = GUN_ANIM_STATE_USE_ITEM;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        case ITEM_SUIT_LF_HAND:
        case ITEM_JOYPAD:
        case ITEM_ROCKETROUND:
        case ITEM_GRENADEROUND:
        case ITEM_TOKEN:
        default:
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
            break;
        }

        handptr->volley = 0;
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_FIRE)
    {
        if ((get_ammo_type_for_weapon(var_s1) == 0) || (handptr->weapon_ammo_in_magazine > 0))
        {
            switch (var_s1)
            {
            case ITEM_CAMERA:
            case ITEM_WATCHMAGNETATTRACT:
                if (handptr->field_88C == 0)
                {
                    handptr->weapon_firing_status = (lvlGetControlsLockedFlag() == 0) && (g_CurrentPlayer->mpmenuon == 0);
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                break;
            case ITEM_WPPK:
            case ITEM_WPPKSIL:
            case ITEM_TT33:
            case ITEM_SHOTGUN:
            case ITEM_AUTOSHOT:
            case ITEM_SNIPERRIFLE:
            case ITEM_RUGER:
            case ITEM_GOLDENGUN:
            case ITEM_SILVERWPPK:
            case ITEM_GOLDWPPK:
            case ITEM_LASER:
            case ITEM_WATCHLASER:
            case ITEM_GRENADELAUNCH:
            case ITEM_ROCKETLAUNCH:
            case ITEM_TRIGGER:
            case ITEM_TANKSHELLS:
            case ITEM_FLAREPISTOL:
            case ITEM_PITONGUN:
                if (handptr->field_88C == 0)
                {
                    if ((getPlayerCount() == 1) || ((checkGamePaused() == 0) && (g_CurrentPlayer->mpmenuon == 0)))
                    {
                        handptr->field_87D = 1;
                    }

                    handptr->weapon_firing_status = (lvlGetControlsLockedFlag() == 0) && (g_CurrentPlayer->mpmenuon == 0);

                    sub_GAME_7F05E808(hand);
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                break;
            case ITEM_SKORPION:
            case ITEM_AK47:
            case ITEM_UZI:
            case ITEM_MP5K:
            case ITEM_MP5KSIL:
            case ITEM_SPECTRE:
            case ITEM_M16:
            case ITEM_FNP90:
                if ((handptr->field_88C == 0)
                    || (handptr->weapon_hold_time != 0)
                    || ((bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_BURST_FIRE) != 0)
                        && (currentPlayerGetIsAiming() == 0)
                        && (((s32) handptr->volley % 3) != 0)))
                {
                    if (((s32) handptr->field_88C % bondwalkItemGetAutomaticFiringRate(var_s1)) == 0)
                    {
                        if ((getPlayerCount() == 1) || ((checkGamePaused() == 0) && (g_CurrentPlayer->mpmenuon == 0)))
                        {
                            handptr->field_87D = 1;
                        }

                        handptr->weapon_firing_status = (lvlGetControlsLockedFlag() == 0)
                            && (g_CurrentPlayer->mpmenuon == 0);
                    }
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                break;
            case ITEM_KNIFE:
                if ((handptr->field_88C == 0) || (handptr->weapon_hold_time != 0))
                {
                    handptr->weapon_firing_status = 0;
                    handptr->field_87D = handptr->weapon_firing_status;
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                break;
            case ITEM_TASER:
                if ((handptr->field_88C == 0) || (handptr->weapon_hold_time != 0))
                {
                    gunSample1PTransform(taserRaiseKeyframes, 0.0f, &handptr->field_8EC, hand);

                    handptr->weapon_firing_status = 0;
                    handptr->field_92C = 1;
                    handptr->field_87D = handptr->weapon_firing_status;

                    if (handptr->field_88C == 0)
                    {
                        handptr->weapon_firing_status = (lvlGetControlsLockedFlag() == 0)
                            && (g_CurrentPlayer->mpmenuon == 0);
                    }
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
                    handptr->field_890 = 0;
                    handptr->field_88C = 0;
                }
                break;
            }

            if (handptr->weapon_firing_status != 0)
            {
                if (var_s1 != ITEM_CAMERA)
                {
                    joyRumblePakStart(get_cur_playernum(), 0.1f);

                    if (cur_player_get_control_type() >= 4)
                    {
                        joyRumblePakStart(get_cur_playernum() + getPlayerCount(), 0.1f);
                    }
                }

#ifdef GE_MODDED_CHEATS
                if (!cheatIsActive(CHEAT_NO_RELOAD)
                    || bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_NO_CLIP_RELOADS) != 0)
#endif
                {
                    handptr->weapon_ammo_in_magazine -= 1;
                }
                handptr->volley += 1;
            }

            if (handptr->weapon_action_state == GUN_ANIM_STATE_FIRE)
            {
                sp1B4 = 0;

                if (bondwalkItemGetSoundTriggerRate(var_s1) > 0)
                {
                    if ((g_CurrentPlayer->hands[1 - hand].field_A50 != g_GlobalTimer)
                        && (handptr->field_A4C < g_GlobalTimer))
                    {
                        handptr->field_A4C = bondwalkItemGetSoundTriggerRate(var_s1) + g_GlobalTimer;
                        sp1B4 = 1;
                    }
                }
                else if (handptr->weapon_firing_status != 0)
                {
                    sp1B4 = 1;
                }

                if ((getPlayerCount() == 1) || ((checkGamePaused() == 0) && (g_CurrentPlayer->mpmenuon == 0)))
                {
                    if (sp1B4 != 0)
                    {
                        if ((handptr->audioHandle != NULL) && (sndGetPlayingState(handptr->audioHandle) != 0))
                        {
                            sndDeactivate(handptr->audioHandle);
                        }

                        if (((struct ALSoundState *)handptr->field_A48 != 0)
                            && (sndGetPlayingState((struct ALSoundState *) handptr->field_A48) != 0))
                        {
                            sndDeactivate((struct ALSoundState *) handptr->field_A48);
                        }

                        if (bondwalkItemGetSound(var_s1) != 0)
                        {
                            if (handptr->audioHandle == NULL)
                            {
                                sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, bondwalkItemGetSound(var_s1), (struct ALSoundState *) &handptr->audioHandle);
                            }
                            else if ((struct ALSoundState *)handptr->field_A48 == 0)
                            {
                                sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, bondwalkItemGetSound(var_s1), (struct ALSoundState *) &handptr->field_A48);
                            }

                            handptr->field_A50 = g_GlobalTimer;
                        }
                    }

                    if (var_s1 == ITEM_WATCHLASER)
                    {
                        sp1B0 = watchlaser_fire_sounds;
                        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, sp1B0.half[randomGetNext() & 1], NULL);
                    }
                }
            }
        }
        else if (handptr->field_88C > 0)
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL1;
            handptr->field_890 = 0;
            handptr->field_88C = 0;
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_DRY_FIRE;
            handptr->field_890 = 0;
            handptr->field_88C = 0;

            if ((getPlayerCount() == 1)
#if defined(VERSION_JP) || defined(VERSION_EU)
                || ((checkGamePaused() == 0) && (g_CurrentPlayer->mpmenuon == 0))
#else
                || (checkGamePaused() == 0)
#endif
               )
            {
                sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, EMPTY_GUN_FIRE_SFX, NULL);
            }
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RECOIL1)
    {
        if (var_s1 == ITEM_TASER)
        {
            tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
            if (gunSample1PTransform(taserRaiseKeyframes, tempf, &handptr->field_8EC, hand) != 0)
            {
                handptr->field_92C = 1;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
        }
        else
        {
            weapon_stats = get_ptr_item_statistics(var_s1);

#if defined(VERSION_US)
            sp1A4 = weapon_stats->b44[0];
            sp1A0 = weapon_stats->b44[1];
#endif
#if defined(VERSION_JP)
            sp1A4 = weapon_stats->b44[0];
            sp1A0 = weapon_stats->b44[1];
            stat_2 = weapon_stats->b44[2];
            stat_3 = weapon_stats->b44[3];
            stat_4 = weapon_stats->SingleFiringRate;
#endif
#if defined(VERSION_EU)
            sp1A4 = ((s32)weapon_stats->b44[0] * 50) / 60;
            sp1A0 = ((s32)weapon_stats->b44[1] * 50) / 60;
            stat_2 = ((s32)weapon_stats->b44[2] * 50) / 60;
            stat_3 = ((s32)weapon_stats->b44[3] * 50) / 60;
            stat_4 = weapon_stats->SingleFiringRate * 50 / 60;
#endif

            if ((
                    (handptr->field_888 != 0)
                    && (handptr->field_890 >= (sp1A4 + sp1A0))
                )
                ||
                (
                    ((weapon_stats->SingleFiringRate >= 0))
                    && (handptr->field_888 == 0)
#if defined(VERSION_US)
                    && (handptr->field_890 >= (sp1A4 + sp1A0 + weapon_stats->SingleFiringRate))
#endif
#if defined(VERSION_JP) ||  defined(VERSION_EU)
                    && (handptr->field_890 >= (sp1A4 + sp1A0 + stat_4))
#endif
                )
               )
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
            else if (
                (handptr->field_888 != 0)
                && (handptr->weapon_hold_time != 0)

#if defined(VERSION_US)
                && (handptr->field_890 >= weapon_stats->b44[2])
#endif
#if defined(VERSION_JP) ||  defined(VERSION_EU)
                && (handptr->field_890 >= stat_2)
#endif

                && (weapon_stats->b44[3] >= 0)

#if defined(VERSION_US)
                // HACK: registers are swapped
                // addu a1, v1, a0
                && (handptr->field_890 + weapon_stats->b44[3] < (0,sp1A4) + sp1A0)
                && (handptr->field_890 + weapon_stats->b44[3] >= (s32)weapon_stats->b44[2])
#endif
#if defined(VERSION_JP) ||  defined(VERSION_EU)
                && (handptr->field_890 + stat_3 < sp1A4 + sp1A0)
                && (handptr->field_890 + stat_3 >= (s32)stat_2)
#endif
            )
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_RECOIL2;
                handptr->field_890 = 0;
                handptr->field_88C = 0;
#if defined(VERSION_US)
                handptr->field_8A8 = weapon_stats->b44[3];
#endif
#if defined(VERSION_JP) ||  defined(VERSION_EU)
                handptr->field_8A8 = stat_3;
#endif
            }
            else if (handptr->field_890 < sp1A4 + sp1A0)
            {
                sp198 = weapon_stats->RecoilBack;
                sp19C = weapon_stats->RecoilUp;

                if (handptr->field_890 == 0)
                {
                    handptr->field_8C8 = handptr->field_8E8;
                    handptr->field_8BC = handptr->field_8DC;
                    handptr->field_8C0 = handptr->field_8E0;
                    handptr->field_8C4 = handptr->field_8E4;
                }

                if (handptr->field_890 < sp1A4)
                {
                    handptr->field_8D8 = M_TAU_F - ((sp19C * M_TAU_F) / 360.0f);

                    handptr->field_8CC = ((gunSetHorizontalOffset(hand) - handptr->field_A38) * sp198) / 1000.0f;
                    handptr->field_8D0 = 0;
                    handptr->field_8D4 = ((weapon_stats->PosZ - handptr->field_A40) * sp198) / 1000.0f;

                    sp190 = sinf(((f32) handptr->field_890 * M_PI_2F) / (f32) sp1A4);
                }
                else
                {
                    handptr->field_8D8 = M_TAU_F - ((sp19C * M_TAU_F) / 360.0f);

                    handptr->field_8CC = ((gunSetHorizontalOffset(hand) - handptr->field_A38) * sp198) / 1000.0f;
                    handptr->field_8D0 = 0;
                    handptr->field_8D4 = ((weapon_stats->PosZ - handptr->field_A40) * sp198) / 1000.0f;

                    sp190 = (cosf(((f32) (handptr->field_890 - sp1A4) * M_PI_F) / (f32) sp1A0) * 0.5f) + 0.5f;
                }

                temp_f0_2 = sub_GAME_7F06D0CC(handptr->field_8C8, handptr->field_8D8, sp190);

                handptr->field_8E8 = temp_f0_2;
                handptr->field_92C = 1;
                handptr->field_8DC = ((handptr->field_8CC - handptr->field_8BC) * sp190) + handptr->field_8BC;
                handptr->field_8E0 = ((handptr->field_8D0 - handptr->field_8C0) * sp190) + handptr->field_8C0;
                handptr->field_8E4 = ((handptr->field_8D4 - handptr->field_8C4) * sp190) + handptr->field_8C4;

                matrix_4x4_set_rotation_around_x(temp_f0_2, &handptr->field_8EC);
                matrix_4x4_set_position((struct coord3d *)&handptr->field_8DC, &handptr->field_8EC);
            }
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RECOIL2)
    {
        if (handptr->field_890 == 0)
        {
            handptr->field_8C8 = handptr->field_8E8;
            handptr->field_8BC = handptr->field_8DC;
            handptr->field_8C0 = handptr->field_8E0;
            handptr->field_8C4 = handptr->field_8E4;
            handptr->field_8D8 = 0.0f;
            handptr->field_8CC = 0.0f;
            handptr->field_8D0 = 0.0f;
            handptr->field_8D4 = 0.0f;
        }

        if (handptr->field_890 < handptr->field_8A8)
        {
            sp18C = (cosf(((f32) (handptr->field_8A8 - handptr->field_890) * M_PI_2F) / (f32) handptr->field_8A8) * 0.5f) + 0.5f;

            temp_f0_2 = sub_GAME_7F06D0CC(handptr->field_8C8, handptr->field_8D8, sp18C);

            handptr->field_8E8 = temp_f0_2;
            handptr->field_92C = 1;
            handptr->field_8DC = ((handptr->field_8CC - handptr->field_8BC) * sp18C) + handptr->field_8BC;
            handptr->field_8E0 = ((handptr->field_8D0 - handptr->field_8C0) * sp18C) + handptr->field_8C0;
            handptr->field_8E4 = ((handptr->field_8D4 - handptr->field_8C4) * sp18C) + handptr->field_8C4;

            matrix_4x4_set_rotation_around_x(temp_f0_2, &handptr->field_8EC);
            matrix_4x4_set_position((struct coord3d *)&handptr->field_8DC, &handptr->field_8EC);
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_DRY_FIRE)
    {
        if (handptr->field_88C == 0)
        {
            sub_GAME_7F05E808(hand);
        }

        if ((handptr->field_888 != 0) || ((handptr->field_888 == 0) && (handptr->field_890 >= WHEN_D_FLD890)))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    sp188 = WHEN_5_SP188_INIT;
    if (handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_LOWER)
    {
        if (getPlayerCount() >= 2)
        {
            sp188 = WHEN_5_SP188_MULTI;
        }

        if (handptr->field_88C == 0)
        {
            if (getPlayerCount() == 1)
            {
                handptr->field_8B0 = WHEN_5_FLD8B0_SP;
            }
            else
            {
                handptr->field_8B0 = WHEN_5_FLD8B0_MULTI;
            }
        }

        if (handptr->field_890 >= sp188)
        {
            g_CurrentPlayer->ammoheldarr[get_ammo_type_for_weapon(var_s1)] += handptr->weapon_ammo_in_magazine;
            handptr->weapon_ammo_in_magazine = 0;

            if (getPlayerCount() >= 2)
            {
                sub_GAME_7F09B368(hand);
            }

            sub_GAME_7F05FB00(hand);

            handptr->weapon_action_state = GUN_ANIM_STATE_SWITCH_SWAP;

            if (bondinvItemAvailable(ITEM_SNIPERRIFLE) != 0)
            {
                g_CurrentPlayer->cur_item_weapon_getname = ITEM_SNIPERRIFLE;
            }
            else
            {
                g_CurrentPlayer->cur_item_weapon_getname = ITEM_FIST;
            }
        }
        else
        {
            sp184 = ((f32) handptr->field_890 * M_LN2F) / (f32) sp188;
            handptr->field_92C = 1;

            matrix_4x4_set_rotation_around_x(sp184, &handptr->field_8EC);

            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(sp184)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(sp184) * 15.0f;
        }
    }

    if ((handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_SWAP) || (handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_HOLD))
    {
        if ((handptr->weapon_animation_trigger == 0) || (handptr->field_890 >= handptr->field_8B0))
        {
            if (handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_SWAP)
            {
                temp_v1_5 = (g_CurrentPlayer->hands - hand) + 1;

                if ((temp_v1_5->weapon_action_state != GUN_ANIM_STATE_SWITCH_SWAP) && (temp_v1_5->weapon_action_state != GUN_ANIM_STATE_SWITCH_LOWER))
                {
                    if (
                        (temp_v1_5->weapon_current_animation != 5)
                        && (temp_v1_5->weapon_action_state != GUN_ANIM_STATE_WATCH_LOWER)
                        && (temp_v1_5->weapon_action_state != GUN_ANIM_STATE_WATCH_SWAP)
                        && (temp_v1_5->weapon_action_state != GUN_ANIM_STATE_WATCH_RAISE)
                        && (temp_v1_5->weapon_current_animation != 0xE))
                    {
                        if (hand == GUNRIGHT)
                        {
                            if (bondinvItemAvailableForHand(handptr->weapon_next_weapon, getCurrentPlayerWeaponId(GUNLEFT)) == 0)
                            {
                                currentPlayerEquipWeaponWrapper(GUNLEFT, 0);
                            }
                        }
                        else if (bondinvItemAvailableForHand(getCurrentPlayerWeaponId(GUNRIGHT), handptr->weapon_next_weapon) == 0)
                        {
                            handptr->weapon_next_weapon = ITEM_UNARMED;
                        }
                    }
                }
                currentPlayerUnEquipWeaponWrapper(hand, handptr->weapon_next_weapon);
                var_s1 = get_item_in_hand_or_watch_menu(hand);
                handptr->weapon_action_state = GUN_ANIM_STATE_SWITCH_HOLD;
            }
            else if (Gun_hand_without_item(hand) != 0)
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_SWITCH_RAISE;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
        }

        if ((handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_SWAP) || (handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_HOLD))
        {
            handptr->field_92C = 1;
            matrix_4x4_set_rotation_around_x(M_LN2F, &handptr->field_8EC);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(M_LN2F)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(M_LN2F) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_SWITCH_RAISE)
    {
        sp178 = WHEN_8_SP178_INIT;

        if (getPlayerCount() >= 2)
        {
            sp178 = WHEN_8_SP178_MULTI;
        }

        if (handptr->field_88C == 0)
        {
            if (getPlayerCount() >= 2)
            {
                sub_GAME_7F09B398(hand);
            }

            sub_GAME_7F0649D8(hand);

            g_CurrentPlayer->trigger_released = 0;

            if ((g_ClockTimer > 0)
                && (g_CurrentPlayer->cameramode != CAMERAMODE_INTRO)
                && (Gun_hand_without_item(hand) != 0)
                && (g_PlayerInvincible == FALSE)
#if defined(VERSION_JP) || defined(VERSION_EU)
                && (g_CurrentPlayer->bonddead == 0)
#endif
               )
            {
                switch (var_s1)
                {
                    case ITEM_LASER:
                        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_LASER_SFX, NULL);
                        break;

                    case ITEM_KNIFE:
                    case ITEM_THROWKNIFE:
                        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_KNIFE_SFX, NULL);
                        break;

                    case ITEM_TIMEDMINE:
                    case ITEM_PROXIMITYMINE:
                    case ITEM_REMOTEMINE:
                        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_MINE_SFX, NULL);
                        break;

                    case ITEM_UNARMED:
                    case ITEM_FIST:
                    case ITEM_WATCHLASER:
                    case ITEM_GRENADE:
                    case ITEM_TRIGGER:
                    case ITEM_TASER:
                    case ITEM_TANKSHELLS:
                    case ITEM_BOMBCASE:
                    case ITEM_PLASTIQUE:
                    case ITEM_CAMERA:
                    case ITEM_BUG:
                    case ITEM_MICROCAMERA:
                    case ITEM_WATCHMAGNETATTRACT:
                    case ITEM_GOLDENEYEKEY:
                    case ITEM_TOKEN:
                        break;

                    default:
                        sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, PICKUP_GUN_SFX, NULL);
                        break;
                }
            }
        }

        if ((handptr->field_890 >= sp178)
            || (get_ptr_weapon_model_header_line(var_s1) == NULL)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) == 0)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_HIDE_FIRST_PERSON_HAND) != 0))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            sp174 = ((f32) (sp178 - handptr->field_890) * M_LN2F) / (f32) sp178;
            handptr->field_92C = 1;
            matrix_4x4_set_rotation_around_x(sp174, &handptr->field_8EC);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(sp174)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(sp174) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RELOAD_START)
    {
        if (((handptr->weapon_ammo_in_magazine < get_ptr_item_statistics(var_s1)->MagSize)
             || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_AMMO_CLIP_LIMIT) != 0))
            && ((get_ammo_in_hands_weapon(hand) > 0)))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_RELOAD_LOWER;
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RELOAD_LOWER)
    {
        if ((handptr->field_890 >= WHEN_A_FLD890) || (handptr->field_87F == 0))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_RELOAD_SWAP;
            handptr->field_8B0 = WHEN_A_FLD8B0;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            sp128 = ((f32) handptr->field_890 * M_LN2F) / un_f32_div_1;
            handptr->field_92C = 1;

            if (hand == GUNRIGHT)
            {
                matrix_4x4_set_rotation_around_z((un_f32_num / un_f32_div_1), &handptr->field_8EC);
            }
            else
            {
                matrix_4x4_set_rotation_around_z(-(un_f32_num / un_f32_div_1), &handptr->field_8EC);
            }

            matrix_4x4_set_rotation_around_x(sp128, &sp12C);
            matrix_4x4_multiply_in_place(&sp12C, &handptr->field_8EC);
            sinf((un_f32_num / un_f32_div_1));
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = sub_GAME_7F0649AC(var_s1) * (1.0f - cosf(sp128));
            handptr->field_8EC.m[3][2] = sinf(sp128) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RELOAD_SWAP)
    {
        if ((handptr->field_88C == 0)
#if defined(VERSION_JP) || defined(VERSION_EU)
            && (g_ClockTimer > 0)
#endif
            && (g_CurrentPlayer->cameramode != CAMERAMODE_INTRO)
            && (Gun_hand_without_item(hand) != 0)
            && (g_PlayerInvincible == FALSE)
#if defined(VERSION_JP) || defined(VERSION_EU)
            && (g_CurrentPlayer->bonddead == 0)
#endif
           )
        {
            switch (var_s1)
            {
            case ITEM_UNARMED:
            case ITEM_FIST:
            case ITEM_KNIFE:
            case ITEM_THROWKNIFE:
            case ITEM_LASER:
            case ITEM_WATCHLASER:
            case ITEM_GRENADE:
            case ITEM_TIMEDMINE:
            case ITEM_PROXIMITYMINE:
            case ITEM_REMOTEMINE:
            case ITEM_TRIGGER:
            case ITEM_TASER:
            case ITEM_TANKSHELLS:
            case ITEM_BOMBCASE:
            case ITEM_PLASTIQUE:
            case ITEM_CAMERA:
            case ITEM_BUG:
            case ITEM_MICROCAMERA:
            case ITEM_WATCHMAGNETATTRACT:
            case ITEM_GOLDENEYEKEY:
            case ITEM_TOKEN:
                break;
            default:
            case ITEM_WPPK:
            case ITEM_WPPKSIL:
            case ITEM_TT33:
            case ITEM_SKORPION:
            case ITEM_AK47:
            case ITEM_UZI:
            case ITEM_MP5K:
            case ITEM_MP5KSIL:
            case ITEM_SPECTRE:
            case ITEM_M16:
            case ITEM_FNP90:
            case ITEM_SHOTGUN:
            case ITEM_AUTOSHOT:
            case ITEM_SNIPERRIFLE:
            case ITEM_RUGER:
            case ITEM_GOLDENGUN:
            case ITEM_SILVERWPPK:
            case ITEM_GOLDWPPK:
            case ITEM_GRENADELAUNCH:
            case ITEM_ROCKETLAUNCH:
            case ITEM_FLAREPISTOL:
            case ITEM_PITONGUN:
            case ITEM_BUNGEE:
            case ITEM_DOORDECODER:
            case ITEM_BOMBDEFUSER:
            case ITEM_LOCKEXPLODER:
            case ITEM_DOOREXPLODER:
            case ITEM_BRIEFCASE:
            case ITEM_WEAPONCASE:
            case ITEM_SAFECRACKERCASE:
            case ITEM_KEYANALYSERCASE:
            case ITEM_BUGDETECTOR:
            case ITEM_EXPLOSIVEFLOPPY:
            case ITEM_POLARIZEDGLASSES:
            case ITEM_DARKGLASSES:
            case ITEM_CREDITCARD:
            case ITEM_GASKEYRING:
            case ITEM_DATATHIEF:
            case ITEM_WATCHIDENTIFIER:
            case ITEM_WATCHCOMMUNICATOR:
            case ITEM_WATCHGEIGERCOUNTER:
            case ITEM_WATCHMAGNETREPEL:
                sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, GUN_RIFLECOCK_SFX, NULL);
                break;
            }
        }

        if ((handptr->field_890 >= handptr->field_8B0) && !(((handptr->field_88C < 2))))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_RELOAD_RAISE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            handptr->field_92C = 1;

            if (hand == GUNRIGHT)
            {
                matrix_4x4_set_rotation_around_z(un_f32_num, &handptr->field_8EC);
            }
            else
            {
                matrix_4x4_set_rotation_around_z(-un_f32_num, &handptr->field_8EC);
            }

            matrix_4x4_set_rotation_around_x(M_LN2F, &spE4);
            matrix_4x4_multiply_in_place(&spE4, &handptr->field_8EC);
            sinf(un_f32_num);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = sub_GAME_7F0649AC(var_s1) * (1.0f - cosf(M_LN2F));
            handptr->field_8EC.m[3][2] = sinf(M_LN2F) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_RELOAD_RAISE)
    {
        if (handptr->field_88C == 0)
        {
            sub_GAME_7F0649D8(hand);
            g_CurrentPlayer->trigger_released = 0;
        }

        if ((handptr->field_890 >= WHEN_C_FLD890)
            || (get_ptr_weapon_model_header_line(var_s1) == NULL)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) == 0)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_HIDE_FIRST_PERSON_HAND) != 0))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            sp98 = ((f32) (WHEN_C_FLD890 - handptr->field_890) * M_LN2F) / un_f32_div_2;
            handptr->field_92C = 1;

            if (hand == GUNRIGHT)
            {
                matrix_4x4_set_rotation_around_z((un_f32_num / un_f32_div_2), &handptr->field_8EC);
            }
            else
            {
                matrix_4x4_set_rotation_around_z(-(un_f32_num / un_f32_div_2), &handptr->field_8EC);
            }

            matrix_4x4_set_rotation_around_x(sp98, &sp9C);
            matrix_4x4_multiply_in_place(&sp9C, &handptr->field_8EC);
            sinf(un_f32_num / un_f32_div_2);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = sub_GAME_7F0649AC(var_s1) * (1.0f - cosf(sp98));
            handptr->field_8EC.m[3][2] = sinf(sp98) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_WATCH_LOWER)
    {
        if ((handptr->field_890 >= WHEN_E_FLD890) || (handptr->field_87F == 0))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_WATCH_SWAP;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            sp94 = ((f32) handptr->field_890 * M_LN2F) / un_f32_div_1;
            handptr->field_92C = 1;

            matrix_4x4_set_rotation_around_x(sp94, &handptr->field_8EC);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(sp94)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(sp94) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_WATCH_SWAP)
    {
        if ((handptr->field_88C == 0) || (Gun_hand_without_item(hand) == 0))
        {
            sub_GAME_7F05DA8C(hand, handptr->weapon_next_weapon);
            var_s1 = get_item_in_hand_or_watch_menu(hand);
        }

        if (Gun_hand_without_item(hand) != 0)
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_WATCH_RAISE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            handptr->field_92C = 1;
            matrix_4x4_set_rotation_around_x(M_LN2F, &handptr->field_8EC);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(M_LN2F)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(M_LN2F) * 15.0f;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_WATCH_RAISE)
    {
        if ((handptr->field_88C == 0) && (var_s1 < 0x21))
        {
            if (getPlayerCount() >= 2)
            {
                sub_GAME_7F09B398(hand);
            }
            sub_GAME_7F0649D8(hand);
            g_CurrentPlayer->trigger_released = 0;
        }

        if ((handptr->field_890 >= WHEN_10_FLD890)
            || (get_ptr_weapon_model_header_line(var_s1) == NULL)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_SHOW_FIRST_PERSON) == 0)
            || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_HIDE_FIRST_PERSON_HAND) != 0))
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
        else
        {
            sp8C = ((f32) (WHEN_10_FLD890 - handptr->field_890) * M_LN2F) / un_f32_div_2;
            handptr->field_92C = 1;
            matrix_4x4_set_rotation_around_x(sp8C, &handptr->field_8EC);
            handptr->field_8EC.m[3][0] = 0.0f;
            handptr->field_8EC.m[3][1] = (1.0f - cosf(sp8C)) * -60.0f;
            handptr->field_8EC.m[3][2] = sinf(sp8C) * 15.0f;
        }
    }

    if ((handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_STRIKE)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_RECOVER)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH2_BEGIN)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH2_STRIKE)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH2_RECOVER))
    {
        sp88 = WEAPON_1P_ANIM_TIME(handptr->field_890);

        if (((handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN)
                || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH2_BEGIN))
                && (handptr->field_890 >= WHEN_11_FLD890_1))
        {
            sp7C = knife_throw_sounds;
            sndPlaySfx((struct ALBankAlt_s *) g_musicSfxBufferPtr, sp7C.half[randomGetNext() % 3U], NULL);


            if (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN)
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH1_STRIKE;
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH1_STRIKE;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH2_STRIKE;
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH2_STRIKE;
            }
        }

        if ((handptr->weapon_action_state != GUN_ANIM_STATE_KNIFE_SLASH1_RECOVER)
            && (handptr->weapon_action_state != GUN_ANIM_STATE_KNIFE_SLASH2_RECOVER)
            && (handptr->field_890 >= WHEN_11_FLD890_2))
        {
            handptr->weapon_firing_status = 1;
            if ((handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN) || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_STRIKE))
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH1_RECOVER;
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH1_RECOVER;
            }
            else
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH2_RECOVER;
                handptr->weapon_action_state = GUN_ANIM_STATE_KNIFE_SLASH2_RECOVER;
            }
        }

        if ((handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_BEGIN)
            || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_STRIKE)
            || (handptr->weapon_action_state == GUN_ANIM_STATE_KNIFE_SLASH1_RECOVER))
        {
            var_a0_2 = D_80034CA4;
        }
        else
        {
            var_a0_2 = D_80034E0C;
        }

        if (gunSample1PTransform(var_a0_2, sp88, &handptr->field_8EC, hand) != 0)
        {
            handptr->field_92C = 1;
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if ((handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH1_STRIKE)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH1_RECOVER)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH2_STRIKE)
        || (handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH2_RECOVER))
    {
        temp_v1_9 = WEAPON_1P_ANIM_TIME(handptr->field_890);

        if ((handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH1_STRIKE) || (handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH1_RECOVER))
        {
            if (g_CurrentPlayer->cur_item_weapon_getname == ITEM_SNIPERRIFLE)
            {
                sp74 = sniperMeleeKeyframes1;
            }
            else
            {
                sp74 = fistMeleeKeyframes1;
            }

            if ((handptr->weapon_action_state != GUN_ANIM_STATE_PUNCH1_RECOVER) && (handptr->field_890 >= WHEN_1E_FLD890))
            {
                handptr->weapon_firing_status = 1;
                handptr->weapon_action_state = GUN_ANIM_STATE_PUNCH1_RECOVER;
            }
        }
        else if ((handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH2_STRIKE) || (handptr->weapon_action_state == GUN_ANIM_STATE_PUNCH2_RECOVER))
        {
            if (g_CurrentPlayer->cur_item_weapon_getname == ITEM_SNIPERRIFLE)
            {
                sp74 = sniperMeleeKeyframes2;
            }
            else
            {
                sp74 = fistMeleeKeyframes2;
            }

            if ((handptr->weapon_action_state != GUN_ANIM_STATE_PUNCH2_RECOVER) && (handptr->field_890 >= WHEN_1E_FLD890))
            {
                handptr->weapon_firing_status = 1;
                handptr->weapon_action_state = GUN_ANIM_STATE_PUNCH2_RECOVER;
            }
        }

        if (gunSample1PTransform(sp74, temp_v1_9, &handptr->field_8EC, hand) != 0)
        {
            handptr->field_92C = 1;
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_GRENADE_THROW)
    {
        if (handptr->weapon_ammo_in_magazine > 0)
        {
            tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
            if (gunSample1PTransform(grenadeThrowKeyframes, tempf, &handptr->field_8EC, hand) != 0)
            {
                handptr->field_92C = 1;
            }
            else
            {
                handptr->field_87E = 0;
                handptr->weapon_firing_status = 1;
                handptr->weapon_ammo_in_magazine -= 1;
                handptr->weapon_action_state = GUN_ANIM_STATE_GRENADE_RECOVER;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_GRENADE_RECOVER)
    {
        tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
        if (gunSample1PTransform(timedMineThrowKeyframes, tempf, &handptr->field_8EC, hand) != 0)
        {
            handptr->field_92C = 1;
        }
        else
        {
            handptr->field_87E = 1;
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_THROWKNIFE_DRAW)
    {
        if (handptr->weapon_ammo_in_magazine > 0)
        {
            if (handptr->field_888 != 0)
            {
                handptr->weapon_action_state = GUN_ANIM_STATE_THROWKNIFE_THROW;
            }
            else
            {
                tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
                if (gunSample1PTransform(throwKnifeDrawBackKeyframes, tempf, &handptr->field_8EC, hand) != 0)
                {
                    handptr->field_92C = 1;
                }
                else if (gunSample1PTransform(throwKnifeReleaseKeyframes, 0.0f, &handptr->field_8EC, hand) != 0)
                {
                    handptr->field_92C = 1;
                }
                else
                {
                    handptr->weapon_action_state = GUN_ANIM_STATE_THROWKNIFE_THROW;
                }
            }
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_THROWKNIFE_THROW)
    {
        if (handptr->weapon_ammo_in_magazine > 0)
        {
            tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
            if (gunSample1PTransform(throwKnifeDrawBackKeyframes, tempf, &handptr->field_8EC, hand) != 0)
            {
                handptr->field_92C = 1;
            }
            else
            {
                handptr->field_87E = 0;
                handptr->weapon_firing_status = 1;
                handptr->weapon_ammo_in_magazine -= 1;
                handptr->weapon_action_state = GUN_ANIM_STATE_THROWKNIFE_RECOVER;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_THROWKNIFE_RECOVER)
    {
        tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
        if (gunSample1PTransform(throwKnifeReleaseKeyframes, tempf, &handptr->field_8EC, hand) != 0)
        {
            handptr->field_92C = 1;
        }
        else
        {
            handptr->field_87E = 1;
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_MINE_PLACE)
    {
        if ((handptr->weapon_ammo_in_magazine > 0) || (bondwalkItemCheckBitflags(var_s1, WEAPONSTATBITFLAG_CLICKY) != 0))
        {
            tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
            
            if (gunSample1PTransform(proxMineThrowKeyframes, tempf, &handptr->field_8EC, hand) != 0)
            {
                handptr->field_92C = 1;
            }
            else
            {
                handptr->field_87E = 0;
                handptr->weapon_firing_status = 1;
                handptr->weapon_ammo_in_magazine -= 1;
                handptr->weapon_action_state = GUN_ANIM_STATE_MINE_RECOVER;
                handptr->field_890 = 0.0f;
                handptr->field_88C = 0;
            }
        }
        else
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_MINE_RECOVER)
    {
        tempf = WEAPON_1P_ANIM_TIME(handptr->field_890);
        if (gunSample1PTransform(remoteMineThrowKeyframes, tempf, &handptr->field_8EC, hand) != 0)
        {
            handptr->field_92C = 1;
        }
        else
        {
            handptr->field_87E = 1;
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }

    if (handptr->weapon_action_state == GUN_ANIM_STATE_USE_ITEM)
    {
        if (var_s1 == ITEM_KEYANALYSERCASE)
        {
            if (handptr->field_88C == 0)
            {
                analyzeGEKey();
            }
        }
        else if (var_s1 == ITEM_WEAPONCASE)
        {
            if (handptr->field_88C == 0)
            {
                give_weapon_case_items();
            }
        }
        else if ((var_s1 == ITEM_BOMBDEFUSER)
            || (var_s1 == ITEM_DATATHIEF)
            || (var_s1 == ITEM_DOORDECODER)
            || (var_s1 == ITEM_EXPLOSIVEFLOPPY)
            || (var_s1 == ITEM_DATTAPE))
        {
            if (handptr->field_88C == 0)
            {
                temp_v0_8 = propFindForInteract();

                if (temp_v0_8 != NULL)
                {
                    temp_v0_8->obj->state |= 0x40;
                }
            }
        }
        else if ((var_s1 != ITEM_POLARIZEDGLASSES)
            && (var_s1 != ITEM_DARKGLASSES)
            && (var_s1 != ITEM_WATCHGEIGERCOUNTER)
            && (var_s1 != ITEM_WATCHMAGNETREPEL)
            && (var_s1 != ITEM_KEYCARD)
            && (var_s1 != ITEM_KEYYALE)
            && (var_s1 != ITEM_KEYBOLT)
            && (var_s1 != ITEM_SAFECRACKERCASE)
            && (var_s1 != ITEM_LOCKEXPLODER)
            && (var_s1 != ITEM_DOOREXPLODER)
            && (var_s1 != ITEM_CREDITCARD)
            && (var_s1 != ITEM_GASKEYRING)
            && (var_s1 == ITEM_BUNGEE))
        {
            // removed
        }
        else if (var_s1 == ITEM_PITONGUN
            || var_s1 == ITEM_GASKEYRING
            || var_s1 == ITEM_BUNGEE)
        {
            // removed
        }

        if (handptr->field_888 != 0)
        {
            handptr->weapon_action_state = GUN_ANIM_STATE_IDLE;
            handptr->field_890 = 0.0f;
            handptr->field_88C = 0;
        }
    }
}

#undef WEAPON_1P_ANIM_TIME
#undef WHEN_1_CASE_GRENADELAUNCH_FLD890
#undef WHEN_1_CASE_GRENADE_FLD890
#undef WHEN_D_FLD890
#undef WHEN_5_SP188_INIT
#undef WHEN_5_SP188_MULTI
#undef WHEN_5_FLD8B0_SP
#undef WHEN_5_FLD8B0_MULTI
#undef WHEN_8_SP178_INIT
#undef WHEN_8_SP178_MULTI
#undef WHEN_A_FLD890
#undef WHEN_A_FLD8B0
#undef WHEN_C_FLD890
#undef WHEN_E_FLD890
#undef WHEN_10_FLD890
#undef WHEN_11_FLD890_1
#undef WHEN_11_FLD890_2
#undef WHEN_1E_FLD890


void analyzeGEKey(void)
{
    if (bondinvHasGEKey())
    {
   	    HUDMESSAGEBOTTOM(langGet(getStringID(LGUN, GUN_STR_D8_ANALYZINGTHEGOLDENEYEKEY_LF))); //Analyzing the GoldenEye key...
    	g_CurrentPlayer->copiedgoldeneye = TRUE;
    	sndPlaySfx(g_musicSfxBufferPtr, KEY_ANALYSER_SFX, 0x0);
    	currentPlayerEquipWeaponWrapper(GUNRIGHT, ITEM_GOLDENEYEKEY);
    	currentPlayerEquipWeaponWrapper(GUNLEFT, ITEM_UNARMED);
  	}
  	else
  	{
	    HUDMESSAGEBOTTOM(langGet(getStringID(LGUN, GUN_STR_D9_YOUDONOTHAVETHEGOLDENEYEKEY_LF))); //You do not have the GoldenEye key.
	    sub_GAME_7F05D690();
  	}
  	return;
}


s32 get_keyanalyzer_flag(void)
{
  return g_CurrentPlayer->copiedgoldeneye;
}


void give_weapon_case_items(void)
{
  add_ammo_to_inventory(AMMO_KNIFE, 2, 0, 1);
  add_ammo_to_inventory(AMMO_GRENADE, 2, 0, 1);
  bondinvAddInvItem(ITEM_SNIPERRIFLE);
  set_sound_effect_for_weapontype_collection(ITEM_SNIPERRIFLE);
  display_text_for_weapon_in_lower_left_corner(ITEM_SNIPERRIFLE);
  give_cur_player_ammo(sniperrifle_stats.AmmoType, check_cur_player_ammo_amount_in_inventory(sniperrifle_stats.AmmoType) + sniperrifle_stats.MagSize);
  bondinvRemoveItemByID(ITEM_WEAPONCASE);
  currentPlayerEquipWeaponWrapper(GUNRIGHT,ITEM_SNIPERRIFLE);
  currentPlayerEquipWeaponWrapper(GUNLEFT,ITEM_UNARMED);
}


f32 get_vertical_position_solo_watch_menu_main_page_for_item(ITEM_IDS item)
{
  return gitem_structs[item].watch_pos_x;
}


f32 get_lateral_position_solo_watch_menu_main_page_for_item(ITEM_IDS item)
{
  return gitem_structs[item].watch_pos_y;
}


f32 get_depth_on_solo_watch_menu_page_for_item(ITEM_IDS item)
{
  return gitem_structs[item].watch_pos_z;
}


f32 get_xrotation_solo_watch_menu_for_item(ITEM_IDS item)

{
  return gitem_structs[item].x_rotation;
}


f32 get_yrotation_solo_watch_menu_for_item(ITEM_IDS item)
{
  return gitem_structs[item].y_rotation;
}


f32 get_45_degree_angle(s32 unk) {
  return 45.0f;
}


u16 *get_ptr_first_title_line_item(ITEM_IDS item)
{
  return langGet(gitem_structs[item].upper_watch_text);
}


u16 *get_ptr_second_title_line_item(ITEM_IDS item)
{
    return langGet(gitem_structs[item].lower_watch_text);
}


u16 *get_ptr_short_watch_text_for_item(ITEM_IDS item)
{
    return langGet(gitem_structs[item].watch_equipment_text);
}


u16 *get_ptr_long_watch_text_for_item(ITEM_IDS item)
{
    return langGet(gitem_structs[item].weapon_of_choice_text);
}


f32 get_45_degree_angle_0(s32 unk)
{
	return 45.0f;
}


f32 get_horizontal_offset_on_solo_watch_menu_for_item(ITEM_IDS item)
{
  return gitem_structs[item].equip_watch_x;
}


f32 get_vertical_offset_on_solo_watch_menu_for_item(ITEM_IDS item)
{
  return gitem_structs[item].equip_watch_y;
}


f32 get_depth_offset_solo_watch_menu_inventory_page_for_item(ITEM_IDS item)
{
  return gitem_structs[item].equip_watch_z;
}


f32 getCurrentPlayerNoise(GUNHAND hand)
{
    return g_CurrentPlayer->hands[hand].noise;
}


void gunTickNoise(void)
{
    enum ITEM_IDS weapon_id_right;
    enum ITEM_IDS weapon_id_left;
    s32 unused2;
    f32 noise_reduction;
    WeaponStats *item_right_stats;
    WeaponStats *item_left_stats;
    f32 noise_reduction_max;
    s32 unused;

    weapon_id_right = getCurrentPlayerWeaponId(GUNRIGHT);
    weapon_id_left = getCurrentPlayerWeaponId(GUNLEFT);
    item_right_stats = get_ptr_item_statistics(weapon_id_right);
    item_left_stats = get_ptr_item_statistics(weapon_id_left);

    if (weapon_id_right != ITEM_UNARMED && get_hands_firing_status(GUNRIGHT))
    {
        g_CurrentPlayer->hands[GUNRIGHT].noise += item_right_stats->NoiseIncreasePerShot;

        if (item_right_stats->LoudnessMax < g_CurrentPlayer->hands[GUNRIGHT].noise)
        {
            g_CurrentPlayer->hands[GUNRIGHT].noise = item_right_stats->LoudnessMax;
        }
    }

    if (weapon_id_left != ITEM_UNARMED && get_hands_firing_status(GUNLEFT))
    {
        g_CurrentPlayer->hands[GUNLEFT].noise += item_left_stats->NoiseIncreasePerShot;

        if (item_left_stats->LoudnessMax < g_CurrentPlayer->hands[GUNLEFT].noise)
        {
            g_CurrentPlayer->hands[GUNLEFT].noise = item_left_stats->LoudnessMax;
        }
    }

    noise_reduction = (item_right_stats->NoiseIncreasePerShot * g_GlobalTimerDelta) / (item_right_stats->NoiseDecayLinearTime * 60.0f);
    noise_reduction_max = ((g_CurrentPlayer->hands[GUNRIGHT].noise - item_right_stats->LoudnessMin) * g_GlobalTimerDelta) / (item_right_stats->NoiseDecayScaledTime * 60.0f);

    if (noise_reduction < noise_reduction_max)
    {
        noise_reduction = noise_reduction_max;
    }

    g_CurrentPlayer->hands[GUNRIGHT].noise -= noise_reduction;

    if (g_CurrentPlayer->hands[GUNRIGHT].noise < item_right_stats->LoudnessMin)
    {
        g_CurrentPlayer->hands[GUNRIGHT].noise = item_right_stats->LoudnessMin;
    }

    noise_reduction = (item_left_stats->NoiseIncreasePerShot * g_GlobalTimerDelta) / (item_left_stats->NoiseDecayLinearTime * 60.0f);
    noise_reduction_max = ((g_CurrentPlayer->hands[GUNLEFT].noise - item_left_stats->LoudnessMin) * g_GlobalTimerDelta) / (item_left_stats->NoiseDecayScaledTime * 60.0f);

    if (noise_reduction < noise_reduction_max)
    {
        noise_reduction = noise_reduction_max;
    }

    g_CurrentPlayer->hands[GUNLEFT].noise -= noise_reduction;

    if (g_CurrentPlayer->hands[GUNLEFT].noise < item_left_stats->LoudnessMin)
    {
        g_CurrentPlayer->hands[GUNLEFT].noise = item_left_stats->LoudnessMin;
    }
}


/**
 * Returns true if the hand has a melee weapon or has ammo in the magazine.
 */
s32 gunCanUseWeapon(enum GUNHAND hand)
{
    return (get_ammo_type_for_weapon(getCurrentPlayerWeaponId(hand)) == 0)
        || (g_CurrentPlayer->hands[hand].weapon_ammo_in_magazine > 0);
}


/**
 * US address 7F067420.
 * Perfect Dark method bgunTickGameplay.
 *
 * Handles logic for single gun and dual wield trigger presses.
 * Calls updates to first person gun animations, gun model loading, noise to AI, and updating color from collision tiles.
 * Also handles the Watch Magnet Attract hum noise.
*/
void gunTickGameplay(s32 triggerOn)
{
    struct gun_trigger_state trigger_state;
    enum ITEM_IDS weapon_id_right;
    enum ITEM_IDS weapon_id_left;
    enum GUNHAND hand = GUNLEFT;
    struct rgba_u8 weapon_color;

    trigger_state = g_ZeroTriggerState;

    // Save previous trigger state.
    g_CurrentPlayer->prev_trigger_down = g_CurrentPlayer->trigger_down;

    // Save raw trigger state.
    g_CurrentPlayer->trigger_down = triggerOn;

    if ((g_CurrentPlayer->trigger_down == 0) && (g_CurrentPlayer->prev_trigger_down != 0))
    {
        g_CurrentPlayer->trigger_released = 1;
    }

    // Z button pressed this frame.
    if (g_CurrentPlayer->trigger_down != 0)
    {
        weapon_id_right = getCurrentPlayerWeaponId(GUNRIGHT);
        weapon_id_left = getCurrentPlayerWeaponId(GUNLEFT);

        g_CurrentPlayer->z_trigger_timer += g_ClockTimer;

        // Dual wielding.
        if ((weapon_id_right != ITEM_UNARMED) && (weapon_id_left != ITEM_UNARMED))
        {
            // Both guns prefer to take turns firing in dual wield.
            if ((bondwalkItemCheckBitflags(weapon_id_right, WEAPONSTATBITFLAG_DUAL_WIELD_ALTERNATING_FIRE) != 0) && (bondwalkItemCheckBitflags(weapon_id_left, WEAPONSTATBITFLAG_DUAL_WIELD_ALTERNATING_FIRE) != 0))
            {
                // Trigger has been held longer than 20 ticks on NTSC (24 on PAL).
                if (g_CurrentPlayer->z_trigger_timer > DUAL_WIELD_TRIGGER_SWAP_TICKS)
                {
                    // 'hand' still has its default value here, which behaves like trigger-on.
                    trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = hand;

                    // If the gun in the other hand is usable or has been held for any amount of time, depress its trigger as well.
                    if (gunCanUseWeapon(1 - g_CurrentPlayer->current_trigger_hand) || g_CurrentPlayer->hands[1 - g_CurrentPlayer->current_trigger_hand].weapon_hold_time)
                    {
                        trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 1;
                    }
                }
                // Z has been held for less than or equal to 20 ticks on NTSC (24 on PAL).
                else
                {
                    if ((g_CurrentPlayer->prev_trigger_down == 0) &&
                        ((gunCanUseWeapon(1 - g_CurrentPlayer->current_trigger_hand) != 0) || (gunCanUseWeapon(g_CurrentPlayer->current_trigger_hand) == 0)))
                    {
                        g_CurrentPlayer->current_trigger_hand = 1 - g_CurrentPlayer->current_trigger_hand;
                    }

                    trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = 1;
                    trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 0;
                }
            }
            /**
             * One gun prefers to take turns firing in dual wield.
             * This doesn't happen much in the vanilla US version with the notable
             * exception of equipping Xenia's RC-P90 and Grenade Launcher in Jungle
            */
            else if ((bondwalkItemCheckBitflags(weapon_id_right, WEAPONSTATBITFLAG_DUAL_WIELD_ALTERNATING_FIRE) != 0) || (bondwalkItemCheckBitflags(weapon_id_left, WEAPONSTATBITFLAG_DUAL_WIELD_ALTERNATING_FIRE) != 0))
            {
                // Z has been held more than 30 ticks on NTSC (36 for PAL), depress trigger on both guns.
                if (g_CurrentPlayer->z_trigger_timer > DUAL_WIELD_SINGLE_TRIGGER_SWAP_TICKS)
                {
                    trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = hand;

                    if ((gunCanUseWeapon(1 - g_CurrentPlayer->current_trigger_hand) != 0) || g_CurrentPlayer->hands[1 - g_CurrentPlayer->current_trigger_hand].weapon_hold_time != 0)
                    {
                        trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 1;
                    }
                }
                // Before the hold threshold (30 ticks NTSC or 36 PAL), prefer the hand whose weapon uses alternating dual wield fire.
                else
                {
                    hand = bondwalkItemCheckBitflags(weapon_id_right, WEAPONSTATBITFLAG_DUAL_WIELD_ALTERNATING_FIRE) ? GUNRIGHT : GUNLEFT;

                    if (gunCanUseWeapon(hand) != 0 || g_CurrentPlayer->hands[hand].weapon_hold_time != 0)
                    {
                        g_CurrentPlayer->current_trigger_hand = hand;
                    }
                    else
                    {
                        if ((gunCanUseWeapon(1 - hand) != 0) || g_CurrentPlayer->hands[1 - hand].weapon_hold_time != 0)
                        {
                            g_CurrentPlayer->current_trigger_hand = 1 - hand;
                        }
                        else
                        {
                            g_CurrentPlayer->current_trigger_hand = 1 - g_CurrentPlayer->current_trigger_hand;
                        }
                    }

                    trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = 1;
                    trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 0;
                }
            }
            /**
             * Neither weapon uses alternating dual wield fire.
             * Once the hold threshold is exceeded, allow the off-hand to become active too.
             */
            else if (g_CurrentPlayer->z_trigger_timer > DUAL_WIELD_SINGLE_TRIGGER_SWAP_TICKS)
            {
                trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = hand;

                if (gunCanUseWeapon(1 - g_CurrentPlayer->current_trigger_hand) || g_CurrentPlayer->hands[1 - g_CurrentPlayer->current_trigger_hand].weapon_hold_time)
                {
                    trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 1;
                }
            }
            /**
             * Neither weapon uses alternating dual wield fire.
             * On a fresh Z press, switch lead hands if the other hand is usable or the current hand cannot be used.
             * The lead hand continues being the lead hand.
             */
            else
            {
                if ((g_CurrentPlayer->prev_trigger_down == 0) &&
                    ((gunCanUseWeapon(1 - g_CurrentPlayer->current_trigger_hand) != 0) || (gunCanUseWeapon(g_CurrentPlayer->current_trigger_hand) == 0)))
                {
                    g_CurrentPlayer->current_trigger_hand = 1 - g_CurrentPlayer->current_trigger_hand;
                }

                trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = 1;
                trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 0;
            }
        }
        // Not dual wielding.
        else
        {
            if ((getCurrentPlayerWeaponId(g_CurrentPlayer->current_trigger_hand) == ITEM_UNARMED) && (getCurrentPlayerWeaponId(1 - g_CurrentPlayer->current_trigger_hand) != ITEM_UNARMED))
            {
                g_CurrentPlayer->current_trigger_hand = 1 - g_CurrentPlayer->current_trigger_hand;
            }

            trigger_state.triggerOn[g_CurrentPlayer->current_trigger_hand] = 1;
            trigger_state.triggerOn[1 - g_CurrentPlayer->current_trigger_hand] = 0;
        }
    }
    // Z button not pressed. Reset the trigger timer.
    else
    {
        g_CurrentPlayer->z_trigger_timer = 0;
    }

    gunTickHandState(0, trigger_state.triggerOn[0]); // Right hand
    gunTickHandState(1, trigger_state.triggerOn[1]); // Left hand
    used_to_load_1st_person_model_on_demand(0);
    used_to_load_1st_person_model_on_demand(1);
    gunTickNoise();

    if (g_CurrentPlayer->resetshadecol)
    {
        set_color_shading_from_tile(getCurrentPlayerProp(), (struct rgba_u8 *) &g_CurrentPlayer->tileColor);
        g_CurrentPlayer->resetshadecol = FALSE;
    }
    else
    {
        set_color_shading_from_tile(getCurrentPlayerProp(), &weapon_color);
        update_color_shading(&g_CurrentPlayer->tileColor, &weapon_color);
    }

    bondinvIncrementHeldTime(getCurrentPlayerWeaponId(GUNRIGHT), getCurrentPlayerWeaponId(GUNLEFT));

    if(1);

    if (g_CurrentPlayer->magnetattracttime >= 0)
    {
        struct hand *hand_right = &g_CurrentPlayer->hands[0];

        g_CurrentPlayer->magnetattracttime += g_ClockTimer;

        if (g_CurrentPlayer->magnetattracttime < WATCH_SOUND_DURATION_TICKS)
        {
            // Start or restart the hum sound if needed
            if (hand_right->audioHandle == NULL
                || sndGetPlayingState((struct ALSoundState *) hand_right->audioHandle) == 0)
            {
                if (lvlGetControlsLockedFlag() == 0)
                {
                    sndPlaySfx(
                        (struct ALBankAlt_s *) g_musicSfxBufferPtr,
                        MAGNETIC_HUM_SFX,
                        (struct ALSoundState *) &hand_right->audioHandle);
                }
            }
        }
        else
        {
            g_CurrentPlayer->magnetattracttime = -1;

            if (hand_right->audioHandle != NULL)
            {
                if (sndGetPlayingState((struct ALSoundState *) hand_right->audioHandle) != 0)
                {
                    sndDeactivate((struct ALSoundState *) hand_right->audioHandle);
                }
            }
        }
    }
}


void gunSetAimType(s32 param_1)
{
  g_CurrentPlayer->aimtype = param_1;
}


void sub_GAME_7F067AB4(coord3d *param_1)
{
  g_CurrentPlayer->hands[GUNRIGHT].field_A38 = sub_GAME_7F05DCB8(GUNRIGHT) + param_1->x;
  g_CurrentPlayer->hands[GUNRIGHT].field_A3C = param_1->y;
  g_CurrentPlayer->hands[GUNRIGHT].field_A40 = param_1->z;

  g_CurrentPlayer->hands[GUNLEFT].field_A38 = sub_GAME_7F05DCB8(GUNLEFT) + param_1->x;
  g_CurrentPlayer->hands[GUNLEFT].field_A3C = param_1->y;
  g_CurrentPlayer->hands[GUNLEFT].field_A40 = param_1->z;

}


void gunSetTracerTarget(coord3d* pos)
{
    g_CurrentPlayer->hands[GUNLEFT].item_related.x = g_CurrentPlayer->hands[GUNRIGHT].item_related.x = pos->x;
    g_CurrentPlayer->hands[GUNLEFT].item_related.y = g_CurrentPlayer->hands[GUNRIGHT].item_related.y = pos->y;
    g_CurrentPlayer->hands[GUNLEFT].item_related.z = g_CurrentPlayer->hands[GUNRIGHT].item_related.z = pos->z;
}


void caclulate_gun_crosshair_position_rotation(f32 turn_x, f32 turn_y, f32 guncrossdamp, f32 gunaimdamp)
{
    s32 i;
    f32 screen_width;
    f32 screen_height;
    coord3d coords;

    screen_width = getPlayer_c_screenwidth();
    screen_height = getPlayer_c_screenheight();

    if (guncrossdamp != g_CurrentPlayer->guncrossdamp)
    {
        g_CurrentPlayer->crosshair_x_pos = (g_CurrentPlayer->crosshair_x_pos * (1.0f - g_CurrentPlayer->guncrossdamp)) / (1.0f - guncrossdamp);
        g_CurrentPlayer->crosshair_y_pos = (g_CurrentPlayer->crosshair_y_pos * (1.0f - g_CurrentPlayer->guncrossdamp)) / (1.0f - guncrossdamp);
        g_CurrentPlayer->guncrossdamp = guncrossdamp;
    }

    if (gunaimdamp != g_CurrentPlayer->gunaimdamp)
    {
        g_CurrentPlayer->gun_azimuth_angle = (g_CurrentPlayer->gun_azimuth_angle * (1.0f - g_CurrentPlayer->gunaimdamp)) / (1.0f - gunaimdamp);
        g_CurrentPlayer->gun_azimuth_turning = (g_CurrentPlayer->gun_azimuth_turning * (1.0f - g_CurrentPlayer->gunaimdamp)) / (1.0f - gunaimdamp);
        g_CurrentPlayer->gunaimdamp = gunaimdamp;
    }

    for (i = 0; i < g_ClockTimer; i++)
    {
        g_CurrentPlayer->crosshair_x_pos = (g_CurrentPlayer->crosshair_x_pos * guncrossdamp) + turn_x;
        g_CurrentPlayer->crosshair_y_pos = (g_CurrentPlayer->crosshair_y_pos * guncrossdamp) + turn_y;
    }

    g_CurrentPlayer->crosshair_angle.f[0] = (g_CurrentPlayer->crosshair_x_pos * (1.0f - guncrossdamp) * screen_width * 0.5f) + (screen_width * 0.5f);
    g_CurrentPlayer->crosshair_angle.f[1] = (g_CurrentPlayer->crosshair_y_pos * (1.0f - guncrossdamp) * screen_height * 0.5f) + (screen_height * 0.5f);

    if (g_CurrentPlayer->crosshair_angle.f[0] < 3.0f)
    {
        g_CurrentPlayer->crosshair_angle.f[0] = 3.0f;
    }
    else if ((screen_width - 4.0f) < g_CurrentPlayer->crosshair_angle.f[0])
    {
        g_CurrentPlayer->crosshair_angle.f[0] = screen_width - 4.0f;
    }

    if (g_CurrentPlayer->crosshair_angle.f[1] < 3.0f)
    {
        g_CurrentPlayer->crosshair_angle.f[1] = 3.0f;
    }
    else if ((screen_height - 4.0f) < g_CurrentPlayer->crosshair_angle.f[1])
    {
        g_CurrentPlayer->crosshair_angle.f[1] = (screen_height - 4.0f);
    }

    g_CurrentPlayer->crosshair_angle.f[0] += getPlayer_c_screenleft();
    g_CurrentPlayer->crosshair_angle.f[1] += getPlayer_c_screentop();

    for (i = 0; i < g_ClockTimer; i++)
    {
        g_CurrentPlayer->gun_azimuth_angle = (g_CurrentPlayer->gun_azimuth_angle * gunaimdamp) + turn_x;
        g_CurrentPlayer->gun_azimuth_turning = (g_CurrentPlayer->gun_azimuth_turning * gunaimdamp) + turn_y;
    }

    g_CurrentPlayer->field_FFC.x = (g_CurrentPlayer->gun_azimuth_angle * (1.0f - gunaimdamp) * screen_width * 0.5f) + (screen_width * 0.5f);
    g_CurrentPlayer->field_FFC.y = (g_CurrentPlayer->gun_azimuth_turning * (1.0f - gunaimdamp) * screen_height * 0.5f) + (screen_height * 0.5f);

    g_CurrentPlayer->field_FFC.x += getPlayer_c_screenleft();
    g_CurrentPlayer->field_FFC.y += getPlayer_c_screentop();

    transformAndNormalizeByLength2Dto3D(&g_CurrentPlayer->field_FFC, &coords, 1000.0f);
    sub_GAME_7F067AB4(&coords);
}


void sub_GAME_7F067F58(f32 turn_x, f32 turn_y, f32 max_aim_lock_speed)
{
    f32 aim_lock_speed;

#if defined(VERSION_US) || defined(VERSION_JP)
    aim_lock_speed = get_ptr_item_statistics(getCurrentPlayerWeaponId(GUNRIGHT))->AimLockSpeed;
#elif defined(VERSION_EU)
    aim_lock_speed = get_ptr_item_statistics(getCurrentPlayerWeaponId(GUNRIGHT))->CrosshairSpeed;
#endif

    if (aim_lock_speed < max_aim_lock_speed)
    {
        aim_lock_speed = max_aim_lock_speed;
    }

    caclulate_gun_crosshair_position_rotation(turn_x, turn_y, max_aim_lock_speed, aim_lock_speed);
}


void sub_GAME_7F067FBC(f32 turn_x, f32 turn_y)
{
    WeaponStats * item_stats;
    f32 guncrossdamp;
    f32 gunaimdamp;

    item_stats = get_ptr_item_statistics(getCurrentPlayerWeaponId(GUNRIGHT));

#if defined(VERSION_US)
    guncrossdamp = item_stats->CrosshairSpeed;
    gunaimdamp = item_stats->AimLockSpeed;
#elif defined(VERSION_EU)
    guncrossdamp = 0.7651f;
    gunaimdamp = item_stats->CrosshairSpeed;
#elif defined(VERSION_JP)
    guncrossdamp = 0.8f;
    gunaimdamp = item_stats->AimLockSpeed;
#endif

    caclulate_gun_crosshair_position_rotation(turn_x, turn_y, guncrossdamp, gunaimdamp);
}


/*
* Address: 0x7f068008
*/
void get_bullet_angle(f32* horizontal_angle, f32* vertical_angle) {
	*horizontal_angle = g_CurrentPlayer->crosshair_angle.f[0];
	*vertical_angle = g_CurrentPlayer->crosshair_angle.f[1];
}


void sub_GAME_7F06802C(void)
{
    coord3d coord;
    f32 tmp;

    tmp = getPlayer_c_screenleft() + (getPlayer_c_screenwidth() * 0.5f);
    g_CurrentPlayer->crosshair_angle.x = tmp;
    g_CurrentPlayer->field_FFC.x = tmp;

    tmp = getPlayer_c_screentop() + (getPlayer_c_screenheight() * 0.5f);
    g_CurrentPlayer->crosshair_angle.y = tmp;
    g_CurrentPlayer->field_FFC.y = tmp;

    transformAndNormalizeByLength2Dto3D((coord2d *) &g_CurrentPlayer->field_FFC, &coord, 1000.0f);
    sub_GAME_7F067AB4(&coord);
}


void sub_GAME_7F0680D4(coord3d * coord)
{
    coord3d tmp;

    g_CurrentPlayer->field_1010.x = coord->x;
    g_CurrentPlayer->field_1010.y = coord->y;
    g_CurrentPlayer->field_1010.z = coord->z;
    matrix_4x4_set_rotation_around_xyz(coord->f, &g_CurrentPlayer->field_101C);

    tmp.x = g_CurrentPlayer->field_101C.m[2][0] * 1000.0f;
    tmp.y = g_CurrentPlayer->field_101C.m[2][1] * 1000.0f;
    tmp.z = g_CurrentPlayer->field_101C.m[2][2] * 1000.0f;
    transform3Dto2DCoords(&tmp, (coord3d* ) &g_CurrentPlayer->crosshair_angle);

    g_CurrentPlayer->field_FFC.x = g_CurrentPlayer->crosshair_angle.x;
    g_CurrentPlayer->field_FFC.y = g_CurrentPlayer->crosshair_angle.y;

    sub_GAME_7F067AB4(&tmp);
}

extern const f32 g_GunScreenAspectRatio;


/**
 * Address 0x7F068190.
*/
#if !defined(VERSION_US)
void sub_GAME_7F068190(coord3d *zeropos, coord3d *vec)
{
    zeropos->x = 0.0f;
    zeropos->y = 0.0f;
    zeropos->z = 0.0f;

    transformAndNormalizeByLength2Dto3D(&g_CurrentPlayer->crosshair_angle, vec, 1.0f);
}


/*
* Address: 0x7f0681cc
* This function computes the angle the player's bullets are fired at
*/
void bullet_path_from_screen_center(coord3d* arg0, coord3d* result, enum GUNHAND arg2)
{
    coord2d crosspos;
    s32 unused;
    f32 inaccuracy;
    f32 scaledspread;
    f32 randfactor;

    inaccuracy = get_ptr_item_statistics(getCurrentPlayerWeaponId(arg2))->Inaccuracy;
    if ((bondwalkItemCheckBitflags(get_item_in_hand_or_watch_menu(arg2), WEAPONSTATBITFLAG_FIRST_SHOT_ACCURACY) != 0) && (g_CurrentPlayer->hands[arg2].volley == 1))
    {
        // Single shots are four times more accurate
        inaccuracy *= 0.25f;
    }

    scaledspread = (120.0f * inaccuracy) / viGetFovY();

    randfactor = (RANDOMFRAC() - 0.5f) * RANDOMFRAC();
    crosspos.x = g_CurrentPlayer->crosshair_angle.f[0] + randfactor * scaledspread * getPlayer_c_screenwidth() * g_GunScreenAspectRatio
        / (getPlayer_c_perspaspect() * 320.0f);

    randfactor = (RANDOMFRAC() - 0.5f) * RANDOMFRAC();
    crosspos.y =  g_CurrentPlayer->crosshair_angle.f[1] + randfactor * scaledspread * getPlayer_c_screenheight()
        / (PAL ? (f32)(SCREEN_HEIGHT_272) : (f32)(SCREEN_HEIGHT_240));

    arg0->x = 0.0f;
    arg0->y = 0.0f;
    arg0->z = 0.0f;

    // Result is a normalized vector describing the path the bullet will follow
    // Can be used to compute x,y,z displacement off the center of the screen if done for a projectile
    transformAndNormalizeByLength2Dto3D(&crosspos, result, 1.0f);
}


/*
* Address: 0x7f068420
*/
CasingRecord* casingCreate(ModelFileHeader* header, Mtxf* mtx)
{
    CasingRecord* entry = g_Casings;
    CasingRecord* end = g_Casings + ARRAYCOUNT(g_Casings);

    while (entry < end && entry->header != NULL)
    {
        entry++;
    }

    if (entry < end)
    {
        entry->header = header;

        entry->pos.x = mtx->m[3][0];
        entry->pos.y = mtx->m[3][1];
        entry->pos.z = mtx->m[3][2];
#if VERSION_EU
        matrix_7f05842c_eu(mtx, entry->rot_mtx);
#else
        entry->rot_mtx.m[0][0] = mtx->m[0][0];
        entry->rot_mtx.m[0][1] = mtx->m[0][1];
        entry->rot_mtx.m[0][2] = mtx->m[0][2];
        entry->rot_mtx.m[0][3] = 0.0f;

        entry->rot_mtx.m[1][0] = mtx->m[1][0];
        entry->rot_mtx.m[1][1] = mtx->m[1][1];
        entry->rot_mtx.m[1][2] = mtx->m[1][2];
        entry->rot_mtx.m[1][3] = 0.0f;

        entry->rot_mtx.m[2][0] = mtx->m[2][0];
        entry->rot_mtx.m[2][1] = mtx->m[2][1];
        entry->rot_mtx.m[2][2] = mtx->m[2][2];
        entry->rot_mtx.m[2][3] = 0.0f;

        entry->rot_mtx.m[3][0] = 0.0f;
        entry->rot_mtx.m[3][1] = 0.0f;
        entry->rot_mtx.m[3][2] = 0.0f;
        entry->rot_mtx.m[3][3] = 1.0f;
#endif
        return entry;
    }

    return NULL;
}
#endif


#if VERSION_EU
#define THROWMTX_OFFSET      0xAD0
#define THROWPOS_OFFSET      0xB00
#define THROWPOS_PREV_OFFSET 0xB40
#else
#define THROWMTX_OFFSET      0xAD8
#define THROWPOS_OFFSET      0xB08
#define THROWPOS_PREV_OFFSET 0xB48
#endif
 
#define THROWMTX     ((Mtxf *) ((u8 *) g_CurrentPlayer + handoffset + THROWMTX_OFFSET))
#define THROWPOS(k)  (((f32 *) ((u8 *) g_CurrentPlayer + handoffset + THROWPOS_OFFSET))[k])
#define THROWPREV(k) (((f32 *) ((u8 *) g_CurrentPlayer + handoffset + THROWPOS_PREV_OFFSET))[k])
 
extern const f32 g_CasingSwitchScale;
extern const f32 g_PistolCasingHorizontalSpeed;
extern const f32 g_PistolCasingRotationScaleX;
extern const f32 g_PistolCasingRotationOffsetX;
extern const f32 g_PistolCasingRotationScaleY;
extern const f32 g_PistolCasingRotationOffsetY;
extern const f32 g_PistolCasingRotationScaleZ;
extern const f32 g_PistolCasingRotationOffsetZ;
extern const f32 g_PistolCasingRandomDivisor;
extern const f32 g_PistolCasingGravity;
extern const f32 g_RifleCasingHorizontalSpeed;
extern const f32 g_RifleCasingVerticalSpeed;
extern const f32 g_RifleCasingRotationScaleX;
extern const f32 g_RifleCasingRotationOffsetX;
extern const f32 g_RifleCasingRotationScaleY;
extern const f32 g_RifleCasingRotationOffsetY;
extern const f32 g_RifleCasingRotationScaleZ;
extern const f32 g_RifleCasingRotationOffsetZ;
extern const f32 g_RifleCasingRandomDivisor;
extern const f32 g_RifleCasingGravity;
 
 
/**
 * Address: 7F068508
 * 
 * Ejects a spent cartridge casing from the gun in the given hand.
 */
#if defined(VERSION_EU)
void sub_GAME_7F068508(GUNHAND handnum, f32 floor_y_pos)
{
    CasingRecord *casing;
    Mtxf mtx;
    ITEM_IDS weaponid;
    coord3d *switchdata;
    ModelFileHeader *cartridge_header;
    coord3d switchpos;
    ModelNode *switch0;
    coord3d rot;
#if VERSION_EU
    Mtxf rotmtx;
#endif
    f32 rand;
    s32 new_var; /* dead but declared on EU — still reserves its frame slot */
    f32 frac;
#if VERSION_EU
    s32 randlimit;
#endif
    f32 oldvely;
    f32 newvely;
#ifndef VERSION_EU
    s32 randlimit;
#endif
    s32 handoffset;
    u32 randval;
#if VERSION_EU
    s32 pad[2];
#endif
 
    weaponid = getCurrentPlayerWeaponId(handnum);
    cartridge_header = get_ptr_item_statistics(weaponid)->ptr_cartridge_struct;
 
    // Do not create ejected casings in multiplayer.
    if ((cartridge_header == NULL) || (getPlayerCount() >= 2))
    {
        return;
    }
 
    handoffset = handnum * sizeof(struct hand);
    switch0 = g_CurrentPlayer->copy_of_body_obj_header[handnum].Switches[0];
 
    if (switch0 != NULL)
    {
        switchdata = (coord3d *) switch0->Data;
 
        switchpos.x = switchdata->x * g_CasingSwitchScale;
        switchpos.y = switchdata->y * g_CasingSwitchScale;
        switchpos.z = switchdata->z * g_CasingSwitchScale;
 
        matrix_4x4_set_identity_and_position(&switchpos, &mtx);
        matrix_4x4_multiply_in_place(THROWMTX, &mtx);
    }
    else
    {
        matrix_4x4_copy(THROWMTX, &mtx);
    }
 
    casing = casingCreate(cartridge_header, &mtx);
 
    if (casing == NULL)
    {
        return;
    }
 
    rot = *((coord3d *) (&D_80035EA4));
    casing->floor_y_pos = floor_y_pos;
 
    if (((((weaponid == ITEM_WPPK) || (weaponid == ITEM_WPPKSIL)) || (weaponid == ITEM_TT33)) || (weaponid == ITEM_SILVERWPPK)) || (weaponid == ITEM_GOLDWPPK))
    {
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        newvely = 0.0625f;
        casing->vel.x = -(((rand * g_PistolCasingHorizontalSpeed) * newvely) + g_PistolCasingHorizontalSpeed);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.y = ((rand * 2.5f) * 0.0625f) + 2.5f;
        casing->vel.z = frac * 0.0f;
 
        mtx4RotateVecInPlace(THROWMTX, &casing->vel);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.x = (((rand + rand) * g_PistolCasingRotationScaleX) * newvely) - g_PistolCasingRotationOffsetX;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.y = (((rand + rand) * g_PistolCasingRotationScaleY) * newvely) - g_PistolCasingRotationOffsetY;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.z = (((rand + rand) * g_PistolCasingRotationScaleZ) * newvely) - g_PistolCasingRotationOffsetZ;
 
#if VERSION_EU
        matrix_4x4_set_rotation_around_xyz(&rot, &rotmtx);
        matrix_7f05842c_eu(&rotmtx, casing->rot_velocity_mtx);
        if (handoffset);
#else
        matrix_4x4_set_rotation_around_xyz(&rot, &casing->rot_velocity_mtx);
#endif
 
#if VERSION_EU
        randlimit = ((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10;
        randlimit = randlimit + 0x158679;
        randval = randomGetNext();
        oldvely = casing->vel.y;
        frac = ((f32) ((u32) (randval % randlimit))) / g_PistolCasingRandomDivisor;
#else
        randlimit = (((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10) + 0x158679;
        new_var = randlimit;
        randval = randomGetNext();
        oldvely = casing->vel.y;
        frac = ((f32) ((u32) (randval % new_var))) / g_PistolCasingRandomDivisor;
#endif
        newvely = oldvely - (frac * g_PistolCasingGravity);
 
        casing->vel.y = newvely;
        casing->pos.y += (frac * (oldvely + newvely)) * 0.5f;
        casing->pos.x += frac * casing->vel.x;
        casing->pos.z += frac * casing->vel.z;
 
        // Keep the 0 + 1 for matching.
        if (g_ClockTimer >= (0 + 1))
        {
            casing->vel.x += (THROWPOS(0) - THROWPREV(0)) / g_GlobalTimerDelta;
            casing->vel.y += (THROWPOS(1) - THROWPREV(1)) / g_GlobalTimerDelta;
            casing->vel.z += (THROWPOS(2) - THROWPREV(2)) / g_GlobalTimerDelta;
        }
    }
    else
    {
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.x = -(((rand * g_RifleCasingHorizontalSpeed) * 0.125f) + g_RifleCasingHorizontalSpeed);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.y = ((rand * g_RifleCasingVerticalSpeed) * 0.125f) + g_RifleCasingVerticalSpeed;
        casing->vel.z = 0.0f;
 
        mtx4RotateVecInPlace(THROWMTX, &casing->vel);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.x = (((rand + rand) * g_RifleCasingRotationScaleX) * 0.0625f) - g_RifleCasingRotationOffsetX;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.y = (((rand + rand) * g_RifleCasingRotationScaleY) * 0.0625f) - g_RifleCasingRotationOffsetY;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.z = (((rand + rand) * g_RifleCasingRotationScaleZ) * 0.0625f) - g_RifleCasingRotationOffsetZ;
 
#if VERSION_EU
        matrix_4x4_set_rotation_around_xyz(&rot, &rotmtx);
        matrix_7f05842c_eu(&rotmtx, casing->rot_velocity_mtx);
        if (handoffset);
#else
        matrix_4x4_set_rotation_around_xyz(&rot, &casing->rot_velocity_mtx);
#endif
        randval = ((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10;
#if VERSION_EU
        randval = randval + 0x158679;
        randlimit = randomGetNext();
        oldvely = (&casing->vel)->y;
        frac = ((f32) ((u32) (randlimit % randval))) / g_RifleCasingRandomDivisor;
#else
        randlimit = randval + 0x158679;
        randval = randomGetNext();
        oldvely = (&casing->vel)->y;
        frac = ((f32) ((u32) (randval % randlimit))) / g_RifleCasingRandomDivisor;
#endif
        newvely = (casing->vel.y = oldvely - (frac * g_RifleCasingGravity));
 
        casing->pos.y += (frac * (oldvely + newvely)) * 0.5f;
        casing->pos.x += frac * casing->vel.x;
        casing->pos.z += frac * casing->vel.z;
 
        if (g_ClockTimer > 0)
        {
            casing->vel.x += (THROWPOS(0) - THROWPREV(0)) / g_GlobalTimerDelta;
            casing->vel.y += (THROWPOS(1) - THROWPREV(1)) / g_GlobalTimerDelta;
            casing->vel.z += (THROWPOS(2) - THROWPREV(2)) / g_GlobalTimerDelta;
        }
    }
 
    if (handoffset);
}
#endif



/* ===== merged from gun2.c ===== */

#if VERSION_EU
extern const f32 g_GunSightAspectRatio;
#endif
extern ALSoundState *g_CasingSfxState;
extern u32 g_DefaultCasingModelRenderData[];

#define AMMO_RELATED_MAX 30
extern AmmoStats ammo_related[AMMO_RELATED_MAX];
extern const char g_GunHudIntegerFormat[];
extern const char aSD[];
extern const char g_GunDeathCountFormat[];
extern const char aSD_0[];


#if defined(VERSION_US)
/**
 * Address 0x7F068190.
*/
void sub_GAME_7F068190(coord3d *zeropos, coord3d *vec)
{
    zeropos->x = 0.0f;
    zeropos->y = 0.0f;
    zeropos->z = 0.0f;

    transformAndNormalizeByLength2Dto3D(&g_CurrentPlayer->crosshair_angle, vec, 1.0f);
}


/*
* Address: 0x7f0681cc
* This function computes the angle the player's bullets are fired at
*/
void bullet_path_from_screen_center(coord3d* arg0, coord3d* result, enum GUNHAND arg2)
{
    coord2d crosspos;
    s32 unused;
    f32 inaccuracy;
    f32 scaledspread;
    f32 randfactor;

    inaccuracy = get_ptr_item_statistics(getCurrentPlayerWeaponId(arg2))->Inaccuracy;
    if ((bondwalkItemCheckBitflags(get_item_in_hand_or_watch_menu(arg2), WEAPONSTATBITFLAG_FIRST_SHOT_ACCURACY) != 0) && (g_CurrentPlayer->hands[arg2].volley == 1))
    {
        // Single shots are four times more accurate
        inaccuracy *= 0.25f;
    }

    scaledspread = (120.0f * inaccuracy) / viGetFovY();

    randfactor = (RANDOMFRAC() - 0.5f) * RANDOMFRAC();
    crosspos.x = g_CurrentPlayer->crosshair_angle.f[0] + randfactor * scaledspread * getPlayer_c_screenwidth() * g_GunScreenAspectRatio
        / (getPlayer_c_perspaspect() * 320.0f);

    randfactor = (RANDOMFRAC() - 0.5f) * RANDOMFRAC();
    crosspos.y =  g_CurrentPlayer->crosshair_angle.f[1] + randfactor * scaledspread * getPlayer_c_screenheight()
        / (PAL ? (f32)(SCREEN_HEIGHT_272) : (f32)(SCREEN_HEIGHT_240));

    arg0->x = 0.0f;
    arg0->y = 0.0f;
    arg0->z = 0.0f;

    // Result is a normalized vector describing the path the bullet will follow
    // Can be used to compute x,y,z displacement off the center of the screen if done for a projectile
    transformAndNormalizeByLength2Dto3D(&crosspos, result, 1.0f);
}


/*
* Address: 0x7f068420
*/
CasingRecord* casingCreate(ModelFileHeader* header, Mtxf* mtx)
{
    CasingRecord* entry = g_Casings;
    CasingRecord* end = g_Casings + ARRAYCOUNT(g_Casings);

    while (entry < end && entry->header != NULL)
    {
        entry++;
    }

    if (entry < end)
    {
        entry->header = header;

        entry->pos.x = mtx->m[3][0];
        entry->pos.y = mtx->m[3][1];
        entry->pos.z = mtx->m[3][2];
#if VERSION_EU
        matrix_7f05842c_eu(mtx, entry->rot_mtx);
#else
        entry->rot_mtx.m[0][0] = mtx->m[0][0];
        entry->rot_mtx.m[0][1] = mtx->m[0][1];
        entry->rot_mtx.m[0][2] = mtx->m[0][2];
        entry->rot_mtx.m[0][3] = 0.0f;

        entry->rot_mtx.m[1][0] = mtx->m[1][0];
        entry->rot_mtx.m[1][1] = mtx->m[1][1];
        entry->rot_mtx.m[1][2] = mtx->m[1][2];
        entry->rot_mtx.m[1][3] = 0.0f;

        entry->rot_mtx.m[2][0] = mtx->m[2][0];
        entry->rot_mtx.m[2][1] = mtx->m[2][1];
        entry->rot_mtx.m[2][2] = mtx->m[2][2];
        entry->rot_mtx.m[2][3] = 0.0f;

        entry->rot_mtx.m[3][0] = 0.0f;
        entry->rot_mtx.m[3][1] = 0.0f;
        entry->rot_mtx.m[3][2] = 0.0f;
        entry->rot_mtx.m[3][3] = 1.0f;
#endif
        return entry;
    }

    return NULL;
}
#endif


#if VERSION_EU
#else
#endif
 
 
 
 
/**
 * Address: 7F068508
 * 
 * Ejects a spent cartridge casing from the gun in the given hand.
 */
#if !defined(VERSION_EU)
void sub_GAME_7F068508(GUNHAND handnum, f32 floor_y_pos)
{
    CasingRecord *casing;
    Mtxf mtx;
    ITEM_IDS weaponid;
    coord3d *switchdata;
    ModelFileHeader *cartridge_header;
    coord3d switchpos;
    ModelNode *switch0;
    coord3d rot;
#if VERSION_EU
    Mtxf rotmtx;
#endif
    f32 rand;
    s32 new_var; /* dead but declared on EU — still reserves its frame slot */
    f32 frac;
#if VERSION_EU
    s32 randlimit;
#endif
    f32 oldvely;
    f32 newvely;
#ifndef VERSION_EU
    s32 randlimit;
#endif
    s32 handoffset;
    u32 randval;
#if VERSION_EU
    s32 pad[2];
#endif
 
    weaponid = getCurrentPlayerWeaponId(handnum);
    cartridge_header = get_ptr_item_statistics(weaponid)->ptr_cartridge_struct;
 
    // Do not create ejected casings in multiplayer.
    if ((cartridge_header == NULL) || (getPlayerCount() >= 2))
    {
        return;
    }
 
    handoffset = handnum * sizeof(struct hand);
    switch0 = g_CurrentPlayer->copy_of_body_obj_header[handnum].Switches[0];
 
    if (switch0 != NULL)
    {
        switchdata = (coord3d *) switch0->Data;
 
        switchpos.x = switchdata->x * g_CasingSwitchScale;
        switchpos.y = switchdata->y * g_CasingSwitchScale;
        switchpos.z = switchdata->z * g_CasingSwitchScale;
 
        matrix_4x4_set_identity_and_position(&switchpos, &mtx);
        matrix_4x4_multiply_in_place(THROWMTX, &mtx);
    }
    else
    {
        matrix_4x4_copy(THROWMTX, &mtx);
    }
 
    casing = casingCreate(cartridge_header, &mtx);
 
    if (casing == NULL)
    {
        return;
    }
 
    rot = *((coord3d *) (&D_80035EA4));
    casing->floor_y_pos = floor_y_pos;
 
    if (((((weaponid == ITEM_WPPK) || (weaponid == ITEM_WPPKSIL)) || (weaponid == ITEM_TT33)) || (weaponid == ITEM_SILVERWPPK)) || (weaponid == ITEM_GOLDWPPK))
    {
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        newvely = 0.0625f;
        casing->vel.x = -(((rand * g_PistolCasingHorizontalSpeed) * newvely) + g_PistolCasingHorizontalSpeed);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.y = ((rand * 2.5f) * 0.0625f) + 2.5f;
        casing->vel.z = frac * 0.0f;
 
        mtx4RotateVecInPlace(THROWMTX, &casing->vel);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.x = (((rand + rand) * g_PistolCasingRotationScaleX) * newvely) - g_PistolCasingRotationOffsetX;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.y = (((rand + rand) * g_PistolCasingRotationScaleY) * newvely) - g_PistolCasingRotationOffsetY;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.z = (((rand + rand) * g_PistolCasingRotationScaleZ) * newvely) - g_PistolCasingRotationOffsetZ;
 
#if VERSION_EU
        matrix_4x4_set_rotation_around_xyz(&rot, &rotmtx);
        matrix_7f05842c_eu(&rotmtx, casing->rot_velocity_mtx);
        if (handoffset);
#else
        matrix_4x4_set_rotation_around_xyz(&rot, &casing->rot_velocity_mtx);
#endif
 
#if VERSION_EU
        randlimit = ((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10;
        randlimit = randlimit + 0x158679;
        randval = randomGetNext();
        oldvely = casing->vel.y;
        frac = ((f32) ((u32) (randval % randlimit))) / g_PistolCasingRandomDivisor;
#else
        randlimit = (((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10) + 0x158679;
        new_var = randlimit;
        randval = randomGetNext();
        oldvely = casing->vel.y;
        frac = ((f32) ((u32) (randval % new_var))) / g_PistolCasingRandomDivisor;
#endif
        newvely = oldvely - (frac * g_PistolCasingGravity);
 
        casing->vel.y = newvely;
        casing->pos.y += (frac * (oldvely + newvely)) * 0.5f;
        casing->pos.x += frac * casing->vel.x;
        casing->pos.z += frac * casing->vel.z;
 
        // Keep the 0 + 1 for matching.
        if (g_ClockTimer >= (0 + 1))
        {
            casing->vel.x += (THROWPOS(0) - THROWPREV(0)) / g_GlobalTimerDelta;
            casing->vel.y += (THROWPOS(1) - THROWPREV(1)) / g_GlobalTimerDelta;
            casing->vel.z += (THROWPOS(2) - THROWPREV(2)) / g_GlobalTimerDelta;
        }
    }
    else
    {
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.x = -(((rand * g_RifleCasingHorizontalSpeed) * 0.125f) + g_RifleCasingHorizontalSpeed);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        casing->vel.y = ((rand * g_RifleCasingVerticalSpeed) * 0.125f) + g_RifleCasingVerticalSpeed;
        casing->vel.z = 0.0f;
 
        mtx4RotateVecInPlace(THROWMTX, &casing->vel);
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.x = (((rand + rand) * g_RifleCasingRotationScaleX) * 0.0625f) - g_RifleCasingRotationOffsetX;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.y = (((rand + rand) * g_RifleCasingRotationScaleY) * 0.0625f) - g_RifleCasingRotationOffsetY;
 
        rand = ((f32) ((u32) randomGetNext())) * 2.3283064e-10f;
        rot.z = (((rand + rand) * g_RifleCasingRotationScaleZ) * 0.0625f) - g_RifleCasingRotationOffsetZ;
 
#if VERSION_EU
        matrix_4x4_set_rotation_around_xyz(&rot, &rotmtx);
        matrix_7f05842c_eu(&rotmtx, casing->rot_velocity_mtx);
        if (handoffset);
#else
        matrix_4x4_set_rotation_around_xyz(&rot, &casing->rot_velocity_mtx);
#endif
        randval = ((s32) ((randomGetNext() >> 24) * 0x158679)) >> 10;
#if VERSION_EU
        randval = randval + 0x158679;
        randlimit = randomGetNext();
        oldvely = (&casing->vel)->y;
        frac = ((f32) ((u32) (randlimit % randval))) / g_RifleCasingRandomDivisor;
#else
        randlimit = randval + 0x158679;
        randval = randomGetNext();
        oldvely = (&casing->vel)->y;
        frac = ((f32) ((u32) (randval % randlimit))) / g_RifleCasingRandomDivisor;
#endif
        newvely = (casing->vel.y = oldvely - (frac * g_RifleCasingGravity));
 
        casing->pos.y += (frac * (oldvely + newvely)) * 0.5f;
        casing->pos.x += frac * casing->vel.x;
        casing->pos.z += frac * casing->vel.z;
 
        if (g_ClockTimer > 0)
        {
            casing->vel.x += (THROWPOS(0) - THROWPREV(0)) / g_GlobalTimerDelta;
            casing->vel.y += (THROWPOS(1) - THROWPREV(1)) / g_GlobalTimerDelta;
            casing->vel.z += (THROWPOS(2) - THROWPREV(2)) / g_GlobalTimerDelta;
        }
    }
 
    if (handoffset);
}
#endif


extern const f32 g_CasingGravity;
extern const f32 g_CasingModelScale;
extern const f32 g_CasingMinMatrixTranslation;
extern const f32 g_CasingMaxMatrixTranslation;

void update_bullet_casing(CasingRecord* casing)
{
    f32 new_val_y;
    f32 delta;
    s32 i;
    struct player* current_player;

    delta = g_GlobalTimerDelta;
    new_val_y = casing->vel.y - (delta * g_CasingGravity);

    casing->pos.y += delta * 0.5f * (casing->vel.y + new_val_y);

    if (casing->pos.y < casing->floor_y_pos)
    {
#if defined(BUGFIX_R1)
        if (g_CasingSfxState == 0 && (g_ClockTimer > 0))
#else
        if (g_CasingSfxState == 0)
#endif
        {
            if ((g_CurrentPlayer->hands[0].weapon_action_state != GUN_ANIM_STATE_FIRE) && (g_CurrentPlayer->hands[1].weapon_action_state != GUN_ANIM_STATE_FIRE))
            {
                // Play bullet casing rolling on floor sound
                sndPlaySfx((struct ALBankAlt_s* ) g_musicSfxBufferPtr, CART_SPENT_SFX, (ALSoundState* ) &g_CasingSfxState);
            }
        }

        // This casing is removed and not updated anymore
        casing->header = NULL;
        return;
    }

    casing->vel.y = new_val_y;
    casing->pos.x += delta * casing->vel.x;
    casing->pos.z += delta * casing->vel.z;

    for (i = 0; i < g_ClockTimer; i++)
    {
#if defined(VERSION_US) || defined(VERSION_JP)
        matrix_4x4_multiply_homogeneous_in_place(&casing->rot_velocity_mtx, &casing->rot_mtx);
#else
        matrix_4x4_multiply_homogeneous_in_place_eu(casing->rot_velocity_mtx, casing->rot_mtx);
#endif
    }
}


void update_bullet_casings(void)
{
    CasingRecord* end = g_Casings + ARRAYCOUNT(g_Casings);
    CasingRecord* entry = g_Casings;

    while (entry < end)
    {
        if (entry->header)
        {
            update_bullet_casing(entry);
        }

        entry++;
    }
}

typedef struct ModelHead {
    s16 unk00;
    s16 rwdatalen;
    void *chr;
    ModelFileHeader *obj;
    RenderPosView *render_pos;
    union ModelRwData **datas;
    f32 scale;
    Model *attachedto;
    ModelNode *attachedto_objinst;
} ModelHead;

void sub_GAME_7F068EC4(CasingRecord *casing, Gfx **gdl)
{
    Gfx             *savedgdl = *gdl;
    ModelFileHeader *model_header = casing->header;
    RenderPosView   *model_matrices = dynAllocate(model_header->numMatrices * sizeof(RenderPosView));
    ModelHead        model;
    ModelRenderData  render_data = *(ModelRenderData *)g_DefaultCasingModelRenderData;
    Mtxf             casing_model_mtx;
    s32              axis_offset;
    s32              matrix_translation_in_range = TRUE;
    f32              max_matrix_translation;
    f32              model_scale_or_min_translation;
    u8              *matrix_axis_ptr;

    modelCalculateRwDataLen(model_header);
    modelInit((Model *)&model, model_header, NULL);

    model.render_pos = model_matrices;

#if defined(VERSION_EU)
    matrix_4x4_copy_eu(casing->rot_mtx, casing_model_mtx.m);
#else
    matrix_4x4_copy(&casing->rot_mtx, &casing_model_mtx);
#endif

    model_scale_or_min_translation = g_CasingModelScale;
    matrix_scalar_multiply(model_scale_or_min_translation, &casing_model_mtx);

    matrix_4x4_set_position(&casing->pos, &casing_model_mtx);

    matrix_4x4_multiply_homogeneous(
        camGetWorldToScreenMtxf(),
        &casing_model_mtx,
        (Mtxf *)model.render_pos);

    model_scale_or_min_translation = g_CasingMinMatrixTranslation;
    max_matrix_translation         = g_CasingMaxMatrixTranslation;

    axis_offset     = 0;
    matrix_axis_ptr = (u8 *)model.render_pos;

    // Offset 0x30 is m[3][0]; advancing the pointer checks translation X, Y and Z.
    while (axis_offset != 12)
    {
        if (max_matrix_translation < *(f32 *)(matrix_axis_ptr + 0x30))
        {
            matrix_translation_in_range = FALSE;
        }
        else if (*(f32 *)(matrix_axis_ptr + 0x30) < model_scale_or_min_translation)
        {
            matrix_translation_in_range = FALSE;
        }

        axis_offset += 4;
        matrix_axis_ptr += 4;
    }

    if (matrix_translation_in_range)
    {
        render_data.zbufferenabled = 0;
        render_data.gdl            = savedgdl;
        render_data.mtxlist        = (Mtxf *)model_matrices;
        render_data.PropType       = PROP_TYPE_WEAPON;

        render_data.envcolour.word =
            ((g_CurrentPlayer->tileColor.a |
              (g_CurrentPlayer->tileColor.r << 24)) |
             (g_CurrentPlayer->tileColor.g << 16)) |
            (g_CurrentPlayer->tileColor.b << 8);

        subdraw(&render_data, (Model *)&model);

        *gdl = render_data.gdl;

        bondviewTransformManyPosToViewMatrix(model_matrices, model_header->numMatrices);
    }
}


// Address: 0x7F06908C
void gunRenderCasings(Gfx **gdl)
{
    CasingRecord* end = g_Casings + ARRAYCOUNT(g_Casings);
    CasingRecord* entry = g_Casings;

    while (entry < end)
    {
        if (entry->header)
        {
            sub_GAME_7F068EC4(entry, gdl);
        }
        
        entry++;
    }
}


void gunSetGunAmmoVisible(s32 reason, bool enable) {

	if (enable)
    {
		g_CurrentPlayer->gunammooff &= ~reason;
		return;
	}

	g_CurrentPlayer->gunammooff |= reason;
}



void give_cur_player_ammo(s32 ammo_type, s32 ammo_amount) {
    enum ITEM_IDS weapon_id;
    s32 max_ammo;

    weapon_id = getCurrentPlayerWeaponId(GUNRIGHT);
    if ((get_ammo_type_for_weapon(weapon_id) == ammo_type) && (bondwalkItemCheckBitflags(weapon_id, WEAPONSTATBITFLAG_AMMO_CLIP_LIMIT) != 0))
    {
        g_CurrentPlayer->hands[0].weapon_ammo_in_magazine += ammo_amount;
        if (get_ptr_item_statistics(weapon_id)->MagSize < g_CurrentPlayer->hands[0].weapon_ammo_in_magazine)
        {
            g_CurrentPlayer->hands[0].weapon_ammo_in_magazine = (s32) get_ptr_item_statistics(weapon_id)->MagSize;
        }
        g_CurrentPlayer->ammoheldarr[ammo_type] = 0;
        return;
    }

    max_ammo = ammo_related[ammo_type].MaxAmmo;
    if (max_ammo < ammo_amount)
    {
        g_CurrentPlayer->ammoheldarr[ammo_type] = max_ammo;
        return;
    }

    g_CurrentPlayer->ammoheldarr[ammo_type] = ammo_amount;
}




s32 check_cur_player_ammo_amount_in_inventory(AMMOTYPE ammotype) {
    return g_CurrentPlayer->ammoheldarr[ammotype];
}

s32 currentPlayerGetAmmoCount(AMMOTYPE ammotype) {

    s32 total_ammo = check_cur_player_ammo_amount_in_inventory(ammotype);

    if (get_ammo_type_for_weapon(getCurrentPlayerWeaponId(GUNRIGHT)) == ammotype) {
        total_ammo += get_ammo_in_hands_magazine(GUNRIGHT);
    }

    if (get_ammo_type_for_weapon(getCurrentPlayerWeaponId(GUNLEFT)) == ammotype) {
        total_ammo += get_ammo_in_hands_magazine(GUNLEFT);
    }

    return total_ammo;
}



s32 get_max_ammo_for_type(s32 arg0)
{
    return ammo_related[arg0].MaxAmmo;
}




void set_max_ammo_for_cur_player(void)
{
    s32 ammo_type;

    for (ammo_type = 0; ammo_type < AMMO_RELATED_MAX; ammo_type++)
    {
        give_cur_player_ammo(ammo_type, ammo_related[ammo_type].MaxAmmo);
    }
}



s32 get_ammo_in_hands_magazine(GUNHAND hand) {
    return g_CurrentPlayer->hands[hand].weapon_ammo_in_magazine;
}



s32 get_ammo_in_hands_weapon(enum GUNHAND hand)
{
    s32 weapon_id;
    s32 ammo_count;

    weapon_id = getCurrentPlayerWeaponId(hand);
    ammo_count = get_ammo_count_for_weapon(weapon_id);

    if ((weapon_id == ITEM_SHOTGUN) || (weapon_id == ITEM_AUTOSHOT))
    {
        s32 other_weapon_id;
        other_weapon_id = getCurrentPlayerWeaponId(1 - hand);

        if ((other_weapon_id == ITEM_SHOTGUN) || (other_weapon_id == ITEM_AUTOSHOT))
        {
            return ammo_count - g_CurrentPlayer->hands[1 - hand].numvisibleshells;
        }

        /* I don't know why there's an extra return here, but it's needed to match */
        return ammo_count;
    }

    return ammo_count;
}



s32 get_ammo_type_for_weapon(ITEM_IDS weapon) {
    return get_ptr_item_statistics(weapon)->AmmoType;
}

s32 get_ammo_count_for_weapon(ITEM_IDS weapon) {
  WeaponStats *weaponstats = get_ptr_item_statistics(weapon);
  return g_CurrentPlayer->ammoheldarr[weaponstats->AmmoType];
}

void add_ammo_to_weapon(ITEM_IDS weapon, s32 ammo) {
    give_cur_player_ammo(get_ptr_item_statistics(weapon)->AmmoType, ammo);
}

s32 get_max_ammo_for_weapon(enum ITEM_IDS weapon)
{
    return ammo_related[get_ptr_item_statistics(weapon)->AmmoType].MaxAmmo;
}


Gfx *microcode_generation_ammo_related(Gfx *gdl, struct sImageTableEntry *tconfig, f32 x, f32 y, f32 arg4, s32 arg5, f32 arg6, s32 arg7, s32 red, s32 green, s32 blue, s32 alpha)
{
    f32 xy[2];
    f32 halfed[2];
 
    gDPSetColorDither(gdl++, G_CD_DISABLE);
    gDPSetTexturePersp(gdl++, G_TP_NONE);
    gDPSetAlphaCompare(gdl++, G_AC_NONE);
    gDPSetTextureLOD(gdl++, G_TL_TILE);
    gDPSetTextureFilter(gdl++, G_TF_POINT);
    gDPSetTextureConvert(gdl++, G_TC_FILT);
    gDPSetTextureLUT(gdl++, G_TT_NONE);
 
    xy[0] = ((u32)tconfig->width * 0.5f) - (f32)(tconfig->width / 2);

    if (arg5 != 0) 
    { 
        xy[0] = -xy[0]; 
    }

    xy[0] = xy[0] + x;
 
    if (0.0f <= y)
    {
        xy[1] = y - (((f32) ((u32) tconfig->height)) * 0.5f);
    }
    else
    {
        xy[1] = -((((f32)((u32)tconfig->height)) * 0.5f) - ((f32)(tconfig->height / 2)));
        xy[1] = arg4 + xy[1];
        xy[1] = xy[1] + arg6;
    }
 
    halfed[0] = ((f32) ((u32) tconfig->width)) * 0.5f;
    halfed[1] = ((f32) ((u32) tconfig->height)) * 0.5f;
 
    gDPPipeSync(gdl++);
    gDPSetCycleType(gdl++, G_CYC_1CYCLE);
    gDPSetRenderMode(gdl++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPSetCombineMode(gdl++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(gdl++, 0, 0, 0, 0, 0, 0);
    gDPFillRectangle(gdl++, ((s32)(xy[0] - halfed[0])) - 1, ((s32)(xy[1] - halfed[1])) - 1, ((s32)(xy[0] + halfed[0])) + 1, ((s32)(xy[1] + halfed[1])) + 1);
 
    texSelect(&gdl, tconfig, (arg7 != 0) ? (2) : (1), 0, 0);
    display_image_at_position(&gdl, xy, halfed, tconfig->width, tconfig->height, 0, 0, 1, red, green, blue, alpha, 0 < tconfig->level, 0);
 
    gDPPipeSync(gdl++);
    gDPSetColorDither(gdl++, G_CD_BAYER);
    gDPSetTexturePersp(gdl++, G_TP_PERSP);
    gDPSetAlphaCompare(gdl++, G_AC_NONE);
    gDPSetTextureLOD(gdl++, G_TL_LOD);
    gDPSetTextureFilter(gdl++, G_TF_BILERP);
    gDPSetTextureConvert(gdl++, G_TC_FILT);
    gDPSetTextureLUT(gdl++, G_TT_NONE);
 
    return gdl;
}


/**
 * Address: TODO
 * WARNING: This function is missing a "return". This will cause bugs on other compilers.
 */
Gfx *set_rgba_redirect_generate_microcode(Gfx *gdl, sImageTableEntry *tconfig, f32 x, f32 y, f32 arg4, s32 arg5, f32 arg6, s32 arg7)
{
    microcode_generation_ammo_related(gdl, tconfig, x, y, arg4, arg5, arg6, arg7, 0xff, 0xff, 0xff, 0xff);
}


/**
 * Address: TODO
 */
Gfx *gunDrawHudString(Gfx *gdl, s8 *text, s32 x, s32 halign, s32 y, s32 valign, bool outline)
{
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;
    s32 textheight;
    s32 textwidth;

    x1 = 0;
    y1 = 0;
    x2 = 0;
    y2 = 0;
    textwidth = 0;
    textheight = 0;

    textMeasure(&textheight, &textwidth, text, ptrFontBankGothicChars, ptrFontBankGothic, 0);

    if (halign == HUDHALIGN_LEFT) { // left
		x2 = x + textwidth;
		x1 = x;
	} else if (halign == HUDHALIGN_RIGHT) { // right
		x1 = x - textwidth;
		x2 = x;
	} else if (halign == HUDHALIGN_MIDDLE) { // middle
		x2 = x + textwidth / 2;
		x1 = x2 - textwidth;
	}

    if (valign == HUDVALIGN_TOP) { // top
		y2 = y + textheight;
		y1 = y;
	} else if (valign == HUDVALIGN_BOTTOM) { // bottom
		y1 = y - textheight;
		y2 = y;
	} else if (valign == HUDVALIGN_MIDDLE) { // middle
		y2 = y + textheight / 2;
		y1 = y2 - textheight;
	}

    gdl = draw_blackbox_to_screen(gdl, &x1, &y1, &x2, &y2);

    if (outline) {
        gdl = textRenderOutlined(gdl, &x1, &y1, text, ptrFontBankGothicChars, ptrFontBankGothic, -1, 0x646464FF, (s32) viGetX(), viGetY(), 0, 0);
    } else {
        gdl = textRender(gdl, &x1, &y1, text, ptrFontBankGothicChars, ptrFontBankGothic, 0xFF00B0, (s32) viGetX(), viGetY(), 0, 0);
    }

    return gdl;
}


/**
 * Address: TODO
 */
Gfx *gunDrawHudInteger(Gfx *gdl, s32 value, s32 x, s32 halign, s32 y, s32 valign, bool outline)
{
    char buffer[12];
    sprintf(buffer, g_GunHudIntegerFormat, value);
    return gunDrawHudString(gdl, buffer, x, halign, y, valign, outline);
}


/**
 * Draw magazine ammo number, ammo type icon, and total ammo number at the bottom right of the viewport.
 * Render an additional ammo counter at the bottom left of the viewport when dual wielding.
 */
Gfx *generate_ammo_total_microcode(Gfx *gdl)
{
    ITEM_IDS weapon_left;
    ITEM_IDS weapon_right;
    s32 ammotype;
    s32 leftx;
    s32 rightx;
    s32 reserveammo;
    s32 magammo;
    u32 imageoffset_r;
    s32 textwidth_r;
    u32 imageoffset_l;
    s32 textwidth_l;

    if (g_CurrentPlayer->gunammooff == 0)
    {
        if (g_CurrentPlayer->mpmenuon == 0)
        {
            weapon_left = getCurrentPlayerWeaponId(GUNLEFT);
            weapon_right = getCurrentPlayerWeaponId(GUNRIGHT);

            if (getPlayerCount() == 1)
            {
                /* Preserve the original single-player dual-wield HUD exactly. */
                leftx = 59;
                rightx = 59;
            }
            else if (getPlayerCount() == 2)
            {
                /* Two-player views are full-width. Keep both counters at the
                 * same 59px edge inset so the left hand is an exact horizontal
                 * mirror of the right hand. */
                leftx = 59;
                rightx = 59;
            }
            else
            {
                s32 oldrightx;

                /* 3/4P: put the left-hand readout where the old right-hand
                 * readout lived, then mirror that location for the new right
                 * hand readout. */
                oldrightx = (get_cur_playernum() & 1) ? 127 : 109;
                leftx = getPlayer_c_screenwidth() - oldrightx;
                rightx = leftx;
            }

#ifdef GE_MODDED_CHEATS
            /* R19: equipment selected through B+Z/the watch has no ammo HUD.
             * Put its watch/inventory name at the normal right-ammo location. */
            if (weapon_right >= ITEM_BOMBCASE && weapon_right < ITEM_IDS_MAX)
            {
                s8 *itemname = (s8 *)bondinvGetNameByIndex(bondinvGetCurEquippedItem());

                if (itemname && *itemname)
                {
                    gdl = microcode_constructor(gdl);
                    /* Mission-item names can be much wider than an ammo
                     * counter. Anchor the string to the viewport's right safe
                     * edge instead of centering it on the ammo icon position;
                     * this keeps long labels such as WATCH MAGNET ATTRACT from
                     * being clipped off the right side. */
                    gdl = gunDrawHudString(gdl, itemname,
                        (viGetViewLeft() + viGetViewWidth()) - 8, HUDHALIGN_RIGHT,
#if defined(VERSION_EU)
                        (viGetViewTop() + viGetViewHeight()) - 28, HUDVALIGN_MIDDLE, 1);
#else
                        (viGetViewTop() + viGetViewHeight()) - 18, HUDVALIGN_MIDDLE, 1);
#endif
                    gdl = combiner_bayer_lod_perspective(gdl);
                }

                return gdl;
            }
#endif

            if (weapon_right != ITEM_UNARMED)
            {
                ammotype = get_ammo_type_for_weapon(weapon_right);

                if (ammotype != 0
                    && g_CurrentPlayer->hands[0].weapon_action_state != GUN_ANIM_STATE_SWITCH_SWAP
                    && g_CurrentPlayer->hands[0].weapon_action_state != GUN_ANIM_STATE_SWITCH_HOLD
                    && !bondwalkItemCheckBitflags(weapon_right, WEAPONSTATBITFLAG_HIDE_AMMO_DISPLAY))
                {
                    imageoffset_r = ammo_related[ammotype].IconImage;
                    textwidth_r = 5;

                    if (imageoffset_r != 0)
                    {
                        imageoffset_r += globalbank_rdram_offset;
                        gdl = set_rgba_redirect_generate_microcode(gdl, (u8 *)imageoffset_r,
#ifdef GE_MODDED_CHEATS
                            (getPlayerCount() == 2) ? ((viGetViewLeft() + viGetViewWidth()) - (f32)rightx) :
#endif
                            ((getPlayer_c_screenleft() + getPlayer_c_screenwidth()) - (f32)rightx), -1.0f,
#if defined(VERSION_EU)
                            (viGetViewTop() + viGetViewHeight()) - 30, 0,
#else
                            (viGetViewTop() + viGetViewHeight()) - 20, 0,
#endif
                            ammo_related[ammotype].IconYOffset, 1);
                        textwidth_r = ((u8 *)imageoffset_r)[4];
                    }

                    gdl = microcode_constructor(gdl);

                    if (bondwalkItemCheckBitflags(weapon_right, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        magammo = 0;
                        reserveammo = g_CurrentPlayer->ammoheldarr[ammotype] + g_CurrentPlayer->hands[0].weapon_ammo_in_magazine;
                        if (weapon_left == weapon_right)
                        {
                            reserveammo += g_CurrentPlayer->hands[1].weapon_ammo_in_magazine;
                        }
                    }
                    else
                    {
                        magammo = g_CurrentPlayer->hands[0].weapon_ammo_in_magazine;
                        reserveammo = g_CurrentPlayer->ammoheldarr[ammotype];
                    }

                    if (!bondwalkItemCheckBitflags(weapon_right, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        gdl = gunDrawHudInteger(gdl, magammo, (((viGetViewLeft() + viGetViewWidth()) - rightx) - (textwidth_r / 2)) - 4, 0,
#if defined(VERSION_EU)
                            (viGetViewTop() + viGetViewHeight()) - 28, 2, 1);
#else
                            (viGetViewTop() + viGetViewHeight()) - 18, 2, 1);
#endif
                    }

                    if (reserveammo > 0 || bondwalkItemCheckBitflags(weapon_right, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        gdl = gunDrawHudInteger(gdl, reserveammo, (((viGetViewLeft() + viGetViewWidth()) - rightx) + ((textwidth_r + 1) / 2)) + 3, 1,
#if defined(VERSION_EU)
                            (viGetViewTop() + viGetViewHeight()) - 28, 2, 1);
#else
                            (viGetViewTop() + viGetViewHeight()) - 18, 2, 1);
#endif
                    }

                    gdl = combiner_bayer_lod_perspective(gdl);
                }
            }

            if (weapon_left != ITEM_UNARMED)
            {
                s32 left_hud_y;
                s32 left_hud_valign;

                ammotype = get_ammo_type_for_weapon(weapon_left);

                /* R3: keep both hand-aware counters on the familiar bottom
                 * row. In 3/4-player split-screen leftx/rightx above provide
                 * the horizontal separation so tall ammo icons do not stack. */
#if defined(VERSION_EU)
                left_hud_y = (viGetViewTop() + viGetViewHeight()) - 28;
#else
                left_hud_y = (viGetViewTop() + viGetViewHeight()) - 18;
#endif
                /* Retail passes valign=2 (middle) here.  R3 accidentally
                 * changed this to HUDVALIGN_BOTTOM (0), shifting the left-hand
                 * counter in 1P/2P. Restore the original vertical anchor. */
                left_hud_valign = HUDVALIGN_MIDDLE;

                if (ammotype != 0
                    && g_CurrentPlayer->hands[1].weapon_action_state != GUN_ANIM_STATE_SWITCH_SWAP
                    && g_CurrentPlayer->hands[1].weapon_action_state != GUN_ANIM_STATE_SWITCH_HOLD
                    && !bondwalkItemCheckBitflags(weapon_left, WEAPONSTATBITFLAG_HIDE_AMMO_DISPLAY))
                {
                    imageoffset_l = ammo_related[ammotype].IconImage;
                    textwidth_l = 5;

                    if (imageoffset_l != 0)
                    {
                        imageoffset_l += globalbank_rdram_offset;
                        gdl = set_rgba_redirect_generate_microcode(gdl, (u8 *)imageoffset_l,
#ifdef GE_MODDED_CHEATS
                            (getPlayerCount() == 2) ? (viGetViewLeft() + (f32)leftx) :
#endif
                            (getPlayer_c_screenleft() + (f32)leftx), -1.0f,
#if defined(VERSION_EU)
                            (viGetViewTop() + viGetViewHeight()) - 30, 1,
#else
                            (viGetViewTop() + viGetViewHeight()) - 20, 1,
#endif
                            ammo_related[ammotype].IconYOffset, 1);
                        textwidth_l = ((u8 *)imageoffset_l)[4];
                    }

                    gdl = microcode_constructor(gdl);

                    if (bondwalkItemCheckBitflags(weapon_left, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        magammo = 0;
                        reserveammo = g_CurrentPlayer->ammoheldarr[ammotype] + g_CurrentPlayer->hands[1].weapon_ammo_in_magazine;
                        if (weapon_left == weapon_right)
                        {
                            reserveammo += g_CurrentPlayer->hands[0].weapon_ammo_in_magazine;
                        }
                    }
                    else
                    {
                        magammo = g_CurrentPlayer->hands[1].weapon_ammo_in_magazine;
                        reserveammo = g_CurrentPlayer->ammoheldarr[ammotype];
                    }

                    if (!bondwalkItemCheckBitflags(weapon_left, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        gdl = gunDrawHudInteger(gdl, magammo, ((viGetViewLeft() + leftx) + (textwidth_l / 2)) + 3, 1,
left_hud_y, left_hud_valign, 1);
                    }

                    if (reserveammo > 0 || bondwalkItemCheckBitflags(weapon_left, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
                    {
                        gdl = gunDrawHudInteger(gdl, reserveammo, ((viGetViewLeft() + leftx) - ((textwidth_l + 1) / 2)) - 4, 0,
left_hud_y, left_hud_valign, 1);
                    }

                    gdl = combiner_bayer_lod_perspective(gdl);
                }
            }
        }
    }

    return gdl;
}


/**
 * Address: 7F06A334
 */
Gfx *gunDrawWatchAmmoDisplay(Gfx *gdl)
{
    ITEM_IDS offhanditem;
    ITEM_IDS item;
    s32 ammotype;
    s32 reserveammo;
    s32 magammo;
    u32 imageoffset;
    s32 textwidth;
    s32 pad;

    offhanditem = getCurrentPlayerWeaponId(1);
    item = getCurrentPlayerWeaponId(0);

    if (item != ITEM_UNARMED)
    {
        ammotype = get_ammo_type_for_weapon(item);

        if (ammotype != 0
            && g_CurrentPlayer->hands[GUNRIGHT].weapon_action_state != GUN_ANIM_STATE_SWITCH_SWAP
            && g_CurrentPlayer->hands[GUNRIGHT].weapon_action_state != GUN_ANIM_STATE_SWITCH_HOLD
            && !bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_HIDE_AMMO_DISPLAY))
        {
            imageoffset = ammo_related[ammotype].IconImage;
            textwidth = 5;

            get_ptr_item_statistics(item);

            if (imageoffset != 0)
            {
                imageoffset += globalbank_rdram_offset;

                // Draw the ammo icon
#if defined(VERSION_EU)
                gdl = set_rgba_redirect_generate_microcode(gdl, (u8 *)imageoffset, 200.0f, 208.0f, (viGetViewTop() + viGetViewHeight()) - 30, 0, ammo_related[ammotype].IconYOffset, 1);
#else
                gdl = set_rgba_redirect_generate_microcode(gdl, (u8 *)imageoffset, 200.0f, 180.0f, (viGetViewTop() + viGetViewHeight()) - 20, 0, ammo_related[ammotype].IconYOffset, 1);
#endif

                textwidth = ((u8 *)imageoffset)[4];
            }

            gdl = microcode_constructor(gdl);

            if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
            {
                magammo = 0;
                reserveammo = g_CurrentPlayer->ammoheldarr[ammotype] + g_CurrentPlayer->hands[0].weapon_ammo_in_magazine;

                if (offhanditem == item)
                {
                    reserveammo += g_CurrentPlayer->hands[1].weapon_ammo_in_magazine;
                }
            }
            else
            {
                magammo = g_CurrentPlayer->hands[0].weapon_ammo_in_magazine;
                reserveammo = g_CurrentPlayer->ammoheldarr[ammotype];
            }

            if (!bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
            {
                // Draw the magazine ammo count.
#if defined(VERSION_EU)
                gdl = gunDrawHudInteger(gdl, magammo, 196 - (textwidth / 2), 0, 205, 2, 0);
#else
                gdl = gunDrawHudInteger(gdl, magammo, 196 - (textwidth / 2), 0, 177, 2, 0);
#endif
            }

            if (reserveammo > 0 || bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_NO_CLIP_RELOADS))
            {
                // Draw the reserve ammo count.
#if defined(VERSION_EU)
                gdl = gunDrawHudInteger(gdl, reserveammo, 203 + ((textwidth + 1) / 2), 1, 205, 2, 0);
#else
                gdl = gunDrawHudInteger(gdl, reserveammo, 203 + ((textwidth + 1) / 2), 1, 177, 2, 0);
#endif
            }

            gdl = combiner_bayer_lod_perspective(gdl);
        }
    }

    return gdl;
}


void gunSetSightVisible(s32 reason, bool visible)
{
    if (visible)
    {
        g_CurrentPlayer->gunsightmode &= ~reason;
        return;
    }

    g_CurrentPlayer->gunsightmode |= reason;
}


void gunDrawSight(s32 *gdl) {

    s32 sp54;
    f32 xypos[2];
    f32 halfedxy[2];

    if ((g_CurrentPlayer->gunsightmode == 0) && (g_CurrentPlayer->mpmenuon == FALSE)) {
        sp54 = *gdl;
        texSelect(&sp54, crosshairimage, 4, 0, 0);

        xypos[0] = g_CurrentPlayer->crosshair_angle.f[0];
        xypos[1] = g_CurrentPlayer->crosshair_angle.f[1];
        halfedxy[0] = halfedxy[1] = g_CurrentPlayer->insightaimmode ? 16.0f : 10.0f;

        if (get_screen_ratio() != SCREEN_RATIO_NORMAL) {
            halfedxy[0] = halfedxy[0] / get_screen_ratio_factor();
        }
#ifdef VERSION_EU
        halfedxy[1] = halfedxy[1] * g_GunSightAspectRatio;
#endif
        display_image_at_position(&sp54, &xypos, &halfedxy, 0x20, 0x20, 0, 0, 1, 0xFF, 0xFF, 0xFF, 0x6E, (crosshairimage->level > 0), 0);
        *gdl = sp54;
    }
}


void inc_curplayer_hitcount_with_weapon(ITEM_IDS item, SHOT_REGISTER shot_register) {

    if (bondwalkItemCheckBitflags(item, WEAPONSTATBITFLAG_PLAYER_STAT_HIT)) {
        g_playerPerm->shot_count[shot_register] = g_playerPerm->shot_count[shot_register]+1;
    }
}


s32 get_curplayer_shot_register(SHOT_REGISTER shot_register)
{
  return g_playerPerm->shot_count[shot_register];
}


void inc_cur_civilian_casualties(void)
{
    g_playerPerm->killed_civilians++;
}


s32 get_civilian_casualties(void)
{
    return g_playerPerm->killed_civilians;
}


void increment_num_kills_display_text_in_MP(void)
{
    s8 buffer[256];
    s32 time_since_kill;
    s32 recent_kill_count;
    s32 mission_time;
    s32 unused; // needed this variable to match

    g_playerPerm->kill_count += 1;
    g_CurrentPlayer->kills_this_life += 1;

    if (getPlayerCount() < 2) { return; }

    mission_time = getMissiontimer();
    sprintf(&buffer, aSD, langGet(getStringID(LGUN, GUN_STR_DA_KILLCOUNT)), g_playerPerm->kill_count); // "kill count"

#if defined(VERSION_US)
    hudmsgBottomShow(&buffer);
#elif defined(VERSION_JP) || defined(VERSION_EU)
    jp_hudmsgBottomShow(&buffer);
#endif

    if (g_playerPerm->kill_count >= 2)
    {
        time_since_kill = mission_time - g_CurrentPlayer->last_kill_time[0];
        if (g_playerPerm->max_time_between_kills < time_since_kill)
        {
            g_playerPerm->max_time_between_kills = time_since_kill;
        }

        if (time_since_kill < g_playerPerm->min_time_between_kills)
        {
            g_playerPerm->min_time_between_kills = time_since_kill;
        }
    }

    recent_kill_count = 1;
    g_CurrentPlayer->last_kill_time[3] = g_CurrentPlayer->last_kill_time[2];
    g_CurrentPlayer->last_kill_time[2] = g_CurrentPlayer->last_kill_time[1];
    g_CurrentPlayer->last_kill_time[1] = g_CurrentPlayer->last_kill_time[0];
    g_CurrentPlayer->last_kill_time[0] = mission_time;

    // I tried to turn this into a loop but it didn't match
    if (g_CurrentPlayer->last_kill_time[1] != -1 && (g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[1]) < 0x78)
    {
        recent_kill_count++;
        if ((g_CurrentPlayer->last_kill_time[2] != -1) && ((g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[2]) < 0x78))
        {
            recent_kill_count++;
            if ((g_CurrentPlayer->last_kill_time[3] != -1) && ((g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[3]) < 0x78))
            {
                recent_kill_count++;
            }
        }
    }

    if (g_playerPerm->most_killed_one_time < recent_kill_count)
    {
        g_playerPerm->most_killed_one_time = recent_kill_count;
    }
}



s32 get_curplay_killcount(void) {
    return g_playerPerm->kill_count;
}

void increment_num_times_killed_MwtGC(void){
    g_playerPerm->killed_gg_owner_count++;
}

s32 get_times_killed_mwtgx(void) {
    return g_playerPerm->killed_gg_owner_count;
}


void increment_num_deaths(void)
{
	char buffer[256];
    g_CurrentPlayer->deathcount = (s32) (g_CurrentPlayer->deathcount + 1);
    if (getPlayerCount() >= 2)
    {
        if (g_CurrentPlayer->deathcount == 1)
        {
            sprintf(buffer, langGet(getStringID(LGUN, GUN_STR_DB_DIEDONCE_LF))); //died once
        }
        else
        {
            sprintf(buffer, g_GunDeathCountFormat, langGet(getStringID(LGUN, GUN_STR_DC_DIED)), g_CurrentPlayer->deathcount, langGet(getStringID(LGUN, GUN_STR_DD_TIMES))); //died times
        }
#if defined(VERSION_JP) || defined(VERSION_EU)
		jp_hudmsgBottomShow(buffer);
#else
		hudmsgBottomShow(buffer);
#endif
    }
}


s32 get_curplayer_numdeaths(void) {
    return g_CurrentPlayer->deathcount;
}

void increment_num_suicides_display_MP(void) {
    char buffer[256];
    s32 time_diff;
    s32 recent_kill_count;
    s32 currentTime;

    g_CurrentPlayer->num_suicides += 1;
    if (getPlayerCount() >= 2) {

        currentTime = getMissiontimer();

        sprintf(&buffer, &aSD_0, langGet(getStringID(LGUN, GUN_STR_DE_SUICIDECOUNT)), g_CurrentPlayer->num_suicides); // "suicide count"

#if defined(VERSION_JP) || defined(VERSION_EU)
		jp_hudmsgBottomShow(&buffer);
#else
		hudmsgBottomShow(&buffer);
#endif

        if (g_playerPerm->kill_count >= 2) {
            time_diff = currentTime - g_CurrentPlayer->last_kill_time[0];
            if (g_playerPerm->max_time_between_kills < time_diff) {
                g_playerPerm->max_time_between_kills = time_diff;
            }
            if (time_diff < g_playerPerm->min_time_between_kills) {
                g_playerPerm->min_time_between_kills = time_diff;
            }
        }
        recent_kill_count = 1;
        g_CurrentPlayer->last_kill_time[3] = g_CurrentPlayer->last_kill_time[2];
        g_CurrentPlayer->last_kill_time[2] = g_CurrentPlayer->last_kill_time[1];
        g_CurrentPlayer->last_kill_time[1] = g_CurrentPlayer->last_kill_time[0];
        g_CurrentPlayer->last_kill_time[0] = currentTime;

        if ( g_CurrentPlayer->last_kill_time[1] != -1) {

            if ((g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[1]) < 0x78) {

                recent_kill_count += 1;

                if ((g_CurrentPlayer->last_kill_time[2] != -1) && ((g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[2]) < 0x78)) {

                    recent_kill_count += 1;

                    if ((g_CurrentPlayer->last_kill_time[3] != -1) && ((g_CurrentPlayer->last_kill_time[0] - g_CurrentPlayer->last_kill_time[3]) < 0x78)) {
                        recent_kill_count += 1;
                    }
                }
            }
        }

        if (g_playerPerm->most_killed_one_time < recent_kill_count) {
            g_playerPerm->most_killed_one_time = recent_kill_count;
        }
    }
}

s32 get_curplayer_numsuicides(void) {
    return g_CurrentPlayer->num_suicides;
}

/*
 * IDO emits scalar const objects to .data. The linker keeps this block directly
 * after gunfire's .rodata so these values retain their original ROM layout.
 */
#if VERSION_EU
const f32 g_GunScreenAspectRatio = 20.0f / 17.0f;
#else
const f32 g_GunScreenAspectRatio = 4.0f / 3.0f;
#endif
const f32 g_CasingSwitchScale = 0.10000001f;
const f32 g_PistolCasingHorizontalSpeed = 0.5333333f;
const f32 g_PistolCasingRotationScaleX = M_TAU_F;
const f32 g_PistolCasingRotationOffsetX = M_PI_F / 8.0f;
const f32 g_PistolCasingRotationScaleY = M_TAU_F;
const f32 g_PistolCasingRotationOffsetY = M_PI_F / 8.0f;
const f32 g_PistolCasingRotationScaleZ = M_TAU_F;
const f32 g_PistolCasingRotationOffsetZ = M_PI_F / 8.0f;
#if VERSION_EU
const f32 g_PistolCasingRandomDivisor = 931050.0f;
#else
const f32 g_PistolCasingRandomDivisor = 775875.0f;
#endif
const f32 g_PistolCasingGravity = 0.2777778f;
const f32 g_RifleCasingHorizontalSpeed = 1.4166666f;
const f32 g_RifleCasingVerticalSpeed = 1.6666666f;
const f32 g_RifleCasingRotationScaleX = M_TAU_F;
const f32 g_RifleCasingRotationOffsetX = M_PI_F / 8.0f;
const f32 g_RifleCasingRotationScaleY = M_TAU_F;
const f32 g_RifleCasingRotationOffsetY = M_PI_F / 8.0f;
const f32 g_RifleCasingRotationScaleZ = M_TAU_F;
const f32 g_RifleCasingRotationOffsetZ = M_PI_F / 8.0f;
#if VERSION_EU
const f32 g_RifleCasingRandomDivisor = 931050.0f;
#else
const f32 g_RifleCasingRandomDivisor = 775875.0f;
#endif
const f32 g_RifleCasingGravity = 0.2777778f;
const f32 g_CasingGravity = 0.2777778f;
const f32 g_CasingModelScale = 0.10000001f;
const f32 g_CasingMinMatrixTranslation = -30000.0f;
const f32 g_CasingMaxMatrixTranslation = 30000.0f;
#if VERSION_EU
const f32 g_GunSightAspectRatio = 25.0f / 21.0f;
#endif
