#include "matrixmath.h"
#include "math_atan2f.h"
#include <PR/gu.h>
#include <math.h>

/* Avoid Gimble Lock? */
#define EPSILON FLT_EPSILON * 16

// bss
//CODE.bss:80075DA0
f32 flt_CODE_bss_80075DA0;


// data
//D:80032310
f32 D_80032310[2] = {M_U16_MAX_VALUE_F, M_U16_MAX_VALUE_F};



//#ifdef VERSION_EU

#ifdef VERSION_EU
void matrix_4x4_copy_homogeneous_eu(f32 src[3][3], f32 dst[3][3])
{
    s32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst[i][j] = src[i][j];
        }
    }
}
#endif


#ifdef VERSION_EU
void matrix_4x4_multiply_homogeneous_in_place_eu (f32 lhs[3][3], f32 rhs[3][3])
{
    f32 result[3][3];
    matrix_4x4_multiply_homogeneous_eu(lhs, rhs, &result);
    matrix_4x4_copy_homogeneous_eu(&result, rhs);
}
#endif


#ifdef VERSION_EU
void matrix_4x4_multiply_homogeneous_eu(f32 lhs[3][3], f32 rhs[3][3], f32 result[3][3])
{
    s32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            result[j][i] = (lhs[0][i] * rhs[j][0]) + (lhs[1][i] * rhs[j][1]) + (lhs[2][i] * rhs[j][2]);
        }
    }
}
#endif


#ifdef VERSION_EU
void matrix_4x4_copy_eu(f32 src[][3], f32 dst[4][4])
{
    s32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst[i][j] = src[i][j];
        }
    }

    dst[0][3] = 0.0;
    dst[1][3] = 0.0;
    dst[2][3] = 0.0;
    dst[3][0] = 0.0;
    dst[3][1] = 0.0;
    dst[3][2] = 0.0;
    dst[3][3] = 1.0;
}
#endif

#ifdef VERSION_EU
void matrix_7f05842c_eu(f32 src[][4], f32 dst[3][3])
{
    s32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            dst[i][j] = src[i][j];
        }
    }
}
#endif


//nexthere

//#endif

void matrix_4x4_set_identity(Mtxf *matrix)
{
    matrix->m[0][0] = 1.0f;
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = 1.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = 1.0f;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][2] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

void matrix_4x4_copy(Mtxf *src, Mtxf *dst)
{
    s32 i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            dst->m[i][j] = src->m[i][j];
        }
    }
}

void matrix_4x4_multiply_in_place(Mtxf *lhs, Mtxf *rhs)
{
    Mtxf result;
    matrix_4x4_multiply(lhs, rhs, &result);
    matrix_4x4_copy(&result, rhs);
}

void matrix_4x4_multiply_homogeneous_in_place(Mtxf *lhs, Mtxf *rhs)
{
    Mtxf result;
    matrix_4x4_multiply_homogeneous(lhs, rhs, &result);
    matrix_4x4_copy(&result, rhs);
}

void matrix_4x4_multiply(Mtxf *lhs, Mtxf *rhs, Mtxf *result)
{
    s32 i, j;
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            result->m[j][i] = lhs->m[0][i] * rhs->m[j][0] + lhs->m[1][i] * rhs->m[j][1] + lhs->m[2][i] * rhs->m[j][2] + lhs->m[3][i] * rhs->m[j][3];
        }
    }
}

void matrix_4x4_multiply_homogeneous(Mtxf *lhs, Mtxf *rhs, Mtxf *result)
{
    s32 i, j;
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 4; j++)
        {
            result->m[j][i] = (lhs->m[0][i] * rhs->m[j][0]) + (lhs->m[1][i] * rhs->m[j][1]) + (lhs->m[2][i] * rhs->m[j][2]);
            if (j == 3)
            {
                result->m[j][i] += lhs->m[3][i];
            }
        }
    }

    result->m[0][3] = 0.0f;
    result->m[1][3] = 0.0f;
    result->m[2][3] = 0.0f;
    result->m[3][3] = 1.0f;
}

void matrix_4x4_7F058274(Mtxf *arg0, Mtxf *arg1, Mtxf *arg2)
{
    arg2->m[0][0] = (arg0->m[0][0] * arg1->m[0][0]);
    arg2->m[1][0] = (arg0->m[0][0] * arg1->m[1][0]);
    arg2->m[2][0] = (arg0->m[0][0] * arg1->m[2][0]);
    arg2->m[3][0] = (arg0->m[0][0] * arg1->m[3][0]);
    arg2->m[0][1] = (arg0->m[1][1] * arg1->m[0][1]);
    arg2->m[1][1] = (arg0->m[1][1] * arg1->m[1][1]);
    arg2->m[2][1] = (arg0->m[1][1] * arg1->m[2][1]);
    arg2->m[3][1] = (arg0->m[1][1] * arg1->m[3][1]);
    arg2->m[0][2] = (arg0->m[2][2] * arg1->m[0][2]);
    arg2->m[1][2] = (arg0->m[2][2] * arg1->m[1][2]);
    arg2->m[2][2] = (arg0->m[2][2] * arg1->m[2][2]);
    arg2->m[3][2] = (arg0->m[2][2] * arg1->m[3][2]) + arg0->m[3][2];
    arg2->m[0][3] = (arg0->m[2][3] * arg1->m[0][2]);
    arg2->m[1][3] = (arg0->m[2][3] * arg1->m[1][2]);
    arg2->m[2][3] = (arg0->m[2][3] * arg1->m[2][2]);
    arg2->m[3][3] = (arg0->m[2][3] * arg1->m[3][2]);
}

