#include <ultra64.h>
#include <memp.h>
#include "initBondDATA.h"
#include "bondtypes.h"
#include "bondconstants.h"
#include "bondview.h"
#include "gun.h"
#include "player.h"


#ifdef REFRESH_PAL
#define VALUENAME 16.750415802f
#else
#define VALUENAME 19.999996
#endif

struct hand hand_data_dummy = {
    0, // ITEM_IDS weaponnum;
    0xFFFFFFFF, // ITEM_IDS weaponnum_watchmenu;
    0, // ITEM_IDS previous_weapon;
 
    0, // s8 weapon_firing_status;
 
    0, // s8 field_87D;
    0x1, // s8 field_87E;
    0, // s8 field_87F;
 
    0, // s32 weapon_hold_time;
    0, // s32 field_884;
    0, // s32 field_888;
    0, // s32 field_88C;
    0, // s32 field_890;
    0, // s32 weapon_action_state;
    0, // s32 weapon_current_animation;
    0, // s32 weapon_ammo_in_magazine;
    0, // s32 field_8A0;
    0, // s32 numvisibleshells;
    0, // s32 field_8A8;
    0, // s32 weapon_next_weapon;
    0, // s32 field_8B0;
    0, // s32 weapon_animation_trigger;
    0, // s32 field_8B8;
    0, // s32 field_8BC;
    0, // s32 field_8C0;
    0, // s32 field_8C4;
    0, // s32 field_8C8;
    0, // s32 field_8CC;
    0, // s32 field_8D0;
    0, // s32 field_8D4;
    0, // s32 field_8D8;
    0, // s32 field_8DC;
    0, // s32 field_8E0;
    0, // s32 field_8E4;
    0, // s32 field_8E8;
 
    // Mtxf field_8EC;
    1.0f,0,0,0,
    0,1.0f,0,0,
    0,0,1.0f,0,
    0,0,0,1.0f,
 
    0, // s32 field_92C;
    0, // f32 sway_pos_x;
    0, // f32 sway_pos_y;
    0, // f32 sway_pos_z;
    0, // f32 sway_look_x;
    0, // f32 sway_look_y;
    -1.0f, // f32 sway_look_z;
    0, // f32 sway_up_x;
    1.0f, // f32 sway_up_y;
    0, // f32 sway_up_z;
    0, // f32 spring_pos_x;
    0, // f32 spring_pos_y;
    0, // f32 spring_pos_z;
    0, // f32 spring_look_x;
    0, // f32 spring_look_y;
    -VALUENAME, // f32 spring_look_z;
    0, // f32 spring_up_x;
    VALUENAME, // f32 spring_up_y;
    0, // f32 spring_up_z;
 
    // coord3d blendpos[4];
    0,0,0,
    0,0,0,
    0,0,0,
    0,0,0,
 
    // coord3d blendlook[4];
    0,0,-1.0f,
    0,0,-1.0f,
    0,0,-1.0f,
    0,0,-1.0f,
 
    // coord3d blendup[4];
    0,1.0f,0,
    0,1.0f,0,
    0,1.0f,0,
    0,1.0f,0,
 
    0, // s32 curblendpos;
    0, // f32 dampt;
    1.0f, // f32 blendscale;
    1.0f, // f32 blendscale1;
    0, // s32 sideflag;
    0, // f32 weapon_theta_displacement;
    0, // f32 weapon_verta_displacement;
    0, // s32 field_A24;
    0, // f32 gunofs2_x;
    0, // f32 gunofs2_y;
    0, // f32 gunofs2_z;
    0, // f32 field_A34;
    0, // f32 field_A38;
    0, // f32 field_A3C;
    1000.0f, // f32 field_A40;
    NULL, // ALSoundState * audioHandle;
    0, // s32 field_A48;
    0, // s32 field_A4C;
    0, // s32 field_A50;
 
    // // struct BeamRecord {
    {
        0xff, //     s8      unk00;
        0, //     s8      item_id; // type ITEM_IDS
        0, //     char    unk02;
        0, //     char    unk03;
 
        //     coord3d pos;
        0,0,0,
 
        //     /**
        //     * Offset 0x10
        //     */
        //     struct coord3d delta;
        0,0,0,
 
        0, //     f32     unk1c;
 
        0, //     f32     unk20;
        0, //     f32     unk24;
        0 //     f32     unk28;
    },
    // // }
 
