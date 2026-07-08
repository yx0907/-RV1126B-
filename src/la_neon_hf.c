#include <arm_neon.h>
#include <math.h>
#include "la_neon_hf.h"

void neon_compute_ATB(const float AT[3][16], const float B[16], float ATB[3])
{
    float32x4_t sum0 = vdupq_n_f32(0.0f);
    float32x4_t sum1 = vdupq_n_f32(0.0f);
    float32x4_t sum2 = vdupq_n_f32(0.0f);

    for(int i = 0; i < 16; i += 4)
    {
        float32x4_t b_vec = vld1q_f32(&B[i]);
        float32x4_t a0 = vld1q_f32(&AT[0][i]);
        float32x4_t a1 = vld1q_f32(&AT[1][i]);
        float32x4_t a2 = vld1q_f32(&AT[2][i]);

        sum0 = vmlaq_f32(sum0, a0, b_vec);
        sum1 = vmlaq_f32(sum1, a1, b_vec);
        sum2 = vmlaq_f32(sum2, a2, b_vec);
    }
    ATB[0] = vaddvq_f32(sum0);
    ATB[1] = vaddvq_f32(sum1);
    ATB[2] = vaddvq_f32(sum2);
}

void neon_3x3mat_multi_vec(const float A[9], const float v[3], float out[3])
{
    float32x4_t v_vec = vld1q_f32(v);
    float32x4_t row0 = vld1q_f32(&A[0]);
    float32x4_t prod0 = vmulq_f32(row0, v_vec);
    float32x4_t row1 = vld1q_f32(&A[3]);
    float32x4_t prod1 = vmulq_f32(row1, v_vec);
    float32x4_t row2 = vld1q_f32(&A[6]);
    float32x4_t prod2 = vmulq_f32(row2, v_vec);

    out[0] = vaddvq_f32(prod0);
    out[1] = vaddvq_f32(prod1);
    out[2] = vaddvq_f32(prod2);
}

float hf_3x3mat_det(const Mat3 * A)
{
    return  A->m[0][0] * (A->m[1][1] * A->m[2][2] - A->m[1][2] * A->m[2][1]) -
            A->m[0][1] * (A->m[1][0] * A->m[2][2] - A->m[1][2] * A->m[2][0]) +
            A->m[0][2] * (A->m[1][0] * A->m[2][1] - A->m[1][1] * A->m[2][0]);
}

Mat3 hf_3x3mat_inv(const Mat3 * A)
{
    Mat3 inv;
    float det = hf_3x3mat_det(A);
    float inv_det = 1.0f / det;

    inv.m[0][0] = (A->m[1][1] * A->m[2][2] - A->m[1][2] * A->m[2][1]) * inv_det;
    inv.m[0][1] = (A->m[0][2] * A->m[2][1] - A->m[0][1] * A->m[2][2]) * inv_det;
    inv.m[0][2] = (A->m[0][1] * A->m[1][2] - A->m[0][2] * A->m[1][1]) * inv_det;
    inv.m[1][0] = (A->m[1][2] * A->m[2][0] - A->m[1][0] * A->m[2][2]) * inv_det;
    inv.m[1][1] = (A->m[0][0] * A->m[2][2] - A->m[0][2] * A->m[2][0]) * inv_det;
    inv.m[1][2] = (A->m[0][2] * A->m[1][0] - A->m[0][0] * A->m[1][2]) * inv_det;
    inv.m[2][0] = (A->m[1][0] * A->m[2][1] - A->m[1][1] * A->m[2][0]) * inv_det;
    inv.m[2][1] = (A->m[0][1] * A->m[2][0] - A->m[0][0] * A->m[2][1]) * inv_det;
    inv.m[2][2] = (A->m[0][0] * A->m[1][1] - A->m[0][1] * A->m[1][0]) * inv_det;

    return inv;
}
