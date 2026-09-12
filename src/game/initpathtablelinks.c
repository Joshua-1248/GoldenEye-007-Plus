#include <ultra64.h>
#include "chrai.h"
#include "initpathtablelinks.h"
#include "padhalllv.h"


/**
 * NTSC address 0x7F006890.
 * 
 * Ryan spent a few minutes looking at this and left some notes about what this function does:
 *
 * var_s6 is an "error" variable. If set anywhere, the function goes into an infinite loop at the end.
 * The first outer loop appears to be verifying that the waypoints don't list themselves as their own neighbour.
 * The second outer loop appears to do the same but for waygroups. It also calls sub_GAME_7F08F438, which is a more simple version of PD's waypointFindSegmentIntoGroup, so it's likely verifying that there is a waypoint path between this group and each neighbour.
 * The third outer loop is iterating waygroups, then iterating each group's waypoints, and assigning each waypoint's groupNum if it's less than 0. If a value is stored and it's wrong, the error variable is set.
 * The fourth outer loop is checking that all waypoints have a groupNum assigned.
 * The fifth outer loop I'm not sure about without actually decompiling it and naming stuff. It appears to be using the waypoint->dist property temporary so it can validate something. If it doesn't like it then the error variable is set.
 * The final outer loop is the infinite loop which occurs if the error variable is true.
 */

