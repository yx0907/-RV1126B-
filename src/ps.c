#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "la_neon_hf.h"
#include "rti_conf.h"
#include "ps.h"

/*  
*   C = ( (A^T A)^-1 ) A^T B
*   A: Light source vector
*   B: Luminance vector
*   C: Target vector
*   albedo = ||C||
*   normals = C / albedo
*/

//matrix A
float lights[16][3] =
{
    { 0.9248, -0.3699, 0.0462},
    {-0.9045,  0.4221, 0.0603},
    { 0.0987,  0.1234, 0.9873},
    {-0.1476, -0.0984, 0.9841},
    { 0.8124, -0.2166, 0.5416},
    { 0.7022,  0.1170, 0.7022},
    { 0.8073,  0.4305, 0.4036},
    {-0.7564,  0.3026, 0.5799},
    {-0.7256, -0.1209, 0.6773},
    {-0.7712, -0.4627, 0.4370},
    {-0.5061,  0.7231, 0.4700},
    { 0.1400,  0.7001, 0.7001},
    { 0.4898,  0.5443, 0.6804},
    {-0.5842, -0.6492, 0.4869},
    {-0.0671, -0.6713, 0.7384},
    { 0.4869, -0.6492, 0.5842}
};

/*Conversion LUT*/
float srgb_to_linear[256];
unsigned char linear_to_srgb[256];

void gamma_tables_init(void)
{
    for(int i = 0; i < 256; i++)
    {
        //sRGB -> Linear
        float s = i / 255.0f;
        float lin;
        if(s <= 0.04045f)
            lin = s / 12.92f;
        else
            lin = powf((s + 0.055f) / 1.055f, 2.4f);
        srgb_to_linear[i] = lin;

        //Linear -> sRGB
        float L = i / 255.0f;
        float srgb;
        if(L <= 0.0031308f)
            srgb = 12.92f * L;
        else
            srgb = 1.055f * powf(L, 1.0f/2.4f) - 0.055f;
        if(srgb < 0.0f) 
            srgb = 0.0f;
        if(srgb > 1.0f) 
            srgb = 1.0f;
        linear_to_srgb[i] = (unsigned char)(srgb * 255.0f + 0.5f);
    }
}

/*PS fit*/
//Only need to compute once when init ps
Mat3 ATA = {0};
float AT[3][16];
Mat3 ATA_inv;

void rti_ps_init(void)
{
    for(int i = 0; i < 16; i++) 
    {
        float lx = lights[i][0];
        float ly = lights[i][1];
        float lz = lights[i][2];

        ATA.m[0][0] += lx * lx;
        ATA.m[0][1] += lx * ly;
        ATA.m[0][2] += lx * lz;
        ATA.m[1][1] += ly * ly;
        ATA.m[1][2] += ly * lz;
        ATA.m[2][2] += lz * lz;

        AT[0][i] = lx;
        AT[1][i] = ly;
        AT[2][i] = lz;
    }
    ATA.m[1][0] = ATA.m[0][1];
    ATA.m[2][0] = ATA.m[0][2];
    ATA.m[2][1] = ATA.m[1][2];

    ATA_inv = hf_3x3mat_inv(&ATA);
}

void photometric_stereo(const GImage * images, float * normals, float * albedo)
{
    for(int y = 0; y < FRAME_HEIGHT; y++)
    {
        for(int x = 0; x < FRAME_WIDTH; x++)
        {
            //Get the luminance from images and convert
            float B[16];
            for (int i = 0; i < 16; i++)
                B[i] = srgb_to_linear[images[i].P[y][x]];

            //Compute the ATB
            float ATB[3] = {0};
            neon_compute_ATB(AT, B, ATB);

            //Compute the C
            float C[3];
            neon_3x3mat_multi_vec((float *)&ATA_inv, ATB, C);

            //Compute the normals and albedo
            float len = sqrtf(C[0] * C[0] + C[1] * C[1] + C[2] * C[2]);
            int idx = y * FRAME_WIDTH + x;
            if(len > 1e-6f)
            {
                normals[idx * 3 + 0] = C[0] / len;
                normals[idx * 3 + 1] = C[1] / len;
                normals[idx * 3 + 2] = C[2] / len;
                albedo[idx] = len;
            }
            else
            {
                normals[idx * 3 + 0] = 0.0f;
                normals[idx * 3 + 1] = 0.0f;
                normals[idx * 3 + 2] = 0.0f;
                albedo[idx] = 0.0f;
            }
            //Reduce the impact of specular reflection    
            if(albedo[idx] > 1.2f)
            {
                normals[idx * 3 + 0] = 0.0f;
                normals[idx * 3 + 1] = 0.0f;
                normals[idx * 3 + 2] = 0.0f;
                albedo[idx] = 0.0f;
            }
        
        }
    }
}

void render_by_Lambertian_Diffuse(GImage * images, float * normals, float * albedo,
                                  const float light_dir[3])
{
    float lx = light_dir[0];
    float ly = light_dir[1];
    float lz = light_dir[2];

    for(int y = 0; y < FRAME_HEIGHT; y++)
    {
        for(int x = 0; x < FRAME_WIDTH; x++)
        {
            int idx = y * FRAME_WIDTH + x;
            float nx = normals[idx * 3 + 0];
            float ny = normals[idx * 3 + 1];
            float nz = normals[idx * 3 + 2];
            float alb = albedo[idx];

            //Lambertian_Diffuse: L = albedo * max(0, n·L)
            float dot = nx * lx + ny * ly + nz * lz;
            if(dot < 0)
                dot = 0;
            float L = alb * dot;

            //Linear to srgb convert
            int i = (int)(L * 255.0f + 0.5f);
            if(i < 0)
                i = 0;
            if(i > 255)
                i = 255;
            images[0].P[y][x] = linear_to_srgb[i];
        }
    }
}                                  