void matrix_4x4_rotate_vector(Mtxf *matrix, struct coord3d *vector, struct coord3d *result)
{
    s32 i;
    for (i = 0; i < 3; i++)
    {
        result->f[i] = matrix->m[0][i] * vector->f[0] + matrix->m[1][i] * vector->f[1] + matrix->m[2][i] * vector->f[2];
    }
}

void mtx4RotateVecInPlace(Mtxf *matrix, struct coord3d *vector)
{
    struct coord3d result;
    matrix_4x4_rotate_vector(matrix, vector, &result);
    vector->f[0] = result.f[0];
    vector->f[1] = result.f[1];
    vector->f[2] = result.f[2];
}

void matrix_4x4_transform_vector(Mtxf *matrix, struct coord3d *vector, struct coord3d *result)
{
    matrix_4x4_rotate_vector(matrix, vector, result);
    result->f[0] += matrix->m[3][0];
    result->f[1] += matrix->m[3][1];
    result->f[2] += matrix->m[3][2];
}

void mtx4TransformVecInPlace(Mtxf *matrix, struct coord3d *vector)
{
    mtx4RotateVecInPlace(matrix, vector);
    vector->f[0] += matrix->m[3][0];
    vector->f[1] += matrix->m[3][1];
    vector->f[2] += matrix->m[3][2];
}

void matrix_4x4_set_position_and_rotation_around_y(f32 *position, f32 angle, Mtxf *matrix)
{
    f32 cosine      = cosf(angle);
    f32 sine        = sinf(angle);
    matrix->m[0][0] = cosine;
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = -sine;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = 1.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = sine;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = cosine;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = position[0];
    matrix->m[3][1] = position[1];
    matrix->m[3][2] = position[2];
    matrix->m[3][3] = 1.0f;
}

void matrix_4x4_set_rotation_around_x(f32 angle, Mtxf *matrix)
{
    f32 cosine      = cosf(angle);
    f32 sine        = sinf(angle);
    matrix->m[0][0] = 1.0f;
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = cosine;
    matrix->m[1][2] = sine;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[2][1] = -sine;
    matrix->m[2][2] = cosine;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][2] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

void matrix_4x4_set_rotation_around_y(f32 angle, Mtxf *matrix)
{
    f32 cosine      = cosf(angle);
    f32 sine        = sinf(angle);
    matrix->m[0][0] = cosine;
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = -sine;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = 1.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = sine;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = cosine;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][2] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

void matrix_4x4_set_rotation_around_z(f32 angle, Mtxf *matrix)
{
    f32 cosine      = cosf(angle);
    f32 sine        = sinf(angle);
    matrix->m[0][0] = cosine;
    matrix->m[0][1] = sine;
    matrix->m[0][2] = 0.0f;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = -sine;
    matrix->m[1][1] = cosine;
    matrix->m[1][2] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = 1.0f;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][2] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

void matrix_4x4_set_rotation_around_xyz(struct coord3d *angles, Mtxf *matrix)
{
    f32 cos_x       = cosf(angles->f[0]);
    f32 sin_x       = sinf(angles->f[0]);
    f32 cos_y       = cosf(angles->f[1]);
    f32 sin_y       = sinf(angles->f[1]);
    f32 cos_z       = cosf(angles->f[2]);
    f32 sin_z       = sinf(angles->f[2]);
    f32 sin_x_sin_z = sin_x * sin_z;
    f32 cos_x_sin_z = cos_x * sin_z;
    f32 sin_x_cos_z = sin_x * cos_z;
    f32 cos_x_cos_z = cos_x * cos_z;
    matrix->m[0][0] = (cos_y * cos_z);
    matrix->m[0][1] = (cos_y * sin_z);
    matrix->m[0][2] = -sin_y;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][0] = ((sin_x_cos_z * sin_y) - cos_x_sin_z);
    matrix->m[1][1] = ((sin_x_sin_z * sin_y) + cos_x_cos_z);
    matrix->m[1][2] = sin_x * cos_y;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][0] = ((cos_x_cos_z * sin_y) + sin_x_sin_z);
    matrix->m[2][1] = ((cos_x_sin_z * sin_y) - sin_x_cos_z);
    matrix->m[2][2] = cos_x * cos_y;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[3][2] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

