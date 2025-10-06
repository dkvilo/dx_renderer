//
// cl d3d11_triangle.c /link /out:Tri.exe
//
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <math.h>

#define SHADER_CODE( ... ) #__VA_ARGS__
#define ARRAY_COUNT( arr ) ( sizeof( arr ) / sizeof( ( arr )[0] ) )

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "user32.lib" )

#define HANDMADE_MATH_NO_SSE
#include "code/extern/hmmath.h"

#include "code/base/c_string.c"

#define STB_IMAGE_IMPLEMENTATION
#include "code/extern/stb_image.h"

typedef struct
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
} Vertex;

HWND                      g_hWnd               = NULL;
ID3D11Device             *g_pd3dDevice         = NULL;
ID3D11DeviceContext      *g_pImmediateContext  = NULL;
IDXGISwapChain           *g_pSwapChain         = NULL;
ID3D11RenderTargetView   *g_pRenderTargetView  = NULL;
ID3D11VertexShader       *g_pVertexShader      = NULL;
ID3D11PixelShader        *g_pPixelShader       = NULL;
ID3D11InputLayout        *g_pVertexLayout      = NULL;
ID3D11Buffer             *g_pVertexBuffer      = NULL;
ID3D11Buffer             *g_pIndexBuffer       = NULL;
ID3D11Texture2D          *g_pDepthStencil      = NULL;
ID3D11DepthStencilView   *g_pDepthStencilView  = NULL;
ID3D11Buffer             *g_pCBufferTransforms = NULL;
ID3D11Buffer             *g_pCBufferLights     = NULL;
ID3D11Texture2D          *texture              = NULL;
ID3D11ShaderResourceView *textureView          = NULL;

const int wndWidth  = 1600;
const int wndHeight = 900;

const char *vertexShaderSource = SHADER_CODE(

  cbuffer cbuf : register( b0 ) {
    row_major matrix transform;
    row_major matrix normalMatrix;
    float3 viewPos;
    float padding;
  };

  struct VS_INPUT {
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
    float2 uv : TEXCOORD;
  };

  struct PS_INPUT {
    float4 Pos : SV_POSITION;
    float3 Normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 WorldPos : POSITION1;
  };

  PS_INPUT VS_Main(VS_INPUT input) {
    PS_INPUT output;
    float4 worldPos = input.Pos;
    output.Pos = mul(input.Pos, transform);
    output.WorldPos =  worldPos;
    output.uv     = input.uv;
    output.Normal = input.Normal;
    return output;
  }

);
const char *pixelShaderSource = SHADER_CODE(

 static const int MAX_LIGHTS = 16;

  cbuffer cbuf : register(b0) {
    row_major matrix transform;
    float3 viewPos;
    float padding0;
  };

  struct Light {
      float3 position;
      float  intensity;
      float3 color;
      float  padding;
  };

  cbuffer LightBuffer : register(b1)
  {
      Light lights[MAX_LIGHTS];
      int lightCount;
      float3 padding2;
  };

  Texture2D tex : register(t0);
  SamplerState samp : register(s0);

  struct PS_INPUT {
      // VERTEX LAYOUT
      float4 Pos : SV_POSITION;
      float3 Normal : NORMAL;
      float2 uv : TEXCOORD;
      // END VERTEX LAYOUT

      float3 WorldPos : POSITION;
  };

  float4 PS_Main(PS_INPUT input) : SV_Target {

    float3 result = float3(0, 0, 0);
    float3 N = normalize(input.Normal);
    float3 fragPos = input.WorldPos;
    float3 V = normalize(viewPos - fragPos);

    // for (int i = 0; i < lightCount; ++i)
    // {
    //   float3 L = normalize(lights[i].position - fragPos);
    //   float3 R = reflect(-L, N);

    //   float diff = max(dot(N, L), 0.0);
    //   float spec = pow(max(dot(R, V), 0.0), 8.0);

    //   float3 ambient  = 0.1 * lights[i].color;
    //   float3 diffuse  = diff * lights[i].color * lights[i].intensity;
    //   float3 specular = spec * lights[i].color;

    //   result += ambient + diffuse;
    // }

    float4 texColor = tex.Sample(samp, input.uv); //*  float4(result, 1.0);

    float3 gammaCorrected = pow(texColor, 1.0 / 1.0); // or 1.0 / gamma
    return float4(gammaCorrected, 1.0);

  }
);

int   xPos, yPos = 0;
float DirX, DirY, DirZ;

#include <stdbool.h>

bool keyDown[256]    = { 0 };
bool keyPressed[256] = { 0 };

void UpdateInput()
{
	for ( int i = 0; i < 256; ++i )
	{
		bool isDownNow = ( GetAsyncKeyState( i ) & 0x8000 ) != 0;

		keyPressed[i] = ( isDownNow && !keyDown[i] );
		keyDown[i]    = isDownNow;
	}
}

bool IsDown( int key )
{
	return keyDown[key];
}

