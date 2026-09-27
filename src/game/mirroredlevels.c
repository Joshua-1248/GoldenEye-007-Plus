#include <ultra64.h>
#include "bondconstants.h"
#include "bondtypes.h"
#include "game/bg.h"
#include "game/bondview.h"
#include "game/chrai.h"
#include "game/model.h"
#include "game/loadobjectmodel.h"
#include "game/player.h"
#include "game/propobj.h"
#include "game/mirroredlevels.h"
#include "game/stan.h"

#ifdef GE_MODDED_CHEATS

static s32 g_MirrorLevelsEnabled = FALSE;
static s32 g_MirrorLevelsPending = FALSE;
static s32 g_MirrorLevelsPendingEnabled = FALSE;
static s32 g_MirrorSetupApplied = FALSE;

static f32 mirrorAngleDegrees(f32 angle)
{
    angle = 360.0f - angle;

    while (angle >= 360.0f)
    {
        angle -= 360.0f;
    }

    while (angle < 0.0f)
    {
        angle += 360.0f;
    }

    return angle;
}

f32 mirrorLevelsX(f32 x)
{
    return g_MirrorLevelsEnabled ? -x : x;
}

f32 mirrorLevelsDirX(f32 x)
{
    return g_MirrorLevelsEnabled ? -x : x;
}

static void mirrorObjectMatrix(Mtxf *mtx)
{
    if (mtx == NULL)
    {
        return;
    }

    /* Reflect the object's world orientation without applying a negative
     * model scale.  This keeps the model's own triangle winding intact. */
    mtx->m[0][1] = -mtx->m[0][1];
    mtx->m[0][2] = -mtx->m[0][2];
    mtx->m[1][0] = -mtx->m[1][0];
    mtx->m[2][0] = -mtx->m[2][0];
}

static void mirrorRectX(rect4f *rect)
{
    s32 i;

    if (rect == NULL)
    {
        return;
    }

    for (i = 0; i < 4; i++)
    {
        rect->points[i].x = -rect->points[i].x;
    }
}

static void mirrorCollisionState(struct collision434 *state)
{
    if (state == NULL)
    {
        return;
    }

    state->collision_position.x = -state->collision_position.x;
    state->theta_transform.x = -state->theta_transform.x;
    state->pos3.x = -state->pos3.x;
    state->pos.x = -state->pos.x;
    state->applied_view.x = -state->applied_view.x;
    state->applied_view2.x = -state->applied_view2.x;
}

static void mirrorChrActionState(ChrRecord *chr)
{
    if (chr == NULL)
    {
        return;
    }

    switch (chr->actiontype)
    {
        case ACT_RUNPOS:
            chr->act_runpos.pos.x = -chr->act_runpos.pos.x;
            break;
        case ACT_GOPOS:
            chr->act_gopos.targetpos.x = -chr->act_gopos.targetpos.x;
            chr->act_gopos.waydata.pos.x = -chr->act_gopos.waydata.pos.x;
            chr->act_gopos.waydata.pos2.x = -chr->act_gopos.waydata.pos2.x;
            chr->act_gopos.waydata.pos3.x = -chr->act_gopos.waydata.pos3.x;
            chr->act_gopos.waydata.pos_copy.x = -chr->act_gopos.waydata.pos_copy.x;
            break;
        case ACT_PREARGH:
            chr->act_preargh.pos.x = -chr->act_preargh.pos.x;
            break;
        default:
            break;
    }
}

static void mirrorChrState(ChrRecord *chr)
{
    if (chr == NULL)
    {
        return;
    }

    chr->prevpos.x = -chr->prevpos.x;
    chr->lastknowntargetpos.x = -chr->lastknowntargetpos.x;
    chr->fallspeed.x = -chr->fallspeed.x;
    mirrorRectX(&chr->collision_bounds);
    mirrorChrActionState(chr);

    if (chr->model != NULL)
    {
        coord3d modelpos;

        /* The model root is the authoritative real-time character position.
         * chrUpdateAnim() copies it back into prop->pos every tick, so mirror
         * it together with the prop or guards snap back to the canonical X. */
        getsuboffset(chr->model, &modelpos);
        modelpos.x = -modelpos.x;
        setsuboffset(chr->model, &modelpos);
        setsubroty(chr->model, -getsubroty(chr->model));
    }
}