// https://stackoverflow.com/a/15029416
void matrix_4x4_get_rotation_around_xyz(Mtxf *matrix, struct coord3d *angles)
{
    f32 norm;
    f32 sin_x_cos_y = matrix->m[1][2];
    f32 cos_x_cos_y = matrix->m[2][2];
    norm            = sqrtf(SQR(sin_x_cos_y) + SQR(cos_x_cos_y));
    if (EPSILON < norm)
    {
        angles->f[0] = atan2f(matrix->m[1][2], matrix->m[2][2]);
        angles->f[1] = atan2f(-matrix->m[0][2], norm);
        angles->f[2] = atan2f(matrix->m[0][1], matrix->m[0][0]);
    }
    else
    {
        angles->f[0] = 0.0f;
        angles->f[1] = atan2f(-matrix->m[0][2], norm);
        angles->f[2] = atan2f(-matrix->m[1][0], matrix->m[1][1]);
    }
}

void matrix_4x4_set_position_and_rotation_around_xyz(struct coord3d *position, struct coord3d * rotation, Mtxf *matrix)
{
    matrix_4x4_set_rotation_around_xyz(rotation, matrix);
    matrix_4x4_set_position(position, matrix);
}

void matrix_4x4_set_identity_and_position(struct coord3d * position, Mtxf *matrix)
{
    matrix_4x4_set_identity(matrix);
    matrix_4x4_set_position(position, matrix);
}

void matrix_4x4_set_position(struct coord3d *position, Mtxf *matrix)
{
    matrix->m[3][0] = position->f[0];
    matrix->m[3][1] = position->f[1];
    matrix->m[3][2] = position->f[2];
}

void matrix_column_1_scalar_multiply(f32 scalar, f32 *matrix)
{
    matrix[0] *= scalar;
    matrix[1] *= scalar;
    matrix[2] *= scalar;
}

void matrix_column_2_scalar_multiply(f32 scalar, f32 *matrix)
{
    matrix[4] *= scalar;
    matrix[5] *= scalar;
    matrix[6] *= scalar;
}

void matrix_column_3_scalar_multiply(f32 scalar, f32 *matrix)
{
    matrix[8] *= scalar;
    matrix[9] *= scalar;
    matrix[10] *= scalar;
    matrix[11] *= scalar;
}

void matrix_column_3_scalar_multiply_2(f32 scalar, f32 *matrix)
{
    matrix[8] *= scalar;
    matrix[9] *= scalar;
    matrix[10] *= scalar;
}

void matrix_scalar_multiply(f32 scalar, f32 *matrix)
{
    matrix[0] *= scalar;
    matrix[1] *= scalar;
    matrix[2] *= scalar;
    matrix[3] *= scalar;
    matrix[4] *= scalar;
    matrix[5] *= scalar;
    matrix[6] *= scalar;
    matrix[7] *= scalar;
    matrix[8] *= scalar;
    matrix[9] *= scalar;
    matrix[10] *= scalar;
    matrix[11] *= scalar;
}

void matrix_scalar_multiply_2(f32 scalar, f32 *matrix)
{
    matrix[0] *= scalar;
    matrix[1] *= scalar;
    matrix[2] *= scalar;
    matrix[4] *= scalar;
    matrix[5] *= scalar;
    matrix[6] *= scalar;
    matrix[8] *= scalar;
    matrix[9] *= scalar;
    matrix[10] *= scalar;
}

void matrix_row_3_scalar_multiply(f32 scalar, f32 *matrix)
{
    matrix[2] *= scalar;
    matrix[6] *= scalar;
    matrix[10] *= scalar;
    matrix[14] *= scalar;
}

void matrix_scalar_multiply_3(f32 scalar, f32 *matrix)
{
    matrix[0] *= scalar;
    matrix[4] *= scalar;
    matrix[8] *= scalar;
    matrix[12] *= scalar;
    matrix[1] *= scalar;
    matrix[5] *= scalar;
    matrix[9] *= scalar;
    matrix[13] *= scalar;
    matrix[2] *= scalar;
    matrix[6] *= scalar;
    matrix[10] *= scalar;
    matrix[14] *= scalar;
}

void matrix_4x4_7F058C4C(f32 arg0)
{
    D_80032310[0] = (M_U16_MAX_VALUE_F * arg0);
}

void matrix_4x4_7F058C64(void)
{
    flt_CODE_bss_80075DA0 = D_80032310[0];
    D_80032310[0]         = M_U16_MAX_VALUE_F;
}

void matrix_4x4_7F058C88(void)
{
    D_80032310[0] = flt_CODE_bss_80075DA0;
}

/*
 * Address: 0x7F058C9C
*/
void matrix_4x4_f32_to_s32(f32 mf[4][4], s32 ms[4][4])
{
    f32 *src = (f32 *)mf;
    s32 *dst = (s32 *)ms;
    s32 i;

    for (i = 0; i < 8; i += 1)
    {
        s32 e1, e2;

        e1 = (s32)(src[((i + 0) << 1) + 0] * D_80032310[0]);
        e2 = (s32)(src[((i + 0) << 1) + 1] * D_80032310[(i + 0) & 1]);
        dst[(i + 0) + 0] = (e1 & 0xffff0000) | (((u32)e2) >> 16);
        dst[(i + 0) + 8] = (e1 << 16) | (e2 & 0xffff);
    }
}