    0, // f32 noise;
    0, // f32 field_A84;
    0, // f32 field_A88;
    0, // s32 field_A8C;
    NULL, // ObjectRecord* rocket;
    0, // s32 firedrocket;
    0, // gunmtx_camspace.m[0][0];
    0, // gunmtx_camspace.m[0][1];
    0, // gunmtx_camspace.m[0][2];
    0, // gunmtx_camspace.m[0][3];
    0, // gunmtx_camspace.m[1][0];
    0, // gunmtx_camspace.m[1][1];
    0, // gunmtx_camspace.m[1][2];
    0, // gunmtx_camspace.m[1][3];
    0, // gunmtx_camspace.m[2][0];
    0, // gunmtx_camspace.m[2][1];
    0, // gunmtx_camspace.m[2][2];
    0, // gunmtx_camspace.m[2][3];
    0, // gunmtx_camspace.m[3][0];
    0, // gunmtx_camspace.m[3][1];
    0, // gunmtx_camspace.m[3][2];
    0, // gunmtx_camspace.m[3][3];
 
    // offset 0xad8
    // Mtxf throw_item_pos_related;
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
 
    // offset 0xb18
    // Mtxf throw_item_pos_related_prev;
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
    0,0,0,0,
 
    // coord3d field_B58;
    0,0,0,
 
    0, // f32 field_B64;
    0, // s32 field_B68;
    0, // s32 field_B6C;
    0, // s32 field_B70;
    0, // Mtxf * mtxlist;
    0, // s32 field_B78;
    0, // s32 field_B7C;
    0, // s32 field_B80;
    0, // s32 field_B84;
    0, // s32 modeldatas;
    0, // s32 field_B8C;
    0, // s32 field_B90;
    0, // s32 field_B94;
    0, // s32 field_B98;
    0, // s32 field_B9C;
    0, // s32 field_BA0;
    0, // s32 field_BA4;
    0, // s32 field_BA8;
    0, // s32 field_BAC;
    0, // s32 field_BB0;
    0, // s32 field_BB4;
    0, // s32 field_BB8;
    0, // s32 field_BBC;
    0, // s32 field_BC0;
    0, // s32 field_BC4;
    0, // s32 field_BC8;
    0, // s32 field_BCC;
    0, // s32 field_BD0;
    0, // s32 field_BD4;
    0, // s32 field_BD8;
    0, // s32 field_BDC;
    0, // s32 field_BE0;
    0, // s32 field_BE4;
    0, // s32 field_BE8;
    0, // s32 field_BEC;
    0, // s32 field_BF0;
    0, // s32 field_BF4;
    0, // s32 field_BF8;
    0, // s32 field_BFC;
    0, // s32 field_C00;
    0, // s32 field_C04;
    0, // s32 volley;  // Number of bullets discharged in a row. For pistols, it's always 1 even if the fire button is held.
    // coord3d item_related;
    0,0,0
};