bool IsPressed( int key )
{
	return keyPressed[key];
}

bool  is_mouseDown = false;
float yaw          = 0.0f;
float pitch        = 0.0f;

HMM_Vec3 cameraPos   = { 0, 0, -5.0f };
HMM_Vec3 cameraFront = { 0.0f, 0.0f, 1.0f };
HMM_Vec3 cameraUp    = { 0.0f, 1.0f, 0.0f };

POINT center;
POINT lastMousePos;
bool  firstMouse = true;

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;

	case WM_MOUSEMOVE:
		xPos = GET_X_LPARAM( lParam );
		yPos = GET_Y_LPARAM( lParam );
		break;

	case WM_LBUTTONDOWN:
		is_mouseDown = true;
		ShowCursor( FALSE );
		SetCapture( hWnd );
		GetCursorPos( &lastMousePos );
		ScreenToClient( hWnd, &lastMousePos );
		break;

	case WM_LBUTTONUP:
		is_mouseDown = false;
		ShowCursor( TRUE );
		ReleaseCapture();
		break;

	case WM_KEYDOWN:

		if ( wParam == VK_ESCAPE )
		{
			PostQuitMessage( 0 );
			return 0;
		}
		break;
	}
	return DefWindowProc( hWnd, msg, wParam, lParam );
}

HRESULT D3D11_Init();
HRESULT D3D11_SetupPipeline();
void    D3D11_CleanupDevice();

void R_Frame( HWND hWnd, float deltaTime );

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "code/extern/tinyobj_loader_c.h"