/*
 * Address: 0x7F058E78
 * Reads packed s32 words from src, writes floats to dst (inverse of matrix_4x4_f32_to_s32).
*/
void sub_GAME_7F058E78(Mtxf *src, Mtxf *dst) {
    u32 *srcwords = (u32 *) src;
    f32 *dstfloats = (f32 *) dst;
    s32 i;

    for (i = 0; i < 8; i++) {
        u32 word1 = srcwords[i + 0];
        u32 word2 = srcwords[i + 8];

        dstfloats[(i << 1) + 0] = (s32) ((word1 & 0xffff0000) | (word2 >> 16)) / D_80032310[0];
        dstfloats[(i << 1) + 1] = (s32) ((word1 << 16) | (word2 & 0xffff)) / D_80032310[i & 1];
    }
}

void matrix_4x4_7F059044(Mtxf *arg0, Mtx* arg1) {
    s32 i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            arg1->m[i][j] = arg0->m[i][j] * M_U16_MAX_VALUE_F;
        }
    }
}

void matrix_4x4_7F05914C(Mtx* arg0, Mtxf *arg1) {
    s32 i, j;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            arg1->m[i][j] = arg0->m[i][j] / M_U16_MAX_VALUE_F;
        }
    }
}

/*
 * Address: 0x7F059244
*/
void sub_GAME_7F059244(Mtx *src, Mtx *dst) {
    u32 *srcwords = (u32 *) src;
    u32 *dstwords = (u32 *) dst;
    s32 i;

    for (i = 0; i < 8; i++) {
        u32 word1 = srcwords[i << 1];
        u32 word2 = srcwords[(i << 1) + 1];

        dstwords[i + 0] = (word1 & 0xffff0000) | (word2 >> 16);
        dstwords[i + 8] = (word1 << 16) | (word2 & 0xffff);
    }
}


/*
 * Address: 0x7F059334
*/
void sub_GAME_7F059334(Mtx *src, Mtx *dst) {
    u32 *srcwords = (u32 *) src;
    u32 *dstwords = (u32 *) dst;
    s32 i;

    for (i = 0; i < 8; i++) {
        u32 word1 = srcwords[i + 0];
        u32 word2 = srcwords[i + 8];

        dstwords[(i << 1) + 0] = (word1 & 0xffff0000) | (word2 >> 16);
        dstwords[(i << 1) + 1] = (word1 << 16) | (word2 & 0xffff);
    }
}

