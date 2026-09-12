#include <ultra64.h>
#include "padhalllv.h"
#include "bondtypes.h"
#include "chrai.h"
#include "random.h"


/**
 * Unreferenced.
 */
waypoint *waypointFindNearestToPos(coord3d *pos, s32 arg1)
{
    waypoint *head;
    PadRecord *pad;
    f32 sqrdist;
    f32 compare;
    waypoint *node;
    waypoint *ret;

    head = g_CurrentSetup.pathwaypoints;
    ret = NULL;

    if (head)
    {
        node = head;
        compare = -1.0f;
        while (node->padID >= 0)
        {
            pad = &g_CurrentSetup.pads[node->padID];
            sqrdist = ((pos->z - pad->pos.z) * (pos->z - pad->pos.z))
                    + ((pos->x - pad->pos.x) * (pos->x - pad->pos.x));

            if ((compare < 0.0f) || (sqrdist < compare))
            {
                compare = sqrdist;
                ret = node;
            }

            node++;
        }
    }

    return ret;
}


/**
 * Unreferenced.
 */
waypoint* waypointRefineNearestToPos(coord3d* pos, s32 arg1, waypoint* arg2)
{
    PadRecord* pad;
    f32 sqrdist2;
    f32 sqrdist;
    s32* neighbor;
    waypoint* node;
    waypoint* ret;
    waypoint* waypoints;

    neighbor = arg2->neighbours;
    ret = arg2;

    if (neighbor != NULL)
    {
        pad = &g_CurrentSetup.pads[arg2->padID];
        sqrdist = ((pos->z - pad->pos.z) * (pos->z - pad->pos.z))
                + ((pos->x - pad->pos.x) * (pos->x - pad->pos.x));
        waypoints = g_CurrentSetup.pathwaypoints;

        while (*neighbor >= 0)
        {
            node = &waypoints[*neighbor];
            pad = &g_CurrentSetup.pads[node->padID];
            sqrdist2 = ((pos->z - pad->pos.z) * (pos->z - pad->pos.z))
                     + ((pos->x - pad->pos.x) * (pos->x - pad->pos.x));

            if (sqrdist2 < sqrdist)
            {
                sqrdist = sqrdist2;
                ret = node;
            }
            neighbor++;
        }
    }
    return ret;
}


waygroup *waygroupFindByDist(s32 *groupnums, s32 value)
{
    waygroup *groups = g_CurrentSetup.waypointgroups;

    while (*groupnums >= 0)
    {
        waygroup *group = &groups[*groupnums];

        if (group->dist == value)
        {
            return group;
        }

        groupnums++;
    }

    return NULL;
}


/**
 * For each group number in the given list assign value to their
 * dist if their dist has no value (ie. is negative).
 */
void waygroupsSetUnvisitedDist(s32 *groupnums, s32 value)
{
    waygroup *groups = g_CurrentSetup.waypointgroups;

    while (*groupnums >= 0)
    {
        waygroup *group = &groups[*groupnums];

        if (group->dist < 0)
        {
            group->dist = value;
        }

        groupnums++;
    }
}


/**
 * Iterate the given groups and find any with a dist matching arg1.
 * For all groups that match, iterate their neighbouring groups and set their
 * dist to arg1 + 1, but only if their they have no existing dist value.
 *
 * Return true if any matched.
 */
bool waygroupsPropagateDist(struct waygroup *group, s32 arg1)
{
    bool result = FALSE;

    while (group->neighbours)
    {
        if (group->dist == arg1)
        {
            result = TRUE;
            waygroupsSetUnvisitedDist(group->neighbours, arg1 + 1);
        }

        group++;
    }

    return result;
}


bool waygroupsFloodDist(struct waygroup *from, struct waygroup *to, struct waygroup *groups, s32 arg3)
{
    bool result = TRUE;
    struct waygroup *group;
    s32 i;

    for (group = groups; group->neighbours; group++)
    {
        group->dist = -1;
    }

    from->dist = 0;

    for (i = 0; (arg3 || to->dist < 0) && result; i++)
    {
        result = waygroupsPropagateDist(groups, i);
    }

    return result;
}