static void mirrorRuntimeProps(void)
{
    /*
     * Prop slots are recycled through g_FreeProps, but chrpropFree() does not
     * clear PropRecord::type or the type-specific union pointer.  Walking all
     * MAX_PROPS by type therefore eventually treats freed CHR/OBJ/DOOR slots
     * as live and dereferences stale pointers.  This is especially easy to
     * trigger after exploring a large stage such as Caverns, where many props
     * have been allocated and freed before a live Mirrored Levels toggle.
     *
     * Mark the actual free-list slots first, then retain the full pool walk so
     * allocated-but-currently-delisted/parented props are still reflected.
     */
    u32 freemask[(MAX_PROPS + 31) / 32];
    PropRecord *freeprop;
    s32 freecount;
    s32 i;

    for (i = 0; i < (s32)(sizeof(freemask) / sizeof(freemask[0])); i++)
    {
        freemask[i] = 0;
    }

    freeprop = g_FreeProps;
    freecount = 0;

    while (freeprop != NULL && freecount < MAX_PROPS)
    {
        s32 slot = freeprop - g_Props;

        if (slot < 0 || slot >= MAX_PROPS)
        {
            /* A corrupt free list must never make the mirror transaction walk
             * arbitrary memory.  Stop marking and leave the normal runtime to
             * surface the underlying allocator problem instead. */
            break;
        }

        freemask[slot >> 5] |= 1u << (slot & 31);
        freeprop = freeprop->prev;
        freecount++;
    }

    for (i = 0; i < MAX_PROPS; i++)
    {
        PropRecord *prop = &g_Props[i];

        if (freemask[i >> 5] & (1u << (i & 31)))
        {
            continue;
        }

        if (prop->type <= PROP_TYPE_NUL || prop->type >= PROP_TYPE_MAX)
        {
            continue;
        }

        if (prop->type == PROP_TYPE_PLAYER)
        {
            continue;
        }

        prop->pos.x = -prop->pos.x;

        if (prop->type == PROP_TYPE_CHR)
        {
            mirrorChrState(prop->chr);
        }
        else if (prop->type == PROP_TYPE_OBJ
              || prop->type == PROP_TYPE_DOOR
              || prop->type == PROP_TYPE_WEAPON)
        {
            ObjectRecord *obj = prop->obj;

            if (obj != NULL)
            {
                obj->runtime_pos.x = -obj->runtime_pos.x;
                mirrorObjectMatrix(&obj->mtx);

                if (prop->type == PROP_TYPE_DOOR)
                {
                    DoorRecord *door = prop->door;

                    if (door != NULL)
                    {
                        /* Sliding displacement is runtime_pos + direction *
                         * openPosition.  Reflect only the X component of the
                         * displacement vector so the door follows the exact
                         * mirrored track during a live toggle. */
                        door->frac = -door->frac;

                        /* The door collision polygon is cached from the door
                         * transform.  Rebuild it immediately after a live
                         * mirror toggle so players/guards cannot pass through
                         * a visual door that moved to the reflected side. */
                        doorUpdateBbox(door);
                    }
                }
                else if (prop->type == PROP_TYPE_OBJ)
                {
                    /* Ordinary solid props cache their collision hull too.
                     * Their visual/runtime transform has just been reflected,
                     * so regenerate the polygon/top/bottom from that exact
                     * mirrored transform instead of leaving collision at the
                     * old canonical X. */
                    chrobjCollisionRelated(obj);
                }

                if (obj->type == PROPDEF_VEHICHLE)
                {
                    VehichleRecord *vehicle = (VehichleRecord *)obj;
                    vehicle->turnrot60 = -vehicle->turnrot60;
                    vehicle->roty = -vehicle->roty;
                }
                else if (obj->type == PROPDEF_AIRCRAFT)
                {
                    AircraftRecord *aircraft = (AircraftRecord *)obj;
                    aircraft->yrot = -aircraft->yrot;
                }
                else if (obj->type == PROPDEF_TANK)
                {
                    TankRecord *tank = (TankRecord *)obj;
                    tank->turret_orientation_angle = -tank->turret_orientation_angle;
                    tank->tank_orientation_angle = -tank->tank_orientation_angle;
                    mirrorRectX(&tank->rect);
                }
                else if (obj->type == PROPDEF_GLASS)
                {
                    ((GlassRecord *)obj)->normal.x = -((GlassRecord *)obj)->normal.x;
                }
                else if (obj->type == PROPDEF_SAFE)
                {
                    ((SafeRecord *)obj)->normal.x = -((SafeRecord *)obj)->normal.x;
                }
            }
        }
    }
}