/*
 * Address: 0x7F059424
*/
void matrix_4x4_set_lookat(Mtxf *matrix, f32 eye_x, f32 eye_y, f32 eye_z, f32 forward_x, f32 forward_y, f32 forward_z, f32 up_x, f32 up_y, f32 up_z) {
    f32 right_x;
    f32 right_y;
    f32 norm_right;
    f32 norm_up;
    f32 right_z;

    // Normalize forward vector
    f32 norm_forward = -1.0f / sqrtf((forward_x * forward_x) + (forward_y * forward_y) + (forward_z * forward_z));
    forward_x *= norm_forward;
    forward_y *= norm_forward;
    forward_z *= norm_forward;

    // Compute right vector (cross product of up and forward)
    right_x = (up_y * forward_z) - (up_z * forward_y);
    right_y = (up_z * forward_x) - (up_x * forward_z);
    right_z = (up_x * forward_y) - (up_y * forward_x);

    // Normalize right vector
    norm_right = 1.0f / sqrtf((right_x * right_x) + (right_y * right_y) + (right_z * right_z));
    right_x *= norm_right;
    right_y *= norm_right;
    right_z *= norm_right;

    // Recompute up vector (cross product of forward and right)
    up_x = (forward_y * right_z) - (forward_z * right_y);
    up_y = (forward_z * right_x) - (forward_x * right_z);
    up_z = (forward_x * right_y) - (forward_y * right_x);

    // Normalize up vector
    norm_up = 1.0f / sqrtf((up_x * up_x) + (up_y * up_y) + (up_z * up_z));
    up_x *= norm_up;
    up_y *= norm_up;
    up_z *= norm_up;

    // Set matrix columns
    matrix->m[0][0] = right_x;
    matrix->m[1][0] = right_y;
    matrix->m[2][0] = right_z;
    matrix->m[3][0] = -((eye_x * right_x) + (eye_y * right_y) + (eye_z * right_z));
    matrix->m[0][1] = up_x;
    matrix->m[1][1] = up_y;
    matrix->m[2][1] = up_z;
    matrix->m[3][1] = -((eye_x * up_x) + (eye_y * up_y) + (eye_z * up_z));
    matrix->m[0][2] = forward_x;
    matrix->m[1][2] = forward_y;
    matrix->m[2][2] = forward_z;
    matrix->m[3][2] = -((eye_x * forward_x) + (eye_y * forward_y) + (eye_z * forward_z));
    matrix->m[0][3] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

/*
 * Address: 0x7F059694
*/
void matrix_4x4_set_lookat_target(Mtxf *matrix, f32 eye_x, f32 eye_y, f32 eye_z, f32 target_x, f32 target_y, f32 target_z, f32 up_x, f32 up_y, f32 up_z) {
    matrix_4x4_set_lookat(matrix, eye_x, eye_y, eye_z, target_x - eye_x, target_y - eye_y, target_z - eye_z, up_x, up_y, up_z);
}

/*
 * Address: 0x7F059708
*/
void matrix_4x4_set_basis_and_position(Mtxf *matrix, f32 pos_x, f32 pos_y, f32 pos_z, f32 basis_x, f32 basis_y, f32 basis_z, f32 up_x, f32 up_y, f32 up_z) {
    f32 right_x;
    f32 right_y;
    f32 norm_right;
    f32 norm_up;
    f32 right_z;
    f32 norm_basis = -1.0f / sqrtf((basis_x * basis_x) + (basis_y * basis_y) + (basis_z * basis_z));
    basis_x *= norm_basis;
    basis_y *= norm_basis;
    basis_z *= norm_basis;
    right_x = (up_y * basis_z) - (up_z * basis_y);
    right_y = (up_z * basis_x) - (up_x * basis_z);
    right_z = (up_x * basis_y) - (up_y * basis_x);
    norm_right = 1.0f / sqrtf((right_x * right_x) + (right_y * right_y) + (right_z * right_z));
    right_x *= norm_right;
    right_y *= norm_right;
    right_z *= norm_right;
    up_x = (basis_y * right_z) - (basis_z * right_y);
    up_y = (basis_z * right_x) - (basis_x * right_z);
    up_z = (basis_x * right_y) - (basis_y * right_x);
    norm_up = 1.0f / sqrtf((up_x * up_x) + (up_y * up_y) + (up_z * up_z));
    up_x *= norm_up;
    up_y *= norm_up;
    up_z *= norm_up;
    matrix->m[0][0] = right_x;
    matrix->m[1][0] = up_x;
    matrix->m[2][0] = basis_x;
    matrix->m[3][0] = pos_x;
    matrix->m[0][1] = right_y;
    matrix->m[1][1] = up_y;
    matrix->m[2][1] = basis_y;
    matrix->m[3][1] = pos_y;
    matrix->m[0][2] = right_z;
    matrix->m[1][2] = up_z;
    matrix->m[2][2] = basis_z;
    matrix->m[3][2] = pos_z;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[2][3] = 0.0f;
    matrix->m[3][3] = 1.0f;
}

/*
 * Address: 0x7F059908
*/
void matrix_4x4_set_basis_and_position_target(Mtxf *matrix, f32 pos_x, f32 pos_y, f32 pos_z, f32 target_x, f32 target_y, f32 target_z, f32 up_x, f32 up_y, f32 up_z) {
    matrix_4x4_set_basis_and_position(matrix, pos_x, pos_y, pos_z, target_x - pos_x, target_y - pos_y, target_z - pos_z, up_x, up_y, up_z);
}


/*
 * Address: 0x7F05997C
*/
u32 matrix_4x4_calc_depth_scale(f32 near, f32 far)
{
    f32 sum = near + far;
    u16 result;

    if (sum <= 2)
    {
        result = 0xffff;
    }
    else
    {
        result = 0x20000 / sum;

        if (result <= 0)
        {
            result = 1;
        }
    }

    return result;
}

/*
 * Address: 0x7F059A48
*/
void matrix_4x4_set_projection(Mtxf *matrix, u16* depth_scale, f32 fovy, f32 aspect, f32 near, f32 far, f32 scale)
{
    f32 temp = cosf(fovy * 0.5f) / sinf(fovy * 0.5f);
    scale *= M_U16_MAX_VALUE_F;
    matrix->m[0][0] = ((temp / aspect) * scale);
    matrix->m[1][1] = (temp * scale);
    matrix->m[1][0] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[3][0] = 0.0f;
    matrix->m[0][1] = 0.0f;
    matrix->m[2][1] = 0.0f;
    matrix->m[3][1] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[2][2] = (((near + far) / (near - far)) * scale);
    matrix->m[3][2] = ((((near + near) * far) / (near - far)) * scale);
    matrix->m[2][3] = -scale;
    matrix->m[0][3] = 0.0f;
    matrix->m[1][3] = 0.0f;
    matrix->m[3][3] = 0.0f;
    if (depth_scale != 0) {
        *depth_scale = matrix_4x4_calc_depth_scale(near, far);
    }
}

/*
 * Address: 0x7F059B58
*/
void matrix_4x4_set_rotation_axis_angle(Mtxf *matrix, f32 angle, f32 x, f32 y, f32 z) 
{
    f32 sine;
    f32 cosine;
    f32 norm;
    f32 invnorm;
    f32 cos_x;
    f32 sin_x;
    f32 cos_z;
    f32 sin_z;
    guNormalize(&x, &y, &z);
    sine = sinf(angle);
    cosine = cosf(angle);
    norm = sqrtf((x * x) + (z * z));
    if (norm != 0.0f) {
        cos_x = x * cosine;
        sin_x = x * sine;
        cos_z = z * cosine;
        sin_z = z * sine;
        invnorm = 1.0f / norm;
        matrix->m[0][0] = ((-cos_z - (y * sin_x)) * invnorm);
        matrix->m[1][0] = (sine * norm);
        matrix->m[2][0] = ((cos_x - (y * sin_z)) * invnorm);
        matrix->m[3][0] = 0.0f;
        matrix->m[0][1] = ((sin_z - (y * cos_x)) * invnorm);
        matrix->m[1][1] = (cosine * norm);
        matrix->m[2][1] = ((-sin_x - (y * cos_z)) * invnorm);
        matrix->m[3][1] = 0.0f;
        matrix->m[0][2] = -x;
        matrix->m[1][2] = -y;
        matrix->m[2][2] = -z;
        matrix->m[3][2] = 0.0f;
        matrix->m[0][3] = 0.0f;
        matrix->m[1][3] = 0.0f;
        matrix->m[2][3] = 0.0f;
        matrix->m[3][3] = 1.0f;
        return;
    }
    matrix_4x4_set_identity(matrix);
}


void matrix_4x4_align(Mtxf *matrix, f32 angle, f32 x, f32 y, f32 z) 
{
    angle = RadToDeg(angle);
    guAlignF(matrix->m, angle, x, y, z);
}

// DEBUG prints a 4x4 matrix
void matrix_4x4_7F059D30(Mtxf *matrix)
{
#ifdef DEBUG
    int i, j;

    for (i = 0; i < 4; i++)
    {
        osSyncPrintf("(");
        for (j = 0; j < 4; j++)
        {
            osSyncPrintf(" %9f", matrix->m[i][j]);
        }
        osSyncPrintf(" )\n");
    }
#endif
    return;
}

void matrix_4x4_set_rotation_inverse(Mtxf *rotation, Mtxf *transpose) 
{
    transpose->m[0][0] = rotation->m[0][0];
    transpose->m[0][1] = rotation->m[1][0];
    transpose->m[0][2] = rotation->m[2][0];
    transpose->m[1][0] = rotation->m[0][1];
    transpose->m[1][1] = rotation->m[1][1];
    transpose->m[1][2] = rotation->m[2][1];
    transpose->m[2][0] = rotation->m[0][2];
    transpose->m[2][1] = rotation->m[1][2];
    transpose->m[2][2] = rotation->m[2][2];
    transpose->m[3][0] = 0.0f;
    transpose->m[3][1] = 0.0f;
    transpose->m[3][2] = 0.0f;
    transpose->m[0][3] = 0.0f;
    transpose->m[1][3] = 0.0f;
    transpose->m[2][3] = 0.0f;
    transpose->m[3][3] = 1.0f;
}

/*
 * Address: 0x7F059DAC
*/
void matrix_4x4_normalize_rotation(Mtxf *matrix, Mtxf *result) {
    f32 norm = ((matrix->m[0][0] * matrix->m[0][0]) +
                (matrix->m[1][0] * matrix->m[1][0]) +
                (matrix->m[2][0] * matrix->m[2][0]));
    norm = 1.0f / norm;
    result->m[0][0] = (matrix->m[0][0] * norm);
    result->m[0][1] = (matrix->m[1][0] * norm);
    result->m[0][2] = (matrix->m[2][0] * norm);
    result->m[1][0] = (matrix->m[0][1] * norm);
    result->m[1][1] = (matrix->m[1][1] * norm);
    result->m[1][2] = (matrix->m[2][1] * norm);
    result->m[2][0] = (matrix->m[0][2] * norm);
    result->m[2][1] = (matrix->m[1][2] * norm);
    result->m[2][2] = (matrix->m[2][2] * norm);
    result->m[3][0] = 0.0f;
    result->m[3][1] = 0.0f;
    result->m[3][2] = 0.0f;
    result->m[0][3] = 0.0f;
    result->m[1][3] = 0.0f;
    result->m[2][3] = 0.0f;
    result->m[3][3] = 1.0f;
}

/*
 * Address: 0x7F059E64
*/
void matrix_4x4_set_inverse_rotation_and_translation(Mtxf *matrix, Mtxf *result) {
    f32 norm = (matrix->m[0][0] * matrix->m[0][0]) +
               (matrix->m[1][0] * matrix->m[1][0]) +
               (matrix->m[2][0] * matrix->m[2][0]);
    norm = 1.0f / norm;
    result->m[0][0] = (matrix->m[0][0] * norm);
    result->m[0][1] = (matrix->m[1][0] * norm);
    result->m[0][2] = (matrix->m[2][0] * norm);
    result->m[1][0] = (matrix->m[0][1] * norm);
    result->m[1][1] = (matrix->m[1][1] * norm);
    result->m[1][2] = (matrix->m[2][1] * norm);
    result->m[2][0] = (matrix->m[0][2] * norm);
    result->m[2][1] = (matrix->m[1][2] * norm);
    result->m[2][2] = (matrix->m[2][2] * norm);
    result->m[3][0] = -((result->m[0][0] * matrix->m[3][0]) + (result->m[1][0] * matrix->m[3][1]) + (result->m[2][0] * matrix->m[3][2]));
    result->m[3][1] = -((result->m[0][1] * matrix->m[3][0]) + (result->m[1][1] * matrix->m[3][1]) + (result->m[2][1] * matrix->m[3][2]));
    result->m[3][2] = -((result->m[0][2] * matrix->m[3][0]) + (result->m[1][2] * matrix->m[3][1]) + (result->m[2][2] * matrix->m[3][2]));
    result->m[3][3] = 1.0f;
    result->m[0][3] = 0.0f;
    result->m[1][3] = 0.0f;
    result->m[2][3] = 0.0f;
}


/*
 * Address: 0x7F059FB8
*/
void matrix_4x4_invert_affine(Mtxf *matrix, Mtxf *result)
{
    f32 f0 = 0.0f;
    f0 += matrix->m[0][0] * matrix->m[1][1] * matrix->m[2][2];
    f0 += matrix->m[0][1] * matrix->m[1][2] * matrix->m[2][0];
    f0 += matrix->m[0][2] * matrix->m[1][0] * matrix->m[2][1];
    f0 -= matrix->m[0][2] * matrix->m[1][1] * matrix->m[2][0];
    f0 -= matrix->m[0][1] * matrix->m[1][0] * matrix->m[2][2];
    f0 -= matrix->m[0][0] * matrix->m[1][2] * matrix->m[2][1];
    f0 = 1.0f / f0;

    result->m[0][0] = (matrix->m[1][1] * matrix->m[2][2] - matrix->m[1][2] * matrix->m[2][1]) * f0;
    result->m[1][0] = (matrix->m[1][2] * matrix->m[2][0] - matrix->m[1][0] * matrix->m[2][2]) * f0;
    result->m[2][0] = (matrix->m[1][0] * matrix->m[2][1] - matrix->m[1][1] * matrix->m[2][0]) * f0;
    result->m[0][1] = (matrix->m[0][2] * matrix->m[2][1] - matrix->m[0][1] * matrix->m[2][2]) * f0;
    result->m[1][1] = (matrix->m[0][0] * matrix->m[2][2] - matrix->m[0][2] * matrix->m[2][0]) * f0;
    result->m[2][1] = (matrix->m[0][1] * matrix->m[2][0] - matrix->m[0][0] * matrix->m[2][1]) * f0;
    result->m[0][2] = (matrix->m[0][1] * matrix->m[1][2] - matrix->m[0][2] * matrix->m[1][1]) * f0;
    result->m[1][2] = (matrix->m[0][2] * matrix->m[1][0] - matrix->m[0][0] * matrix->m[1][2]) * f0;
    result->m[2][2] = (matrix->m[0][0] * matrix->m[1][1] - matrix->m[0][1] * matrix->m[1][0]) * f0;
    result->m[3][0] = -(matrix->m[3][0] * result->m[0][0] + matrix->m[3][1] * result->m[1][0] + matrix->m[3][2] * result->m[2][0]);
    result->m[3][1] = -(matrix->m[3][0] * result->m[0][1] + matrix->m[3][1] * result->m[1][1] + matrix->m[3][2] * result->m[2][1]);
    result->m[3][2] = -(matrix->m[3][0] * result->m[0][2] + matrix->m[3][1] * result->m[1][2] + matrix->m[3][2] * result->m[2][2]);
    result->m[0][3] = 0.0f;
    result->m[1][3] = 0.0f;
    result->m[2][3] = 0.0f;
    result->m[3][3] = 1.0f;
}


/*
 * Address: 0x7F05A250
*/
void matrix_4x4_invert(Mtxf *matrix, Mtxf *result) {
    s32 i, j;
    f32 inv_det;
    matrix_4x4_calc_adjugate(matrix, result);
    inv_det = 1.0f / matrix_4x4_determinant(matrix);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            result->m[i][j] *= inv_det;
        }
    }
}