void init_path_table_links(void)
{
    stagesetup *setup;
    waygroup *groups;
    s32 hasError;
    waypoint *waypoints;
    s32 waypointIndex;
    waygroup *currentWaypoint;

    hasError = 0;
    setup = &g_CurrentSetup;
    waypoints = setup->pathwaypoints;
    groups = setup->waypointgroups;

    if (groups);
    if (groups);
    if (groups);

    if (waypoints != NULL)
    {
        s32 neighbourIndex;
        s32 neighbourNum;

        waypointIndex = 0;
        currentWaypoint = waypoints;

        while (((waypoint *) currentWaypoint)->padID >= 0)
        {
            neighbourIndex = 0;

            while ((neighbourNum = ((waypoint *) currentWaypoint)->neighbours[neighbourIndex]) >= 0)
            {
                if (neighbourNum == waypointIndex)
                {
                    if (g_CurrentSetup.padnames != NULL)
                    {
#ifdef DEBUG
                        osSyncPrintf("loc '%s' has a link to itself!\n",
                            g_CurrentSetup.padnames[((waypoint *) currentWaypoint)->padID].p);
#endif
                    }
                    else if (1)
                    {
#ifdef DEBUG
                            osSyncPrintf("loc number %d has a link to itself!\n", waypointIndex);
#endif
                    }
                    hasError = 1;
                }
                else
                {
                    waypoint *linkedWaypoint;
                    s32 reverseIndex;

                    reverseIndex = 0;
                    linkedWaypoint = &waypoints[neighbourNum];

                    while ((linkedWaypoint->neighbours[reverseIndex] >= 0) && (waypointIndex != linkedWaypoint->neighbours[reverseIndex]))
                    {
                        reverseIndex++;
                    }
 
                    if (waypointIndex != linkedWaypoint->neighbours[reverseIndex])
                    {
#ifdef DEBUG
                        if (g_CurrentSetup.padnames == NULL)
                        {
                            osSyncPrintf("loc number %d has link to number %d but not back again!\n",
                                waypointIndex, neighbourNum);
                        }
                        else
                        {
                            osSyncPrintf("loc '%s' has link to '%s' but not back again!\n",
                                g_CurrentSetup.padnames[((waypoint *) currentWaypoint)->padID].p,
                                g_CurrentSetup.padnames[linkedWaypoint->padID].p);
                        }
#endif
                        hasError = 1;
                    }
                }
                neighbourIndex++;
            }
 
            waypointIndex++;
            currentWaypoint = &waypoints[waypointIndex];
        }
 
    }
    {
        waygroup *assignmentGroupCursor;
        waypoint *validationWaypointCursor;
        waygroup *floodGroupCursor;
        s32 changed;
        s32 floodReverseIndex;
        waypoint *connectionWaypoint1;
        waypoint *connectionWaypoint2;
        s32 *member;
        waygroup *floodGroup;
        s32 disconnected;
        waypoint *floodWaypoint;
        s32 floodWaypointNum;
        s32 assignmentMemberOffset;
        s32 memberIndex;
        waypoint *assignmentWaypoint;
        s32 assignmentGroupIndex;
        s32 linkedWaypointNum;
        waypoint *validationWaypoint;

        if (groups != NULL)
        {
            waygroup *linkedGroup;
            waygroup *validationGroup;
            s32 validationGroupIndex;
            s32 groupNeighbourIndex;
            s32 reverseIndex;
            s32 waypointNum;
            waygroup *validationGroupCursors[1];

            validationGroupIndex = 0;
            validationGroup = groups;

            if (validationGroup->neighbours != NULL)
            {
                validationGroupCursors[-3] = groups;

                do
                {
                    groupNeighbourIndex = 0;
                    waypointNum = validationGroup->neighbours[groupNeighbourIndex];

                    while (waypointNum >= 0)
                    {
                        if (waypointNum == validationGroupIndex)
                        {
                            if (g_CurrentSetup.boundpadnames != NULL)
                            {
#ifdef DEBUG
                                osSyncPrintf("hall '%s' has a link to itself!\n", g_CurrentSetup.boundpadnames[validationGroupIndex].p);
#endif
                            }
                            else
                            {
#ifdef DEBUG
                                osSyncPrintf("hall number %d has a link to itself!\n", validationGroupIndex);
#endif
                            }
                            hasError = 1;
                        }
                        else
                        {
                            linkedGroup = &groups[waypointNum];
                            reverseIndex = 0;

                            while ((linkedGroup->neighbours[reverseIndex] >= 0) && (validationGroupIndex != linkedGroup->neighbours[reverseIndex]))
                            {
                                reverseIndex++;

                                if ((!(&g_CurrentSetup)) && (validationGroupIndex + (linkedGroup != 0)))
                                {
                                }
                                if ((!(&g_CurrentSetup)) && (validationGroupIndex + (groupNeighbourIndex != 0)))
                                {
                                }
                            }
 
                            if (validationGroupIndex != linkedGroup->neighbours[reverseIndex])
                            {
                                if (g_CurrentSetup.boundpadnames != NULL)
                                {
#ifdef DEBUG
                                    osSyncPrintf("hall '%s' has link to '%s' but not connected locs!\n",
                                        g_CurrentSetup.boundpadnames[validationGroupIndex].p,
                                        g_CurrentSetup.boundpadnames[waypointNum].p);
#endif
                                }
                                else
                                {
#ifdef DEBUG
                                    osSyncPrintf("hall number %d has link to number %d but not connected locs! \n",
                                        validationGroupIndex, waypointNum);
#endif
                                }
                                hasError = 1;
                            }
                            else
                                if (g_CurrentSetup.pathwaypoints != NULL)
                                {
                                    sub_GAME_7F08F438(validationGroup, linkedGroup, &connectionWaypoint1, &connectionWaypoint2);

                                    if ((connectionWaypoint1 == NULL) || (connectionWaypoint2 == NULL))
                                    {
#ifdef DEBUG
                                        if (g_CurrentSetup.boundpadnames == NULL)
                                        {
                                            osSyncPrintf("hall number %d has link to number %d but not back again!\n",
                                                validationGroupIndex, waypointNum);
                                        }
                                        else
                                        {
                                            osSyncPrintf("hall '%s' has link to '%s' but not back again!\n",
                                                g_CurrentSetup.boundpadnames[validationGroupIndex].p,
                                                g_CurrentSetup.boundpadnames[waypointNum].p);
                                        }
#endif
                                        hasError = 1;
                                    }
                                }
                        }
                        groupNeighbourIndex++;
                        waypointNum = validationGroup->neighbours[groupNeighbourIndex];
                    }
 
                    validationGroupIndex++;
                    validationGroupCursors[-3]++;
                    validationGroup = validationGroupCursors[-3];
                }
                while (validationGroupCursors[-3]->neighbours != NULL);
            }
        }
        if ((waypoints != NULL) && (groups != NULL))
        {
            {
                assignmentGroupIndex = 0;
                floodGroup = groups;
                validationWaypoint = waypoints + hasError * 0;

                if (floodGroup->neighbours != NULL)
                {
                    currentWaypoint = groups;
                    assignmentGroupCursor = floodGroup;

                    do
                    {
                        waypointIndex = 0;

                        if (assignmentGroupCursor);
                        if (assignmentGroupCursor);

                        member = floodGroup->waypoints;

                        while ((*member) >= 0)
                        {
                            assignmentWaypoint = &waypoints[*member];

                            if (assignmentWaypoint->groupNum < 0)
                            {
                                assignmentWaypoint->groupNum = assignmentGroupIndex;
                            }
                            else if (assignmentWaypoint->groupNum != assignmentGroupIndex)
                            {
#ifdef DEBUG
                                    if ((g_CurrentSetup.boundpadnames == NULL) || (g_CurrentSetup.padnames == NULL))
                                    {
                                        osSyncPrintf("hall number %d contains loc number %d which thinks it is in hall number %d!\n",
                                            assignmentGroupIndex, *member, assignmentWaypoint->groupNum);
                                    }
                                    else
                                    {
                                        osSyncPrintf("hall '%s' contains loc '%s' which thinks it is in hall '%s'! \n",
                                            g_CurrentSetup.boundpadnames[assignmentGroupIndex].p,
                                            g_CurrentSetup.padnames[assignmentWaypoint->padID].p,
                                            g_CurrentSetup.boundpadnames[assignmentWaypoint->groupNum].p);
                                    }
#endif
                                    hasError = 1;
                            }

                            waypointIndex += 4;
                            member = (s32 *) (((char *) floodGroup->waypoints) + waypointIndex);
                        }
 
                        currentWaypoint++;
                        floodGroup = currentWaypoint;
                        assignmentGroupIndex++;
                    }
                    while (currentWaypoint->neighbours != NULL);
                }
 
            }

            floodGroup = groups;

            {
                assignmentGroupIndex = 0;
                validationWaypointCursor = validationWaypoint;
                floodGroupCursor = groups;
                currentWaypoint = floodGroupCursor;

                while (validationWaypointCursor->padID >= 0)
                {
                    if (validationWaypointCursor->groupNum < 0)
                    {
#ifdef DEBUG
                        if (g_CurrentSetup.padnames != NULL)
                        {
                            osSyncPrintf("loc '%s' is not in a hall!\n",
                                g_CurrentSetup.padnames[validationWaypointCursor->padID].p);
                        }
                        else
                        {
                            osSyncPrintf("loc number %d is not in a hall!\n", assignmentGroupIndex);
                        }
#endif
                        hasError = 1;
                    }

                    assignmentGroupIndex++;
                    validationWaypointCursor = &waypoints[assignmentGroupIndex];
                }
 
            }

            if (groups->neighbours != NULL)
            {
                do
                {
                    memberIndex = 0;
                    floodWaypointNum = floodGroup->waypoints[memberIndex];

                    while (floodWaypointNum >= 0)
                    {
                        floodWaypoint = &waypoints[floodWaypointNum];

                        if (memberIndex == 0)
                        {
                            floodWaypoint->dist = 1;
                        }
                        else
                        {
                            floodWaypoint->dist = 0;

                            if (((currentWaypoint) && (currentWaypoint)) && (currentWaypoint));
                        }

                        memberIndex++;
                        floodWaypointNum = floodGroup->waypoints[memberIndex];
                    }
 
                    while (1)
                    {
                        memberIndex = 0;
                        floodWaypointNum = floodGroup->waypoints[memberIndex];
                        changed = 0;
                        disconnected = 0;

                        while (floodWaypointNum >= 0)
                        {
                            floodWaypoint = &waypoints[floodWaypointNum];

                            if (floodWaypoint->dist == 1)
                            {
                                floodReverseIndex = 0;
                                linkedWaypointNum = floodWaypoint->neighbours[floodReverseIndex];

                                if (linkedWaypointNum >= 0)
                                {
                                    do
                                    {
                                        assignmentWaypoint = &waypoints[linkedWaypointNum];

                                        if (assignmentWaypoint->dist != 1)
                                        {
                                            assignmentWaypoint->dist = 1;
                                            changed = 1;
                                        }

                                        floodReverseIndex++;
                                        linkedWaypointNum = floodWaypoint->neighbours[floodReverseIndex];
                                    }
                                    while (linkedWaypointNum >= 0);
                                }
                            }

                            memberIndex++;
                            floodWaypointNum = floodGroup->waypoints[memberIndex];
                        }
 
                        memberIndex = 0;
                        floodWaypointNum = floodGroup->waypoints[memberIndex];

                        while (floodWaypointNum >= 0)
                        {
                            memberIndex++;
                            floodWaypoint = &waypoints[floodWaypointNum];

                            if (floodWaypoint->dist != 1)
                            {
                                disconnected = 1;
                                break;
                            }

                            floodWaypointNum = floodGroup->waypoints[memberIndex];
                        }
 
                        if ((!changed) || (!disconnected))
                        {
                            if (((!validationWaypoint) && (!validationWaypoint)) && (!validationWaypoint))
                            {
                            }

                            if (disconnected)
                            {
                                if (g_CurrentSetup.boundpadnames != NULL)
                                {
#ifdef DEBUG
                                    osSyncPrintf("not all locs in hall '%s' are connected!\n",
                                        g_CurrentSetup.boundpadnames[floodGroup - groups].p);
#endif
                                }
                                else
                                {
#ifdef DEBUG
                                    osSyncPrintf("not all locs in hall number %d are connected!\n",
                                        (s32) (floodGroup - groups));
#endif
                                }
                                hasError = 1;
                            }
                            break;
                        }
                    }
 
                    currentWaypoint = ((waygroup *) currentWaypoint) + 1;
                    floodGroup = (waygroup *) currentWaypoint;
                }
                while (((waygroup *) currentWaypoint)->neighbours != NULL);
            }
        }
    }

    if (hasError)
    {
#ifdef DEBUG
        osSyncPrintf("PLEASE FIX THE ABOVE LOC ERRORS NOW! ");
#endif
        while (1)
        {
            // Intentional crash/freeze
        }
 
    }
}
