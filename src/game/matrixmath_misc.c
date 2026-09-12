#include <ultra64.h>
#include "matrixmath.h"

/**
 * Address: 7F05AE00
 * 
 * result = x vector plus ((y - x vector) * scaler).
 * 
 * Standard linear interpolation between two 3D points.
 */
void vec3Lerp(vec3d *x, vec3d *y, f32 scaler, vec3d *result)
{
    result->x = ((y->x - x->x) * scaler) + x->x;
    result->y = ((y->y - x->y) * scaler) + x->y;
    result->z = ((y->z - x->z) * scaler) + x->z;
}


/**
 * Address: 7F05AE50
 * 
 * Catmull-Rom cubic interpolation for a float.
 * 
 * Unused.
 */ 
f32 scalarCatmullRomInterp(f32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4)
{
    f32 cube;
    f32 square;
    f32 total;
    f32 t2;

    square = arg4 * arg4;
    cube   = square * arg4;

    t2     = square - ((arg4 + cube) * 0.5f);
    total  = arg0 * t2;

    t2     = ((1.5f * cube) - (2.5f * square)) + 1.0f;
    total += arg1 * t2;

    t2     = (-1.5f * cube) + (2.0f * square) + (0.5f * arg4);
    total += arg2 * t2;

    t2     = (cube - square) * 0.5f;
    total += arg3 * t2;

    return total;
}


/**
 * Address: 7F05AEFC
 * 
 * Catmull-Rom cubic interpolation for 3D points.
 * The result travels from p1 to p2 depending on the value of fraction. p0 and p3 shape the curve.
 * Fraction range is [0.0, 1.0], outside of that is cubic extrapolation.
 * 
 * Specific to GoldenEye, every few seconds the game picks a small randomzied offset to slightly change
 * where the first person weapon view model points. This function smooths the weapon's drift from one target vector to the next.
 */
void coord3dCatmullRomInterp(coord3d *p0, coord3d *p1, coord3d *p2, coord3d *p3, f32 fraction, coord3d *result)
{
    f32 stack;
    f32 mult0;
    f32 mult1;
    f32 mult2;
    f32 mult3;

    f32 squared = fraction * fraction;
    f32 cubed = fraction * fraction * fraction;

    mult0 = squared - 0.5f * (fraction + cubed);
    mult1 = 1.5f * cubed - 2.5f * squared + 1.0f;
    mult2 = -1.5f * cubed + 2.0f * squared + 0.5f * fraction;
    mult3 = 0.5f * (cubed - squared);

    result->x = mult0 * p0->f[0] + mult1 * p1->f[0] + mult2 * p2->f[0] + mult3 * p3->f[0];
    result->y = mult0 * p0->f[1] + mult1 * p1->f[1] + mult2 * p2->f[1] + mult3 * p3->f[1];
    result->z = mult0 * p0->f[2] + mult1 * p1->f[2] + mult2 * p2->f[2] + mult3 * p3->f[2];
}


/**
 * Address: 7F05B024
 * 
 * Interpolation from 'start' to 'end' using a cubic Hermite spline segment. 'fraction' is the position along the segment with
 * 0.0 being at the start and 1.0 being at the end. 
 * 
 * 'prev' and 'next' act as control points used to derive the tangent direction at 'start and 'end'
 * 
 * tangentScale affects how strongly the control points affect the curve.
 * 0.0f = no influence from 'prev' and 'next'
 * >0.5f = stronger tangents, looser curve
 * <0.5f = weaker tangents, tighter/flatter curve
 * 
 * This function is used to move the camera smoothly between waypoints in the intro camera swirl.
 */
void coord3dCubicSplineInterp(coord3d *prev, coord3d *start, coord3d *end, coord3d *next, f32 fraction, f32 tangentScale, coord3d *result)
{
    f32 square;
    f32 scale;
    f32 cube;
    f32 temp_f16;
    f32 temp_f18;
    f32 temp_f6;
    f32 temp_f8;

    square = fraction * fraction;
    scale = tangentScale;
    cube = square * fraction;

    temp_f16 = ((2.0f * square) - (fraction + cube)) * scale;
    temp_f18 = (((2.0f - scale) * cube) + (square * (scale - 3.0f))) + 1.0f;
    temp_f8 = (((scale - 2.0f) * cube) + (square * (3.0f - (2.0f * scale)))) + (fraction * scale);

    result->f[0] = (((temp_f16 * prev->f[0]) + (temp_f18 * start->f[0])) + (temp_f8 * end->f[0])) + (((cube - square) * tangentScale) * next->f[0]);
    result->f[1] = (((temp_f16 * prev->f[1]) + (temp_f18 * start->f[1])) + (temp_f8 * end->f[1])) + (((cube - square) * tangentScale) * next->f[1]);
    result->f[2] = (((temp_f16 * prev->f[2]) + (temp_f18 * start->f[2])) + (temp_f8 * end->f[2])) + (((cube - square) * tangentScale) * next->f[2]);
}


/**
 * Address: 7F05B154
 * 
 * Unused.
 */ 
f32 scalarCubicHermiteInterp(f32 arg0, f32 arg1, f32 arg2, f32 arg3, f32 arg4)
{
    /*
    Quick substition (so this may be wrong), but let x=arg4. Then
    return
    x^3 * (arg3 + arg2 - 2*arg1 + 2*arg0) + x^2 * (-arg3 - 2*arg2 + 3*arg1 - 3*arg0) + x^1 * (arg2) + x^0 * (arg0)
    */
    f32 cube;
    f32 temp_f18;
    f32 square;

    square = arg4 * arg4;
    cube = square * arg4;
    temp_f18 = ((2.0f * cube) - (3.0f * square)) + 1.0f;
    return (arg0 * temp_f18) + (arg1 * (1.0f - temp_f18)) + (arg2 * ((cube - (2.0f * square)) + arg4)) + (arg3 * (cube - square));
}