bool waygroupsMarkRoute(waygroup *from, waygroup *to, waygroup *groups)
{
    u32 stack[2];
    bool result = waygroupsFloodDist(from, to, groups, 0);

    if (result)
    {
        waygroup *curto = to;
        s32 i = curto->dist - 1;

        while (i >= 0)
        {
            curto->dist += 10000;
            curto = waygroupFindByDist(curto->neighbours, i);
            i--;
        }

        curto->dist += 10000;
    }

    return result;
}


/**
 * Unreferenced.
 */
s32 waygroupFindRoute(waygroup* from, waygroup* to, waygroup** arr, s32 arrlen)
{
    waygroup **arrptr = arr;
    waygroup *curfrom;
    s32 i;

    if (arrlen >= 2 && g_CurrentSetup.waypointgroups && waygroupsMarkRoute(from, to, g_CurrentSetup.waypointgroups) != 0)
    {
        *arr = from;
        arrptr++;

        curfrom = from;
        arrlen += 9999;
        i = 10001;

        while (i <= to->dist && i < arrlen)
        {
            curfrom = waygroupFindByDist(curfrom->neighbours, i);
            *arrptr = curfrom;
            arrptr++;
            i++;
        }
    }

    *arrptr = NULL;
    arrptr++;

    return arrptr - arr;
}


waypoint *findPadWithDistAndSet(s32 *pointnums, s32 arg1, s32 groupnum)
{
    waypoint *points = g_CurrentSetup.pathwaypoints;

    while (*pointnums >= 0)
    {
        waypoint *point = &points[*pointnums];

        if (point->groupNum == groupnum && point->dist == arg1)
        {
            return point;
        }

        *pointnums++;
    }

    return NULL;
}


void waypointsSetUnvisitedDist(s32 *pointnums, s32 value, s32 groupnum)
{
    waypoint *waypoints = g_CurrentSetup.pathwaypoints;

    while (*pointnums >= 0)
    {
        waypoint *waypoint = &waypoints[*pointnums];

        if (waypoint->groupNum == groupnum && waypoint->dist < 0)
        {
            waypoint->dist = value;
        }

        *pointnums++;
    }
}


bool waypointsPropagateDist(s32 *pointnums, s32 arg1, s32 groupnum)
{
    bool result = FALSE;
    waypoint *points = g_CurrentSetup.pathwaypoints;

    while (*pointnums >= 0)
    {
        waypoint *point = &points[*pointnums];

        if (arg1 == point->dist && point->neighbours)
        {
            result = TRUE;
            waypointsSetUnvisitedDist(point->neighbours, arg1 + 1, groupnum);
        }

        pointnums++;
    }

    return result;
}


void do_BFS_withinPathSet(waypoint *from, waypoint *to, s32 arg2)
{
    waygroup *groups = g_CurrentSetup.waypointgroups;
    waypoint *points = g_CurrentSetup.pathwaypoints;
    waypoint *point;
    s32 *pointnums = groups[from->groupNum].waypoints;
    s32 i;
    bool more;

    while (*pointnums >= 0)
    {
        point = &points[*pointnums];
        point->dist = -1; // reset waypoint
        pointnums++;
    }

    from->dist = 0;

    more = TRUE;

    for (i = 0; (arg2 || to->dist < 0) && more; i++)
    {
        more = waypointsPropagateDist(groups[from->groupNum].waypoints, i, from->groupNum);
    }
}


void waypointMarkRoute(waypoint *from, waypoint *to)
{
    waypoint *curto;
    s32 value;

    do_BFS_withinPathSet(from, to, 0);

    value = to->dist - 1;
    curto = to;

    while (value >= 0)
    {
        curto->dist += 10000;
        curto = findPadWithDistAndSet(curto->neighbours, value, from->groupNum);

        value--;
    }

    curto->dist += 10000;
}


s32 waypointFindRouteInGroup(waypoint *from, waypoint *to, waypoint **arr, s32 arrlen)
{
    waypoint **arrptr = arr;
    waypoint *curfrom;
    s32 i;

    if (arrlen >= 2)
    {
        waypointMarkRoute(from, to);

        *arr = from;
        arrptr++;

        curfrom = from;
        arrlen += 9999;
        i = 10001;

        while (i <= to->dist && i < arrlen)
        {
            curfrom = findPadWithDistAndSet(curfrom->neighbours, i, from->groupNum);
            *arrptr = curfrom;
            arrptr++;
            i++;
        }
    }

    *arrptr = NULL;
    arrptr++;

    return arrptr - arr;
}