static void mirrorPlayerState(struct player *player)
{
    s32 i;

    if (player == NULL)
    {
        return;
    }

    player->pos.x = -player->pos.x;
    player->pos2.x = -player->pos2.x;
    player->offset.x = -player->offset.x;
    player->pos3.x = -player->pos3.x;
    player->current_model_pos.x = -player->current_model_pos.x;
    player->previous_model_pos.x = -player->previous_model_pos.x;
    player->current_room_pos.x = -player->current_room_pos.x;
    player->bondprevpos.x = -player->bondprevpos.x;
    player->bondshotspeed.x = -player->bondshotspeed.x;

    for (i = 0; i < 4; i++)
    {
        player->collision_bounds.points[i].x = -player->collision_bounds.points[i].x;
    }

    mirrorCollisionState(&player->previous_collision_info);
    mirrorCollisionState(&player->field_488);

    player->headpos.x = -player->headpos.x;
    player->headlook.x = -player->headlook.x;
    player->headup.x = -player->headup.x;
    player->headpossum.x = -player->headpossum.x;
    player->headlooksum.x = -player->headlooksum.x;
    player->headupsum.x = -player->headupsum.x;
    player->headbodyoffset.x = -player->headbodyoffset.x;
    player->standbodyoffset.x = -player->standbodyoffset.x;
    player->standlook[0].f[0] = -player->standlook[0].f[0];
    player->standlook[1].f[0] = -player->standlook[1].f[0];
    player->standup[0].f[0] = -player->standup[0].f[0];
    player->standup[1].f[0] = -player->standup[1].f[0];

    player->vv_theta = mirrorAngleDegrees(player->vv_theta);
    player->speedtheta = -player->speedtheta;
    player->vv_sintheta = -player->vv_sintheta;

    if (player->prop != NULL)
    {
        player->prop->pos.x = -player->prop->pos.x;

        if (player->prop->chr != NULL)
        {
            mirrorChrState(player->prop->chr);
        }
    }

    if (player->bodyModel != NULL
            && (player->prop == NULL || player->prop->chr == NULL
                || player->prop->chr->model != player->bodyModel))
    {
        setsubroty(player->bodyModel, -getsubroty(player->bodyModel));
    }
}

static void mirrorPlayers(void)
{
    s32 i;

    for (i = 0; i < getPlayerCount() && i < 4; i++)
    {
        mirrorPlayerState(g_playerPointers[i]);
    }
}

static void mirrorSetupTables(void)
{
    PadRecord *pad;
    BoundPadRecord *vol;

    if (g_CurrentSetup.pads != NULL)
    {
        for (pad = g_CurrentSetup.pads; pad->plink != NULL; pad++)
        {
            pad->pos.x = -pad->pos.x;
            pad->look.x = -pad->look.x;
            pad->up.x = -pad->up.x;
        }
    }

    if (g_CurrentSetup.boundpads != NULL)
    {
        for (vol = g_CurrentSetup.boundpads; vol->plink != NULL; vol++)
        {
            f32 oldmin = vol->bbox.xmin;
            f32 oldmax = vol->bbox.xmax;

            vol->pos.x = -vol->pos.x;
            vol->look.x = -vol->look.x;
            vol->up.x = -vol->up.x;
            vol->bbox.xmin = -oldmax;
            vol->bbox.xmax = -oldmin;
        }
    }

    if (g_CurrentSetup.propDefs != NULL)
    {
        PropDefHeaderRecord *def = g_CurrentSetup.propDefs;

        while (def->type != PROPDEF_END)
        {
            if (def->type == PROPDEF_CAMERAPOS)
            {
                CutsceneRecord *camera = (CutsceneRecord *)def;
                camera->pos.x = -camera->pos.x;
                camera->theta = -camera->theta;
            }

            def += sizepropdef(def);
        }
    }

    {
        SetupIntroCamera *camera = g_CurrentSetupIntroCamera;

        while (camera != NULL)
        {
            camera->unk04.fval = -camera->unk04.fval;
            camera->unk10.fval = -camera->unk10.fval;
            camera = camera->prev;
        }
    }
}