/*
 * Address: 0x7F05A310
*/
void matrix_4x4_calc_adjugate(Mtxf *matrix, Mtxf *result)
{
    f32 mtx00, mtx10, mtx20, mtx30;
    f32 mtx04, mtx14, mtx24, mtx34;
    f32 mtx08, mtx18, mtx28, mtx38;
    f32 mtx0c, mtx1c, mtx2c, mtx3c;

    mtx00 = matrix->m[0][0]; mtx04 = matrix->m[0][1];
    mtx08 = matrix->m[0][2]; mtx0c = matrix->m[0][3];
    mtx10 = matrix->m[1][0]; mtx14 = matrix->m[1][1];
    mtx18 = matrix->m[1][2]; mtx1c = matrix->m[1][3];
    mtx20 = matrix->m[2][0]; mtx24 = matrix->m[2][1];
    mtx28 = matrix->m[2][2]; mtx2c = matrix->m[2][3];
    mtx30 = matrix->m[3][0]; mtx34 = matrix->m[3][1];
    mtx38 = matrix->m[3][2]; mtx3c = matrix->m[3][3];

    result->m[0][0] =  matrix_3x3_determinant(mtx14, mtx24, mtx34, mtx18, mtx28, mtx38, mtx1c, mtx2c, mtx3c);
    result->m[1][0] = -matrix_3x3_determinant(mtx10, mtx20, mtx30, mtx18, mtx28, mtx38, mtx1c, mtx2c, mtx3c);
    result->m[2][0] =  matrix_3x3_determinant(mtx10, mtx20, mtx30, mtx14, mtx24, mtx34, mtx1c, mtx2c, mtx3c);
    result->m[3][0] = -matrix_3x3_determinant(mtx10, mtx20, mtx30, mtx14, mtx24, mtx34, mtx18, mtx28, mtx38);
    result->m[0][1] = -matrix_3x3_determinant(mtx04, mtx24, mtx34, mtx08, mtx28, mtx38, mtx0c, mtx2c, mtx3c);
    result->m[1][1] =  matrix_3x3_determinant(mtx00, mtx20, mtx30, mtx08, mtx28, mtx38, mtx0c, mtx2c, mtx3c);
    result->m[2][1] = -matrix_3x3_determinant(mtx00, mtx20, mtx30, mtx04, mtx24, mtx34, mtx0c, mtx2c, mtx3c);
    result->m[3][1] =  matrix_3x3_determinant(mtx00, mtx20, mtx30, mtx04, mtx24, mtx34, mtx08, mtx28, mtx38);
    result->m[0][2] =  matrix_3x3_determinant(mtx04, mtx14, mtx34, mtx08, mtx18, mtx38, mtx0c, mtx1c, mtx3c);
    result->m[1][2] = -matrix_3x3_determinant(mtx00, mtx10, mtx30, mtx08, mtx18, mtx38, mtx0c, mtx1c, mtx3c);
    result->m[2][2] =  matrix_3x3_determinant(mtx00, mtx10, mtx30, mtx04, mtx14, mtx34, mtx0c, mtx1c, mtx3c);
    result->m[3][2] = -matrix_3x3_determinant(mtx00, mtx10, mtx30, mtx04, mtx14, mtx34, mtx08, mtx18, mtx38);
    result->m[0][3] = -matrix_3x3_determinant(mtx04, mtx14, mtx24, mtx08, mtx18, mtx28, mtx0c, mtx1c, mtx2c);
    result->m[1][3] =  matrix_3x3_determinant(mtx00, mtx10, mtx20, mtx08, mtx18, mtx28, mtx0c, mtx1c, mtx2c);
    result->m[2][3] = -matrix_3x3_determinant(mtx00, mtx10, mtx20, mtx04, mtx14, mtx24, mtx0c, mtx1c, mtx2c);
    result->m[3][3] =  matrix_3x3_determinant(mtx00, mtx10, mtx20, mtx04, mtx14, mtx24, mtx08, mtx18, mtx28);
}

