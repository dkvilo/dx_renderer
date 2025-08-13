/**
 * @file
 * @brief Auto-generated file from code\render\vtx\geometry_3d_pass.vtx.
 * Do not edit manually.
 */

struct VS_INPUT {
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 col : COLOR;
};

cbuffer Geometry3D_Transform : register(b0) {
    matrix view;
    matrix model;
    matrix projection;
};

cbuffer Geometry3D_InstancedLayout : register(b1) {
    float4 instanceMatrixRow0;
    float4 instanceMatrixRow1;
    float4 instanceMatrixRow2;
    float4 instanceMatrixRow3;
};