void mirrorLevelsStageReset(void)
{
    g_MirrorLevelsEnabled = FALSE;
    g_MirrorLevelsPending = FALSE;
    g_MirrorLevelsPendingEnabled = FALSE;
    g_MirrorSetupApplied = FALSE;
}

void mirrorLevelsStageBegin(s32 enabled)
{
    g_MirrorLevelsEnabled = enabled ? TRUE : FALSE;
    g_MirrorLevelsPending = FALSE;
    g_MirrorLevelsPendingEnabled = g_MirrorLevelsEnabled;
    g_MirrorSetupApplied = FALSE;
}

s32 mirrorLevelsIsEnabled(void)
{
    return g_MirrorLevelsEnabled;
}

s32 mirrorLevelsHasPending(void)
{
    return g_MirrorLevelsPending;
}

void mirrorLevelsApplySetupIfNeeded(void)
{
    if (g_MirrorLevelsEnabled && !g_MirrorSetupApplied)
    {
        mirrorSetupTables();
        g_MirrorSetupApplied = TRUE;
    }
}

void mirrorLevelsSetEnabled(s32 enabled)
{
    enabled = enabled ? TRUE : FALSE;

    if (enabled == g_MirrorLevelsEnabled && !g_MirrorLevelsPending)
    {
        return;
    }

    g_MirrorLevelsPendingEnabled = enabled;
    g_MirrorLevelsPending = TRUE;
}

void mirrorLevelsApplyPending(void)
{
    s32 enabled;

    if (!g_MirrorLevelsPending)
    {
        return;
    }

    enabled = g_MirrorLevelsPendingEnabled;
    g_MirrorLevelsPending = FALSE;

    if (enabled == g_MirrorLevelsEnabled)
    {
        return;
    }

    /* STAN bytes remain untouched.  The collision system changes only how it
     * reads X coordinates, so the same tile pointers/topology remain valid. */
    stanMirrorLevelsSetEnabled(enabled);
    bgMirrorLevelsToggle(enabled);

    if (g_CurrentSetup.pads != NULL || g_CurrentSetup.boundpads != NULL)
    {
        mirrorSetupTables();
        g_MirrorSetupApplied = enabled;
    }

    mirrorRuntimeProps();
    mirrorPlayers();

    g_MirrorLevelsEnabled = enabled;
    osWritebackDCacheAll();
}

void mirrorLevelsPrepareStageUnload(void)
{
    /* Stage teardown is a different problem from a live toggle. At this
     * point bossMainloop has already waited for every graphics task to finish,
     * so it is safe to restore any persistent BG data which the mirror mode
     * changed. More importantly, cleanup code must see canonical props, pads,
     * players and STAN coordinates rather than a half-mirrored world. */
    if (g_MirrorLevelsEnabled)
    {
        stanMirrorLevelsSetEnabled(FALSE);
        bgMirrorLevelsToggle(FALSE);

        if (g_MirrorSetupApplied
                && (g_CurrentSetup.pads != NULL || g_CurrentSetup.boundpads != NULL
                    || g_CurrentSetup.propDefs != NULL || g_CurrentSetupIntroCamera != NULL))
        {
            mirrorSetupTables();
            g_MirrorSetupApplied = FALSE;
        }

        mirrorRuntimeProps();
        mirrorPlayers();

        /* The background display lists and vertices were restored above.
         * Make the canonical bytes visible before the stage memory bank is
         * recycled for the title/mission-report stage. */
        osWritebackDCacheAll();
    }
    else
    {
        /* A queued enable can exist if the player exits before the scheduler
         * reaches the next safe zero-pending-GFX point. Never carry that
         * request across the stage boundary. */
        stanMirrorLevelsSetEnabled(FALSE);
    }

    g_MirrorLevelsEnabled = FALSE;
    g_MirrorLevelsPending = FALSE;
    g_MirrorLevelsPendingEnabled = FALSE;
    g_MirrorSetupApplied = FALSE;
}

#endif
