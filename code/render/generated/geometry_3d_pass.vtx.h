/**
 * @file
 * @brief Auto-generated file from code\render\vtx\geometry_3d_pass.vtx.
 * Do not edit manually.
 */

#ifndef GEOMETRY_3D_PASS_VTX_H_
#define GEOMETRY_3D_PASS_VTX_H_

#include <stdint.h>
#include <d3d11.h>
#include <stddef.h>

typedef struct Geometry3D_Vertex {
    float pos[3];
    float normal[3];
    float texCoord[2];
    float col[3];
} Geometry3D_Vertex;

static const D3D11_INPUT_ELEMENT_DESC Geometry3D_Vertex_desc[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Geometry3D_Vertex, pos), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Geometry3D_Vertex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Geometry3D_Vertex, texCoord), D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Geometry3D_Vertex, col), D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
static const unsigned int Geometry3D_Vertex_desc_count = sizeof(Geometry3D_Vertex_desc) / sizeof(Geometry3D_Vertex_desc[0]);

typedef struct Geometry3D_Transform {
    float view[16];
    float model[16];
    float projection[16];
} Geometry3D_Transform;

typedef struct Geometry3D_InstancedLayout {
    float instanceMatrixRow0[4];
    float instanceMatrixRow1[4];
    float instanceMatrixRow2[4];
    float instanceMatrixRow3[4];
} Geometry3D_InstancedLayout;

#endif // GEOMETRY_3D_PASS_VTX_H_