Vertex _vertices[] = {
    // { -0.5f, -0.5f, -0.5f }, // 0: left-bottom-back
    // { 0.5f, -0.5f, -0.5f },  // 1: right-bottom-back
    // { 0.5f, 0.5f, -0.5f },   // 2: right-top-back
    // { -0.5f, 0.5f, -0.5f },  // 3: left-top-back
    // { -0.5f, -0.5f, 0.5f },  // 4: left-bottom-front
    // { 0.5f, -0.5f, 0.5f },   // 5: right-bottom-front
    // { 0.5f, 0.5f, 0.5f },    // 6: right-top-front
    // { -0.5f, 0.5f, 0.5f },   // 7: left-top-front

    // Front face
    { -0.5f, -0.5f, 0.5f, 0.0f, 1.0f }, // bottom-left
    { 0.5f, -0.5f, 0.5f, 1.0f, 1.0f },  // bottom-right
    { 0.5f, 0.5f, 0.5f, 1.0f, 0.0f },   // top-right
    { -0.5f, 0.5f, 0.5f, 0.0f, 0.0f },  // top-left

    // Back face (z = -0.5)
    { 0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { -0.5f, 0.5f, -0.5f, 1.0f, 0.0f },
    { 0.5f, 0.5f, -0.5f, 0.0f, 0.0f },

    // Left face (x = -0.5)
    { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    { -0.5f, -0.5f, 0.5f, 1.0f, 1.0f },
    { -0.5f, 0.5f, 0.5f, 1.0f, 0.0f },
    { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f },

    // Right face (x = +0.5)
    { 0.5f, -0.5f, 0.5f, 0.0f, 1.0f },
    { 0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 1.0f, 0.0f },
    { 0.5f, 0.5f, 0.5f, 0.0f, 0.0f },

    // Top face (y = +0.5)
    { -0.5f, 0.5f, 0.5f, 0.0f, 1.0f },
    { 0.5f, 0.5f, 0.5f, 1.0f, 1.0f },
    { 0.5f, 0.5f, -0.5f, 1.0f, 0.0f },
    { -0.5f, 0.5f, -0.5f, 0.0f, 0.0f },

    // Bottom face (y = -0.5)
    { -0.5f, -0.5f, -0.5f, 0.0f, 1.0f },
    { 0.5f, -0.5f, -0.5f, 1.0f, 1.0f },
    { 0.5f, -0.5f, 0.5f, 1.0f, 0.0f },
    { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f },
};

// clang-format off
  unsigned short _indices[] = {
      // front
      // 4, 5, 6, 6, 7, 4,
      // // back
      // 1, 0, 3, 3, 2, 1,
      // // left
      // 0, 4, 7, 7, 3, 0,
      // // right
      // 5, 1, 2, 2, 6, 5,
      // // top
      // 3, 7, 6, 6, 2, 3,
      // // bottom
      // 0, 1, 5, 5, 4, 0

      0, 1, 2,   2, 3, 0,     // Front
      4, 5, 6,   6, 7, 4,     // Back
      8, 9,10,  10,11, 8,     // Left
    12,13,14,  14,15,12,     // Right
    16,17,18,  18,19,16,     // Top
    20,21,22,  22,23,20      // Bottom
  };
// clang-format on

Vertex         *vertices = NULL;
unsigned short *indices  = NULL;

size_t vertex_count = 0;
size_t index_count  = 0;

static void *mmap_file_win( size_t *out_len, const char *filename )
{
	HANDLE hFile =
	    CreateFileA( filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if ( hFile == INVALID_HANDLE_VALUE )
	{
		fprintf( stderr, "CreateFileA failed for %s\n", filename );
		*out_len = 0;
		return NULL;
	}

	DWORD fileSize = GetFileSize( hFile, NULL );
	if ( fileSize == INVALID_FILE_SIZE )
	{
		fprintf( stderr, "GetFileSize failed\n" );
		CloseHandle( hFile );
		*out_len = 0;
		return NULL;
	}

	HANDLE hMap = CreateFileMappingA( hFile, NULL, PAGE_READONLY, 0, 0, NULL );

	if ( !hMap )
	{
		fprintf( stderr, "CreateFileMappingA failed\n" );
		CloseHandle( hFile );
		*out_len = 0;
		return NULL;
	}

	void *mappedData = MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

	if ( !mappedData )
	{
		fprintf( stderr, "MapViewOfFile failed\n" );
		CloseHandle( hMap );
		CloseHandle( hFile );
		*out_len = 0;
		return NULL;
	}

	CloseHandle( hMap );
	CloseHandle( hFile );

	*out_len = (size_t)fileSize;
	return mappedData;
}

static void
get_file_data( void *ctx, const char *filename, const int is_mtl, const char *obj_filename, char **data, size_t *len )
{
	(void)ctx;
	(void)is_mtl;
	(void)obj_filename;

	if ( !filename )
	{
		fprintf( stderr, "null filename\n" );
		*data = NULL;
		*len  = 0;
		return;
	}

	*data = mmap_file_win( len, filename );
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	const char *pClassName = "ddx11renderer";
	const char *pCaption   = "D3D11 Window";
	WNDCLASSEX  wc         = { 0 };
	wc.cbSize              = sizeof( wc );
	wc.style               = CS_OWNDC;
	wc.lpfnWndProc         = WndProc;
	wc.cbClsExtra          = 0;
	wc.cbWndExtra          = 0;
	wc.hInstance           = hInstance;
	wc.hIcon               = NULL;
	wc.hCursor             = NULL;
	wc.hbrBackground       = NULL;
	wc.lpszMenuName        = NULL;
	wc.lpszClassName       = pClassName;
	wc.hIconSm             = NULL;

	RegisterClassEx( &wc );

	DWORD dwStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
	g_hWnd = CreateWindowEx( 0, pClassName, pCaption, dwStyle, 0, 0, wndWidth, wndHeight, NULL, NULL, hInstance, NULL );

	AllocConsole();
	freopen( "CONOUT$", "w", stdout );

	unsigned int        flags = TINYOBJ_FLAG_TRIANGULATE;
	size_t              num_materials;
	tinyobj_material_t *materials;
	size_t              num_shapes;
	tinyobj_shape_t    *shapes;
	tinyobj_attrib_t    attrib;

	int ret = tinyobj_parse_obj( &attrib,
	                             &shapes,
	                             &num_shapes,
	                             &materials,
	                             &num_materials,
	                             ".\\DEER\\deer.obj",
	                             get_file_data,
	                             NULL,
	                             flags );
	if ( ret != TINYOBJ_SUCCESS )
	{
		return 0;
	}

	size_t num_triangles = attrib.num_face_num_verts;
	size_t face_offset   = 0;

	size_t total_face_verts = 0;
	for ( size_t i = 0; i < attrib.num_face_num_verts; i++ )
	{
		total_face_verts += attrib.face_num_verts[i];
	}

	printf( "Total Face Vert %zd\n", total_face_verts );

	vertices = (Vertex *)malloc( sizeof( Vertex ) * total_face_verts );
	indices  = (unsigned short *)malloc( sizeof( unsigned short ) * total_face_verts );

	if ( !vertices || !indices )
	{
		printf( "Failed to allocate memory\n" );
		return 1;
	}

	for ( size_t f = 0; f < attrib.num_face_num_verts; f++ )
	{
		int fv = attrib.face_num_verts[f];

		for ( int v = 0; v < fv; v++ )
		{
			if ( ( face_offset + v ) >= total_face_verts )
			{
				printf( "Invalid face index access\n" );
				free( vertices );
				free( indices );
				return 1;
			}

			tinyobj_vertex_index_t idx = attrib.faces[face_offset + v];

			float vx = attrib.vertices[3 * idx.v_idx + 0];
			float vy = attrib.vertices[3 * idx.v_idx + 1];
			float vz = attrib.vertices[3 * idx.v_idx + 2];

			float nx = attrib.vertices[3 * idx.vn_idx + 0];
			float ny = attrib.vertices[3 * idx.vn_idx + 1];
			float nz = attrib.vertices[3 * idx.vn_idx + 2];

			float tx = 0.0f, ty = 0.0f;
			if ( idx.vt_idx >= 0 )
			{
				tx = attrib.texcoords[2 * idx.vt_idx + 0];
				ty = attrib.texcoords[2 * idx.vt_idx + 1];
			}

			vertices[vertex_count] = (Vertex){ vx, vy, vz, nx, ny, nz, tx, ty };
			indices[index_count++] = (unsigned short)vertex_count;
			vertex_count++;
		}

		face_offset += fv;
	}

	// memset( vertices, 0, vertex_count * sizeof( Vertex ) );
	// memset( indices, 0, index_count * sizeof( unsigned short ) );

	// memcpy( vertices, _vertices, sizeof( _vertices ) );
	// memcpy( indices, _indices, sizeof( _indices ) );

	LARGE_INTEGER frequency, lastCounter, currentCounter;
	QueryPerformanceFrequency( &frequency );
	QueryPerformanceCounter( &lastCounter );

	if ( SUCCEEDED( D3D11_Init() ) )
	{
		if ( SUCCEEDED( D3D11_SetupPipeline() ) )
		{
			ShowWindow( g_hWnd, nCmdShow );

			MSG msg = { 0 };
			while ( WM_QUIT != msg.message )
			{
				QueryPerformanceCounter( &currentCounter );
				float deltaTime = (float)( currentCounter.QuadPart - lastCounter.QuadPart ) / (float)frequency.QuadPart;
				lastCounter     = currentCounter;

				if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
				{
					DispatchMessage( &msg );
				}

				R_Frame( g_hWnd, deltaTime );
			}
		}
	}

	while ( 1 )
		;
	D3D11_CleanupDevice();

	UnregisterClass( pClassName, wc.hInstance );
	return 0;
}

typedef struct CUniformBuffer
{
	HMM_Mat4 transform;
	HMM_Mat4 normalMatrix;
	HMM_Vec3 viewPos;
	float    padding;
} CUniformBuffer;

#define MAX_LIGHTS 16

typedef struct
{
	HMM_Vec3 position;
	float    intensity;
	HMM_Vec3 color;
	float    padding;
} Light;

typedef struct
{
	Light lights[MAX_LIGHTS];
	int   lightCount;
	float padding[3];
} CLightBuffer;

static float time = 0.0f;

void UpdateMouseLook( HWND hWnd )
{
	if ( !is_mouseDown )
		return;

	POINT currentMouse;
	GetCursorPos( &currentMouse );
	ScreenToClient( hWnd, &currentMouse );

	int dx = currentMouse.x - lastMousePos.x;
	int dy = currentMouse.y - lastMousePos.y;

	lastMousePos = currentMouse;

	float sensitivity = 0.002f;
	yaw += dx * sensitivity;
	pitch -= dy * sensitivity;

	float P2 = HMM_PI32 * 2.0f;
	if ( pitch > P2 )
		pitch = P2;
	if ( pitch < -P2 )
		pitch = -P2;

	cameraFront.X = cosf( pitch ) * sinf( yaw );
	cameraFront.Y = sinf( pitch );
	cameraFront.Z = cosf( pitch ) * cosf( yaw );
	cameraFront   = HMM_NormV3( cameraFront );
}

void R_Frame( HWND hWnd, float deltaTime )
{
	time += 0.01f;

	UpdateInput();
	UpdateMouseLook( hWnd );

	// === Projection Matrix ===
	float    aspect = (float)wndWidth / (float)wndHeight;
	HMM_Mat4 proj   = HMM_Perspective_LH_NO( HMM_PI / 3.0f, aspect, 0.1f, 100.0f );

	// === Model Transform (TRS) ===
	HMM_Mat4 rot       = HMM_Rotate_LH( 90.0f, HMM_V3( 0.0f, 1.0f, 0.0f ) );
	HMM_Mat4 scale     = HMM_Scale( HMM_V3( 5.0f, 5.0f, 5.0f ) );
	HMM_Mat4 translate = HMM_Translate( HMM_V3( 0.0f, -1.5f, 0.0f ) );
	HMM_Mat4 model     = HMM_MulM4( HMM_MulM4( rot, scale ), translate );

	// === Camera Movement ===
	float    speed       = 20.0f;
	HMM_Vec3 cameraRight = HMM_NormV3( HMM_Cross( cameraUp, cameraFront ) );
	HMM_Vec3 move        = { 0 };

	if ( IsDown( 'W' ) )
		move = HMM_AddV3( move, cameraFront );
	if ( IsDown( 'S' ) )
		move = HMM_SubV3( move, cameraFront );
	if ( IsDown( 'A' ) )
		move = HMM_SubV3( move, cameraRight );
	if ( IsDown( 'D' ) )
		move = HMM_AddV3( move, cameraRight );
	if ( IsDown( VK_SPACE ) )
		move = HMM_AddV3( move, cameraUp );
	if ( IsDown( VK_LSHIFT ) )
		move = HMM_SubV3( move, cameraUp );

	if ( HMM_LenV3( move ) > 0.01f )
	{
		move      = HMM_MulV3F( HMM_NormV3( move ), speed * deltaTime );
		cameraPos = HMM_AddV3( cameraPos, move );
	}

	// === View Matrix ===
	HMM_Vec3 cameraTarget = HMM_AddV3( cameraPos, cameraFront );
	HMM_Mat4 view         = HMM_LookAt_LH( cameraPos, cameraTarget, cameraUp );

	// === Final Transform ===
	CUniformBuffer cb = { 0 };
	cb.transform      = HMM_MulM4( HMM_MulM4( proj, view ), model );
	cb.viewPos        = HMM_MulM4V4( view, HMM_V4( cameraPos.X, cameraPos.Y, cameraPos.Z, 1.0f ) ).XYZ;
	cb.normalMatrix   = HMM_TransposeM4( HMM_InvGeneralM4( model ) );

	// === Update Constant Buffer ===
	D3D11_MAPPED_SUBRESOURCE mappedResourceUbo;
	if ( SUCCEEDED( g_pImmediateContext->lpVtbl->Map( g_pImmediateContext,
	                                                  (ID3D11Resource *)g_pCBufferTransforms,
	                                                  0,
	                                                  D3D11_MAP_WRITE_DISCARD,
	                                                  0,
	                                                  &mappedResourceUbo ) ) )
	{
		memcpy( mappedResourceUbo.pData, &cb, sizeof( cb ) );
		g_pImmediateContext->lpVtbl->Unmap( g_pImmediateContext, (ID3D11Resource *)g_pCBufferTransforms, 0 );
	}

	Light lights[] = {
	    { HMM_V3( 0.0f, 2.5f, 0.0f ), 4.0f, HMM_V3( 1.0f, 1.0f, 1.0f ), 0.0f }, // Front
	    // { HMM_V3( 2.0f, 0.5f, 0.0f ), 1.0f, HMM_V3( 1.0f, 0.0f, 0.0f ), 0.0f },   // Right
	    // { HMM_V3( -2.0f, 0.5f, 0.0f ), 1.0f, HMM_V3( 0.0f, 1.0f, 0.0f ), 0.0f },  // Left
	    // { HMM_V3( 0.0f, 2.0f, 0.0f ), 1.0f, HMM_V3( 0.0f, 0.0f, 1.0f ), 0.0f },   // Top
	    // { HMM_V3( 0.0f, 0.5f, -2.0f ), 1.0f, HMM_V3( 1.0f, 1.0f, 0.5f ), 0.0f }, // Behind/below
	};

	CLightBuffer lightBuffer = { 0 };
	lightBuffer.lightCount   = ARRAY_COUNT( lights );

	memcpy( lightBuffer.lights, lights, sizeof( lights ) );

	D3D11_MAPPED_SUBRESOURCE mapped;
	if ( SUCCEEDED( g_pImmediateContext->lpVtbl->Map( g_pImmediateContext,
	                                                  (ID3D11Resource *)g_pCBufferLights,
	                                                  0,
	                                                  D3D11_MAP_WRITE_DISCARD,
	                                                  0,
	                                                  &mapped ) ) )
	{
		memcpy( mapped.pData, &lightBuffer, sizeof( lightBuffer ) );
		g_pImmediateContext->lpVtbl->Unmap( g_pImmediateContext, (ID3D11Resource *)g_pCBufferLights, 0 );
	}

	// === Rendering ===
	g_pImmediateContext->lpVtbl->VSSetConstantBuffers( g_pImmediateContext, 0, 1, &g_pCBufferTransforms );
	g_pImmediateContext->lpVtbl->PSSetConstantBuffers( g_pImmediateContext, 0, 1, &g_pCBufferTransforms );
	g_pImmediateContext->lpVtbl->PSSetConstantBuffers( g_pImmediateContext, 1, 1, &g_pCBufferLights );

	const float ClearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	g_pImmediateContext->lpVtbl->ClearRenderTargetView( g_pImmediateContext, g_pRenderTargetView, ClearColor );
	g_pImmediateContext->lpVtbl->ClearDepthStencilView( g_pImmediateContext,
	                                                    g_pDepthStencilView,
	                                                    D3D11_CLEAR_DEPTH,
	                                                    1.0f,
	                                                    0 );

	g_pImmediateContext->lpVtbl->VSSetShader( g_pImmediateContext, g_pVertexShader, NULL, 0 );
	g_pImmediateContext->lpVtbl->PSSetShader( g_pImmediateContext, g_pPixelShader, NULL, 0 );
	g_pImmediateContext->lpVtbl->DrawIndexed( g_pImmediateContext, index_count, 0, 0 );
	g_pSwapChain->lpVtbl->Present( g_pSwapChain, 0, 0 );
}

HRESULT D3D11_Init()
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC sd               = { 0 };
	sd.BufferCount                        = 2;
	sd.BufferDesc.Width                   = wndWidth;
	sd.BufferDesc.Height                  = wndHeight;
	sd.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow                       = g_hWnd;
	sd.SampleDesc.Count                   = 1;
	sd.SampleDesc.Quality                 = 0;
	sd.Windowed                           = TRUE;

	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

	hr = D3D11CreateDeviceAndSwapChain( NULL,
	                                    D3D_DRIVER_TYPE_HARDWARE,
	                                    NULL,
	                                    0,
	                                    &featureLevel,
	                                    1,
	                                    D3D11_SDK_VERSION,
	                                    &sd,
	                                    &g_pSwapChain,
	                                    &g_pd3dDevice,
	                                    NULL,
	                                    &g_pImmediateContext );

	if ( FAILED( hr ) )
		return hr;

	ID3D11Texture2D *pBackBuffer = NULL;
	hr = g_pSwapChain->lpVtbl->GetBuffer( g_pSwapChain, 0, &IID_ID3D11Texture2D, (LPVOID *)&pBackBuffer );
	if ( FAILED( hr ) )
		return hr;

	hr = g_pd3dDevice->lpVtbl->CreateRenderTargetView( g_pd3dDevice,
	                                                   (ID3D11Resource *)pBackBuffer,
	                                                   NULL,
	                                                   &g_pRenderTargetView );
	pBackBuffer->lpVtbl->Release( pBackBuffer );
	if ( FAILED( hr ) )
		return hr;

	g_pImmediateContext->lpVtbl->OMSetRenderTargets( g_pImmediateContext, 1, &g_pRenderTargetView, NULL );
	D3D11_VIEWPORT vp = { 0 };
	vp.Width          = (float)wndWidth;
	vp.Height         = (float)wndHeight;
	vp.MinDepth       = 0.0f;
	vp.MaxDepth       = 1.0f;
	vp.TopLeftX       = 0;
	vp.TopLeftY       = 0;
	g_pImmediateContext->lpVtbl->RSSetViewports( g_pImmediateContext, 1, &vp );

	D3D11_TEXTURE2D_DESC descDepth = { 0 };
	descDepth.Width                = wndWidth;
	descDepth.Height               = wndHeight;
	descDepth.MipLevels            = 1;
	descDepth.ArraySize            = 1;
	descDepth.Format               = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count     = 1;
	descDepth.SampleDesc.Quality   = 0;
	descDepth.Usage                = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags            = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags       = 0;
	descDepth.MiscFlags            = 0;

	hr = g_pd3dDevice->lpVtbl->CreateTexture2D( g_pd3dDevice, &descDepth, NULL, &g_pDepthStencil );
	if ( FAILED( hr ) )
		return hr;

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = { 0 };
	descDSV.Format                        = descDepth.Format;
	descDSV.ViewDimension                 = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice            = 0;

	hr = g_pd3dDevice->lpVtbl->CreateDepthStencilView( g_pd3dDevice,
	                                                   (ID3D11Resource *)g_pDepthStencil,
	                                                   &descDSV,
	                                                   &g_pDepthStencilView );
	if ( FAILED( hr ) )
		return hr;

	g_pImmediateContext->lpVtbl->OMSetRenderTargets( g_pImmediateContext,
	                                                 1,
	                                                 &g_pRenderTargetView,
	                                                 g_pDepthStencilView );

	int      texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load( ".\\DEER\\deer.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );

	if ( !pixels )
	{
		fprintf( stderr, "Failed to load texture image: %s\n", "file" );
		exit( 1 );
	}

	D3D11_TEXTURE2D_DESC texDesc = {
	    .Width          = texWidth,
	    .Height         = texHeight,
	    .MipLevels      = 1,
	    .ArraySize      = 1,
	    .Format         = DXGI_FORMAT_R8G8B8A8_UNORM,
	    .SampleDesc     = { 1, 0 },
	    .Usage          = D3D11_USAGE_DEFAULT,
	    .BindFlags      = D3D11_BIND_SHADER_RESOURCE,
	    .CPUAccessFlags = 0,
	    .MiscFlags      = 0,
	};

	int bytesPerPixel = 4;

	D3D11_SUBRESOURCE_DATA initData = {
	    .pSysMem     = pixels,
	    .SysMemPitch = texWidth * bytesPerPixel,
	};

	hr = g_pd3dDevice->lpVtbl->CreateTexture2D( g_pd3dDevice, &texDesc, &initData, &texture );
	if ( SUCCEEDED( hr ) )
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {
		    .Format        = texDesc.Format,
		    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
		    .Texture2D =
		        {
		            .MostDetailedMip = 0,
		            .MipLevels       = 1,
		        },
		};

		hr = g_pd3dDevice->lpVtbl->CreateShaderResourceView( g_pd3dDevice,
		                                                     (ID3D11Resource *)texture,
		                                                     &srvDesc,
		                                                     &textureView );
	}

	return S_OK;
}

HRESULT D3D11_SetupPipeline()
{
	HRESULT   hr         = S_OK;
	ID3DBlob *pVSBlob    = NULL;
	ID3DBlob *pErrorBlob = NULL;

	size_t vertSize = strlen( vertexShaderSource );
	hr              = D3DCompile( vertexShaderSource,
                     vertSize,
                     "VS_Main",
                     NULL,
                     NULL,
                     "VS_Main",
                     "vs_4_0",
                     0,
                     0,
                     &pVSBlob,
                     &pErrorBlob );
	if ( FAILED( hr ) )
	{
		if ( pErrorBlob )
		{
			printf( "Vertex shader compilation error: %s\n",
			        (char *)pErrorBlob->lpVtbl->GetBufferPointer( pErrorBlob ) );
			pErrorBlob->lpVtbl->Release( pErrorBlob );
		}
		return hr;
	}

	hr = g_pd3dDevice->lpVtbl->CreateVertexShader( g_pd3dDevice,
	                                               pVSBlob->lpVtbl->GetBufferPointer( pVSBlob ),
	                                               pVSBlob->lpVtbl->GetBufferSize( pVSBlob ),
	                                               NULL,
	                                               &g_pVertexShader );
	if ( FAILED( hr ) )
	{
		pVSBlob->lpVtbl->Release( pVSBlob );
		return hr;
	}

	D3D11_INPUT_ELEMENT_DESC layout[] = {
	    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( Vertex, x ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof( Vertex, nx ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( Vertex, u ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = g_pd3dDevice->lpVtbl->CreateInputLayout( g_pd3dDevice,
	                                              layout,
	                                              ARRAY_COUNT( layout ),
	                                              pVSBlob->lpVtbl->GetBufferPointer( pVSBlob ),
	                                              pVSBlob->lpVtbl->GetBufferSize( pVSBlob ),
	                                              &g_pVertexLayout );
	pVSBlob->lpVtbl->Release( pVSBlob );
	if ( FAILED( hr ) )
		return hr;

	g_pImmediateContext->lpVtbl->IASetInputLayout( g_pImmediateContext, g_pVertexLayout );

	ID3DBlob *pPSBlob = NULL;
	size_t    pixSize = strlen( pixelShaderSource );

	hr = D3DCompile( pixelShaderSource,
	                 pixSize,
	                 "PS_Main",
	                 NULL,
	                 NULL,
	                 "PS_Main",
	                 "ps_4_0",
	                 0,
	                 0,
	                 &pPSBlob,
	                 &pErrorBlob );
	if ( FAILED( hr ) )
	{
		if ( pErrorBlob )
		{
			printf( "Pixel shader compilation error: %s\n",
			        (char *)pErrorBlob->lpVtbl->GetBufferPointer( pErrorBlob ) );
			pErrorBlob->lpVtbl->Release( pErrorBlob );
		}
		return hr;
	}

	hr = g_pd3dDevice->lpVtbl->CreatePixelShader( g_pd3dDevice,
	                                              pPSBlob->lpVtbl->GetBufferPointer( pPSBlob ),
	                                              pPSBlob->lpVtbl->GetBufferSize( pPSBlob ),
	                                              NULL,
	                                              &g_pPixelShader );
	pPSBlob->lpVtbl->Release( pPSBlob );
	if ( FAILED( hr ) )
		return hr;

	D3D11_BUFFER_DESC cUniformBufferDesc = { 0 };
	cUniformBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
	cUniformBufferDesc.ByteWidth         = sizeof( CUniformBuffer );
	cUniformBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
	cUniformBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

	hr = g_pd3dDevice->lpVtbl->CreateBuffer( g_pd3dDevice, &cUniformBufferDesc, NULL, &g_pCBufferTransforms );
	if ( FAILED( hr ) )
		return hr;

	D3D11_BUFFER_DESC cLightsBufferDesc = { 0 };
	cLightsBufferDesc.Usage             = D3D11_USAGE_DYNAMIC;
	cLightsBufferDesc.ByteWidth         = sizeof( CLightBuffer );
	cLightsBufferDesc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
	cLightsBufferDesc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

	hr = g_pd3dDevice->lpVtbl->CreateBuffer( g_pd3dDevice, &cLightsBufferDesc, NULL, &g_pCBufferLights );
	if ( FAILED( hr ) )
		return hr;

	D3D11_BUFFER_DESC vboDesc = { 0 };
	vboDesc.Usage             = D3D11_USAGE_DEFAULT;
	vboDesc.ByteWidth         = sizeof( Vertex ) * vertex_count;
	vboDesc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
	vboDesc.CPUAccessFlags    = 0;

	D3D11_SUBRESOURCE_DATA initalVertexData = { 0 };
	initalVertexData.pSysMem                = vertices;

	hr = g_pd3dDevice->lpVtbl->CreateBuffer( g_pd3dDevice, &vboDesc, &initalVertexData, &g_pVertexBuffer );
	if ( FAILED( hr ) )
		return hr;

	D3D11_BUFFER_DESC iboDesc = { 0 };
	iboDesc.Usage             = D3D11_USAGE_DEFAULT;
	iboDesc.ByteWidth         = sizeof( unsigned short ) * index_count;
	iboDesc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
	iboDesc.CPUAccessFlags    = 0;

	D3D11_SUBRESOURCE_DATA initalIndexData = { 0 };
	initalIndexData.pSysMem                = indices;

	hr = g_pd3dDevice->lpVtbl->CreateBuffer( g_pd3dDevice, &iboDesc, &initalIndexData, &g_pIndexBuffer );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	UINT stride = sizeof( Vertex );
	UINT offset = 0;
	g_pImmediateContext->lpVtbl->IASetVertexBuffers( g_pImmediateContext, 0, 1, &g_pVertexBuffer, &stride, &offset );
	g_pImmediateContext->lpVtbl->IASetIndexBuffer( g_pImmediateContext, g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );
	g_pImmediateContext->lpVtbl->IASetPrimitiveTopology( g_pImmediateContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	D3D11_RASTERIZER_DESC rastDesc = {
	    .FillMode              = D3D11_FILL_SOLID,
	    .CullMode              = D3D11_CULL_NONE,
	    .FrontCounterClockwise = FALSE,
	    .DepthClipEnable       = TRUE,
	    .ScissorEnable         = FALSE,
	    .MultisampleEnable     = FALSE,
	    .AntialiasedLineEnable = FALSE,
	};

	ID3D11RasterizerState *pRasterizerState = NULL;
	hr = g_pd3dDevice->lpVtbl->CreateRasterizerState( g_pd3dDevice, &rastDesc, &pRasterizerState );
	if ( FAILED( hr ) )
		return hr;

	g_pImmediateContext->lpVtbl->RSSetState( g_pImmediateContext, pRasterizerState );

	D3D11_VIEWPORT vp;
	vp.Width    = wndWidth;
	vp.Height   = wndHeight;
	vp.MaxDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->lpVtbl->RSSetViewports( g_pImmediateContext, 1U, &vp );

	ID3D11SamplerState *samplerState = NULL;

	D3D11_SAMPLER_DESC sampDesc = {
	    .Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT,
	    .AddressU       = D3D11_TEXTURE_ADDRESS_WRAP,
	    .AddressV       = D3D11_TEXTURE_ADDRESS_WRAP,
	    .AddressW       = D3D11_TEXTURE_ADDRESS_WRAP,
	    .MipLODBias     = 0.0f,
	    .MaxAnisotropy  = 1,
	    .ComparisonFunc = D3D11_COMPARISON_ALWAYS,
	    .BorderColor    = { 0, 0, 0, 0 },
	    .MinLOD         = 0,
	    .MaxLOD         = D3D11_FLOAT32_MAX,
	};
	hr = g_pd3dDevice->lpVtbl->CreateSamplerState( g_pd3dDevice, &sampDesc, &samplerState );

	g_pImmediateContext->lpVtbl->PSSetShaderResources( g_pImmediateContext, 0, 1, &textureView );
	g_pImmediateContext->lpVtbl->PSSetSamplers( g_pImmediateContext, 0, 1, &samplerState );

	return S_OK;
}

void D3D11_CleanupDevice()
{

	if ( g_pCBufferLights )
		g_pCBufferLights->lpVtbl->Release( g_pCBufferLights );

	if ( g_pCBufferTransforms )
		g_pCBufferTransforms->lpVtbl->Release( g_pCBufferTransforms );

	if ( g_pDepthStencilView )
		g_pDepthStencilView->lpVtbl->Release( g_pDepthStencilView );
	if ( g_pDepthStencil )
		g_pDepthStencil->lpVtbl->Release( g_pDepthStencil );
	if ( g_pImmediateContext )
		g_pImmediateContext->lpVtbl->ClearState( g_pImmediateContext );
	if ( g_pIndexBuffer )
		g_pIndexBuffer->lpVtbl->Release( g_pIndexBuffer );
	if ( g_pVertexBuffer )
		g_pVertexBuffer->lpVtbl->Release( g_pVertexBuffer );
	if ( g_pVertexLayout )
		g_pVertexLayout->lpVtbl->Release( g_pVertexLayout );
	if ( g_pVertexShader )
		g_pVertexShader->lpVtbl->Release( g_pVertexShader );
	if ( g_pPixelShader )
		g_pPixelShader->lpVtbl->Release( g_pPixelShader );
	if ( g_pRenderTargetView )
		g_pRenderTargetView->lpVtbl->Release( g_pRenderTargetView );
	if ( g_pSwapChain )
		g_pSwapChain->lpVtbl->Release( g_pSwapChain );
	if ( g_pImmediateContext )
		g_pImmediateContext->lpVtbl->Release( g_pImmediateContext );
	if ( g_pd3dDevice )
		g_pd3dDevice->lpVtbl->Release( g_pd3dDevice );
}