void sub_GAME_7F08F438(waygroup *groupa, waygroup *groupb, waypoint **pointa, waypoint **pointb)
{
    waypoint *points = g_CurrentSetup.pathwaypoints;
    waygroup *groups = g_CurrentSetup.waypointgroups;
    s32 *groupapointnums = groupa->waypoints;
    s32 stack;

    while (*groupapointnums >= 0)
    {
        waypoint *groupapoint = &points[*groupapointnums];
        s32 *neighbournums = groupapoint->neighbours;

        while (*neighbournums >= 0)
        {
            waypoint *neighbour = &points[*neighbournums];

            if (groupb == &groups[neighbour->groupNum])
            {
                *pointa = groupapoint;
                *pointb = neighbour;
                return;
            }

            neighbournums++;
        }

        groupapointnums++;
    }
    *pointb = NULL;
    *pointa = NULL;
}


/**
 * Find a route from frompoint to topoint. The arr argument will be populated
 * with pointers to the route's waypoints. If arr is not big enough then only
 * the first part of the route will be populated into the array.
 *
 * The return value is the number of elements populated into the array.
 */
s32 waypointFindRoute(waypoint *frompoint, waypoint *topoint, waypoint **arr, s32 arrlen)
{
    waypoint **arrptr = arr;
    waygroup *groups = g_CurrentSetup.waypointgroups;

    if (groups)
    {
        waygroup *fromgroup = &groups[frompoint->groupNum];
        waygroup *togroup = &groups[topoint->groupNum];

        if (waygroupsMarkRoute(fromgroup, togroup, groups))
        {
            waypoint *curfrompoint = frompoint;
            waygroup *curfromgroup = fromgroup;
            s32 i;

            for (i = fromgroup->dist + 1; i <= togroup->dist && arrlen >= 2; i++)
            {
                s32 numwritten;
                waygroup *nextfromgroup = waygroupFindByDist(curfromgroup->neighbours, i);
                waypoint *tmppoint;
                waypoint *nextfrompoint;

                sub_GAME_7F08F438(curfromgroup, nextfromgroup, &tmppoint, &nextfrompoint);
                numwritten = waypointFindRouteInGroup(curfrompoint, tmppoint, arrptr, arrlen) - 1;

                arrlen -= numwritten;
                arrptr += numwritten;

                curfrompoint = nextfrompoint;
                curfromgroup = nextfromgroup;
            }

            arrptr += waypointFindRouteInGroup(curfrompoint, topoint, arrptr, arrlen) - 1;
        }
    }

    *arrptr = NULL;
    arrptr++;

    return arrptr - arr;
}


// Resets waypoint distance, seems useless since
// do_BFS_withinPathSet already does this for its own
// waypoint group. Even if do_BFS_withinPathSet doesn't
// reset waypoints outside of its own group, is that really needed?
void resetWaypointDistances(void)
{
    waypoint * waypoint;
    waypoint = g_CurrentSetup.pathwaypoints;

    while (waypoint->padID >= 0)
    {
        waypoint->dist = -1;
        waypoint++;
    }
}


waypoint *waypointFindRandomByDist(s32 *pointnums, s32 dist)
{
	s32 len = 0;
	s32 randomindex;
	s32 i;

	while (pointnums[len] >= 0)
    {
		len++;
	}

	// This is effectively randomly dividing the pointnums list into two,
	// then checking the upper portion before the lower portion. Both loops
	// have the same logic so this seems unusual, but there is reason to do
	// this if they want the returned waypoint to be any random waypoint that
	// meets the arg1 criteria, with equal weighting.

	randomindex = randomGetNext() % len;

	for (i = randomindex; i < len; i++)
    {
		waypoint *point = &g_CurrentSetup.pathwaypoints[pointnums[i]];

		if (point->dist == dist)
        {
			return point;
		}
	}

	for (i = 0; i < randomindex; i++)
    {
		waypoint *point = &g_CurrentSetup.pathwaypoints[pointnums[i]];

		if (point->dist == dist)
        {
			return point;
		}
	}

	return NULL;
}


