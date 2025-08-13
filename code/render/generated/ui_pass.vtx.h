/**
 * @file
 * @brief Auto-generated file from code\render\vtx\ui_pass.vtx.
 * Do not edit manually.
 */

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
