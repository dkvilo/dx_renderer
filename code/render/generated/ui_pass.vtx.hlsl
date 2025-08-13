/**
 * @file
 * @brief Auto-generated file from code\render\vtx\ui_pass.vtx.
 * Do not edit manually.
 */

struct VS_INPUT {
    float3 pos : POSITION;
    float3 col : COLOR;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float3 col : COLOR;
};

cbuffer Geometry2D_Transform : register(b0) {
    float time;
    float scale;
    float2 padding;
};

