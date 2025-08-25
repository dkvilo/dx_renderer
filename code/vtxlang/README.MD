# VTXLANG

VTXLang is an experimental domain-specific language (DSL) that generates vertex layouts and shader code at compile time. Currently, it supports DXGI for DirectX 11, with planned support for DirectX 12, Vulkan, OpenGL, and WebGL.

# Example

The following example demonstrates how VTXLang can generate .h and .hlsl files:

```vtx
layout Geometry2D_Vertex
{
  float3 pos : POSITION;
  float3 col : COLOR;
};

layout PS_INPUT
{
  float4 pos : SV_POSITION;
  float3 col : COLOR;
};

layout Geometry2D_Transform
{
  float time;
  float scale;
  float2 padding;
};

//
// THIS IS VERTEX BUFFER INPUT AND LAYOUT
//
vertex Geometry2D_Vertex @input;

//
// THIS IS PIXEL SHADER INPUT
//
pixel PS_INPUT @input @gpu;

//
// THIS IS CONSTANT BUFFER THT IS VISIBLE IN VERTEX AND PIXEL SHADER
// IT IS BOUND TO BUFFER0
//
buffer Geometry2D_Transform @vertex @pixel @b0;
```

# Generated Vertex Layout Descriptor (.H)

```c
#ifndef UI_PASS_VTX_H_
#define UI_PASS_VTX_H_

#include <stdint.h>
#include <d3d11.h>
#include <stddef.h>

typedef struct Geometry2D_Vertex {
    float pos[3];
    float col[3];
} Geometry2D_Vertex;

static const D3D11_INPUT_ELEMENT_DESC Geometry2D_Vertex_desc[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Geometry2D_Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Geometry2D_Vertex, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
static const unsigned int Geometry2D_Vertex_desc_count = sizeof(Geometry2D_Vertex_desc) / sizeof(Geometry2D_Vertex_desc[0]);

typedef struct Geometry2D_Transform {
    float time;
    float scale;
    float padding[2];
} Geometry2D_Transform;

#endif // UI_PASS_VTX_H_
```

# Generated Shader Input/Output (.hlsl)

```hlsl
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

```