waygroup *waygroupFindRandomByDist(s32 *groupnums, s32 dist)
{
    s32 len = 0;
    s32 randomindex;
    s32 i;

    while (groupnums[len] >= 0)
    {
        len++;
    }

    // Similar to the above function, return a random waygroup
    // which matches the dist criteria.
    randomindex = randomGetNext() % len;

    for (i = randomindex; i < len; i++)
    {
        waygroup *group = &g_CurrentSetup.waypointgroups[groupnums[i]];

        if (group->dist == dist)
        {
            return group;
        }
    }

    for (i = 0; i < randomindex; i++)
    {
        waygroup *group = &g_CurrentSetup.waypointgroups[groupnums[i]];

        if (group->dist == dist)
        {
            return group;
        }
    }

    return NULL;
}


waypoint *waypointFindNextStepToward(waypoint *pointa, waypoint *pointb)
{
    if (g_CurrentSetup.waypointgroups)
    {
        waygroup *groupa = &g_CurrentSetup.waypointgroups[pointa->groupNum];
        waygroup *groupb = &g_CurrentSetup.waypointgroups[pointb->groupNum];
        waypoint *result;
        s32 stack;

        if (groupa == groupb)
        {
            resetWaypointDistances();
            do_BFS_withinPathSet(pointb, pointa, 1);

            result = waypointFindRandomByDist(pointa->neighbours, -1);

            if (result)
            {
                return result;
            }

            result = waypointFindRandomByDist(pointa->neighbours, pointa->dist + 1);

            if (result)
            {
                return result;
            }
        }
        else
        {
            waygroupsFloodDist(groupb, groupa, g_CurrentSetup.waypointgroups, 0);

            if (groupa->dist >= 0)
            {
                waygroup *tmpgroup = waygroupFindRandomByDist(groupa->neighbours, -1);

                if (tmpgroup)
                {
                    waypoint *sp48;
                    waypoint *sp44;
                    waypoint *arr[3];

                    sub_GAME_7F08F438(groupa, tmpgroup, &sp48, &sp44);

                    if (sp48 == pointa)
                    {
                        return sp44;
                    }

                    if (waypointFindRouteInGroup(pointa, sp48, arr, 3) >= 3)
                    {
                        return arr[1];
                    }
                }
                else
                {
                    waygroup *tmpgroup = waygroupFindByDist(groupa->neighbours, groupa->dist - 1);

                    if (tmpgroup)
                    {
                        waypoint *sp30;
                        waypoint *sp2c;

                        sub_GAME_7F08F438(groupa, tmpgroup, &sp30, &sp2c);
                        do_BFS_withinPathSet(sp30, pointa, 1);
                        result = findPadWithDistAndSet(pointa->neighbours, pointa->dist + 1, pointa->groupNum);

                        if (result)
                        {
                            return result;
                        }
                    }
                }
            }
        }
    }

    return NULL;
}


/**
 * Unreferenced
 */
void waypointDebugTestRandomRoute(void)
{
    waypoint* waypoints;
    s32 count;
    waypoint* entry;
    waypoint* from;
    waypoint* to;
    waypoint** arr_entry;
    u32 unused[0x30];

    waypoints = g_CurrentSetup.pathwaypoints;

    if (waypoints != NULL)
    {
        waypoint *arr;
        count = 0;
        entry = waypoints;

        while (entry->padID >= 0)
        {
            entry++;
            count++;
        }

        from = &waypoints[randomGetNext() % count];
        to = &waypoints[randomGetNext() % count];

#ifdef DEBUG
        osSyncPrintf("route from loc number %d to number %d", from->padID, to->padID);
#endif

        if (waypointFindRoute(from, to, &arr, 0x32) != 0)
        {
            for (arr_entry = &arr; *arr_entry; arr_entry++)
            {
#ifdef DEBUG
                osSyncPrintf("%d", arr_entry[0]->padID);
#endif
            }
        }
#ifdef DEBUG
        else
        {
            osSyncPrintf(" not connected");
        }
#endif
    }
#ifdef DEBUG
    osSyncPrintf("\n");
#endif
}