void init_player_BONDdata_stats(void)
{
    s32 i;
    struct hand sp24;

    sp24 = hand_data_dummy;

    g_CurrentPlayer->ptr_hand_weapon_buffer[GUNRIGHT] = mempAllocBytesInBank(size_item_buffer[0], MEMPOOL_STAGE);

    if (getPlayerCount() == 1)
    {
        g_CurrentPlayer->ptr_hand_weapon_buffer[GUNLEFT] = mempAllocBytesInBank(size_item_buffer[1], MEMPOOL_STAGE);
    }
#ifdef GE_MODDED_CHEATS
    else
    {
        /* R3: retail never allocates an off-hand first-person model buffer in
         * multiplayer. Multiplayer dual wielding needs one per player. Keep
         * these buffers in otherwise-unused Expansion Pak RAM rather than
         * consuming the retail stage heap. */
        extern u8 _mpLeftWeaponBuffersStart[];
        g_CurrentPlayer->ptr_hand_weapon_buffer[GUNLEFT] =
            _mpLeftWeaponBuffersStart + (get_cur_playernum() * size_item_buffer[1]);
    }
#endif

    g_CurrentPlayer->hand_invisible[GUNRIGHT] = 0;
    g_CurrentPlayer->hand_invisible[GUNLEFT] = 0;
    g_CurrentPlayer->hand_item[GUNRIGHT] = ITEM_UNARMED;
    g_CurrentPlayer->hand_item[GUNLEFT] = ITEM_UNARMED;
    g_CurrentPlayer->field_2A44[GUNRIGHT] = -1;
    g_CurrentPlayer->field_2A44[GUNLEFT] = -1;
    g_CurrentPlayer->lock_hand_model[GUNRIGHT] = 0;
    g_CurrentPlayer->lock_hand_model[GUNLEFT] = 0;

    g_CurrentPlayer->hands[0] = sp24;
    g_CurrentPlayer->hands[1] = sp24;

    for (i=0; i<30; i++)
    {
        g_CurrentPlayer->ammoheldarr[i] = 0;
    }

    for (i=0; i<7; i++)
    {
        g_playerPerm->shot_count[i] = 0;
    }

    g_playerPerm->killed_civilians = 0;
    g_playerPerm->kill_count = 0;
    g_playerPerm->killed_gg_owner_count = 0;
    g_CurrentPlayer->deathcount = 0;
    g_CurrentPlayer->num_suicides = 0;
    g_CurrentPlayer->gunposamplitude = 1.0f;
    g_CurrentPlayer->gunxamplitude = 1.0f;
    g_CurrentPlayer->trigger_released = 0;
    g_CurrentPlayer->trigger_down = 0;
    g_CurrentPlayer->prev_trigger_down = 0;
    g_CurrentPlayer->z_trigger_timer = 0;
    g_CurrentPlayer->current_trigger_hand = 0;
    g_CurrentPlayer->tileColor.rgba[0] = 0xFF;
    g_CurrentPlayer->tileColor.rgba[1] = 0xFF;
    g_CurrentPlayer->tileColor.rgba[2] = 0xFF;
    g_CurrentPlayer->tileColor.rgba[3] = 0;
    g_CurrentPlayer->resetshadecol = TRUE;
    g_CurrentPlayer->aimtype = 0;
    g_CurrentPlayer->crosshair_angle.f[0] = 0.0f;
    g_CurrentPlayer->crosshair_angle.f[1] = 0.0f;
    g_CurrentPlayer->crosshair_x_pos = 0.0f;
    g_CurrentPlayer->crosshair_y_pos = 0.0f;
    g_CurrentPlayer->guncrossdamp = 0.9f;
    g_CurrentPlayer->field_FFC.f[0] = 0.0f;
    g_CurrentPlayer->field_FFC.f[1] = 0.0f;
    g_CurrentPlayer->gun_azimuth_angle = 0.0f;
    g_CurrentPlayer->gun_azimuth_turning = 0.0f;
    g_CurrentPlayer->gunaimdamp = 0.9f;
    g_CurrentPlayer->field_1010.f[0] = 0.0f;
    g_CurrentPlayer->field_1010.f[1] = -3.1415927f;
    g_CurrentPlayer->field_1010.f[2] = 0.0f;
    g_CurrentPlayer->copiedgoldeneye = FALSE;
    g_CurrentPlayer->magnetattracttime = -1;
    g_CurrentPlayer->gunsync = 0.0f;
    g_CurrentPlayer->syncchange = 0.0f;
    g_CurrentPlayer->synccount = 0.0f;
    g_CurrentPlayer->syncoffset = 0;
    g_CurrentPlayer->field_107C = 0.0f;
    g_CurrentPlayer->field_1080 = 0.0f;
    bgunCalculateBlend(GUNRIGHT);
    bgunCalculateBlend(GUNRIGHT);
    bgunCalculateBlend(GUNRIGHT);
    bgunCalculateBlend(GUNLEFT);
    bgunCalculateBlend(GUNLEFT);
    bgunCalculateBlend(GUNLEFT);
    g_CurrentPlayer->gunammooff = FALSE;
    g_CurrentPlayer->gunsightmode = 2; //GUNSIGHTREASON_AIMING
    g_CurrentPlayer->sniper_zoom = sniperrifle_stats.Zoom;
    g_CurrentPlayer->camera_zoom = camera_stats.Zoom;
}