f32 matrix_4x4_determinant(Mtxf *matrix)
{
    f32 tmp;
	f32 sp78, sp74, sp70, sp6c;
	f32 sp68, sp64, sp60, sp5c;
	f32 sp58, sp54, sp50, sp4c;
	f32 sp48, sp44, sp40, sp3c;
	f32 sp38;
	f32 sp34;
	f32 sp30;

	sp78 = matrix->m[0][0]; sp68 = matrix->m[0][1];
	sp58 = matrix->m[0][2]; sp48 = matrix->m[0][3];
	sp74 = matrix->m[1][0]; sp64 = matrix->m[1][1];
	sp54 = matrix->m[1][2]; sp44 = matrix->m[1][3];
	sp70 = matrix->m[2][0]; sp60 = matrix->m[2][1];
	sp50 = matrix->m[2][2]; sp40 = matrix->m[2][3];
	sp6c = matrix->m[3][0]; sp5c = matrix->m[3][1];
	sp4c = matrix->m[3][2]; sp3c = matrix->m[3][3];

	sp30 = matrix_3x3_determinant(sp74, sp70, sp6c, sp64, sp60, sp5c, sp44, sp40, sp3c);
	sp34 = matrix_3x3_determinant(sp74, sp70, sp6c, sp54, sp50, sp4c, sp44, sp40, sp3c);
	sp38 = matrix_3x3_determinant(sp64, sp60, sp5c, sp54, sp50, sp4c, sp44, sp40, sp3c);

	tmp = matrix_3x3_determinant(sp74, sp70, sp6c, sp64, sp60, sp5c, sp54, sp50, sp4c);

	return (sp78 * sp38 - sp68 * sp34 + sp58 * sp30) - tmp * sp48;

}

f32 matrix_3x3_determinant(f32 a, f32 d, f32 g, f32 b, f32 e, f32 h, f32 c, f32 f, f32 i) {
    f32 determinant = (a * matrix_2x2_determinant(e, h, f, i)) - (b * matrix_2x2_determinant(d, g, f, i)) + (c * matrix_2x2_determinant(d, g, e, h));
    return determinant;
}

f32 matrix_2x2_determinant(f32 a, f32 c, f32 b, f32 d) {
     return (a * d) - (c * b);
}
