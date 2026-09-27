#include <ultra64.h>
#include <boss.h>
#include "objective.h"
#include "objective_status.h"
#ifdef GE_MODDED_CHEATS
#include "debugmenu_handler.h"
#endif



/*
 * Clears Objectives and states.
 *
 * NTSC address 0x7F004EC0.
 */
void something_with_stage_objectives(void)
{
    s32 i;

    OBJECTIVESTATUS *start;
    OBJECTIVESTATUS *end;

    if (bossGetStageNum() != LEVELID_TITLE)
    {
        objective_count = -1;
#ifdef GE_MODDED_CHEATS
        /* All Objectives Complete is an action for the current mission, not
         * a persistent toggle.  A fresh stage always starts normally. */
        set_debug_all_obj_complete_flag(FALSE);
#endif

        for (i = 0; i < OBJECTIVES_MAX; i++)
        {
            objectiveStatuses[i] = OBJECTIVESTATUS_INCOMPLETE;
        }
    }

#if defined(VERSION_JP) || defined(VERSION_EU)
    objective_status_display_disabled = FALSE;
#endif

    for (i = 0; i < OBJECTIVES_MAX; i++)
    {
        objective_ptrs[i] = NULL;
    }
    
    ptr_last_tag_entry_type16 = NULL;
    ptr_last_briefing_setup_entry_type23 = NULL;
    ptr_last_enter_room_subobject_entry_type20 = NULL;
    ptr_last_deposit_in_room_subobject_entry_type21 = NULL;
    ptr_last_photo_obj_in_room_subobject_entry_type1E = NULL;
}

void set_parent_cur_tag_entry(struct TagObjectRecord *arg0)
{
    arg0->NextTag = ptr_last_tag_entry_type16;
    ptr_last_tag_entry_type16 = arg0;
}

void setup_briefing_text_entry_parent(struct setup_objective_text *arg0)
{
    arg0->next = ptr_last_briefing_setup_entry_type23;
    ptr_last_briefing_setup_entry_type23 = arg0;
}


/*
* Address: 0x7f004fb8
*/
void add_ptr_to_objective(struct objective_entry* objective)
{
    objective_ptrs[objective->menu] = objective;

    if (objective_count < objective->menu)
    {
        objective_count = objective->menu;
    }
}

void set_parent_cur_obj_enter_room(struct criteria_roomentered *arg0)
{
    arg0->next = ptr_last_enter_room_subobject_entry_type20;
    ptr_last_enter_room_subobject_entry_type20 = arg0;
}

void set_parent_cur_obj_deposited_in_room(struct criteria_deposit *arg0)
{
    arg0->next = ptr_last_deposit_in_room_subobject_entry_type21;
    ptr_last_deposit_in_room_subobject_entry_type21 = arg0;
}


void set_parent_cur_obj_photograph(struct criteria_picture *arg0)
{
    arg0->next = ptr_last_photo_obj_in_room_subobject_entry_type1E;
    ptr_last_photo_obj_in_room_subobject_entry_type1E = arg0;
}

