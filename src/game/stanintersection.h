#ifndef _stanintersection_H_
#define _stanintersection_H_
#include <ultra64.h>
#include "bondtypes.h"

f32 calculateSegmentIntersectionFraction(coord2d *start1, coord2d *end1, coord2d *start2, coord2d *end2);
f32 pointLineDistanceWithRadius(struct coord3d *point3D, coord2d *lineStart, coord2d *lineEnd);
f32 calculateRayToSegmentIntersectionNormalized(coord3d *point3D, coord2d *lineStart, coord2d *lineEnd, coord2d *direction);

#endif
