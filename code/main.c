#include <windows.h>
#include <math.h>
#include "render/backend/3d311/r_d3d11.c"
#include "render/generated/ui_pass.vtx.h"
#include "base/c_io.c"
#include "platform/window.c"

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "user32.lib" )

static LRESULT CALLBACK wnd_proc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if ( msg == WM_DESTROY )
	{
		PostQuitMessage( 0 );
		return 0;
	}
	return DefWindowProc( hWnd, msg, wParam, lParam );
}

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
{
	HWND            hwnd       = NULL;
	R_Context      *ctx        = NULL;
	R_Buffer       *vb         = NULL;
	R_Buffer       *cb         = NULL;
	void           *vsByteCode = NULL;
	void           *psByteCode = NULL;
	R_VertexShader *vs         = NULL;
	R_PixelShader  *ps         = NULL;
	R_InputLayout  *il         = NULL;
	R_Pipeline     *pipe       = NULL;

	PWindowDescriptor wndDesc = {
	    .hInst         = hInst,
	    .lpfnWndProc   = wnd_proc,
	    .lpszClassName = "SKSTR_GAMES_D3D11RENDERER",
	    .Title         = "Hello, DirectX 11",
	    .Width         = 800,
	    .Height        = 600,
	};

	PlatformResult pRes = P_CreateWindow( &wndDesc );
	if ( !PlatformResult_Is_Ok( &pRes ) )
	{
		fprintf( stderr, "Window creation failed: %s\n", pRes.err.message );
		return pRes.err.code;
	}
	hwnd = pRes.ok.value;

	R_Result result;
	ctx = r_create_context( hwnd, wndDesc.Width, wndDesc.Height, true, &result );
	if ( !ctx )
	{
		MessageBox( hwnd, r_result_to_string( result ), "Error", MB_OK );
		return -1;
	}

	Geometry2D_Vertex verts[] = {
	    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	    { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	    { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
	};

	vb = r_create_buffer( ctx, verts, sizeof( verts ), false, D3D11_BIND_VERTEX_BUFFER, &result );
	if ( !vb )
	{
		MessageBox( hwnd, "Failed to create vertex buffer", "Error", MB_OK );
		goto cleanup;
	}

	cb = r_create_constant_buffer( ctx, sizeof( Geometry2D_Transform ), &result );
	if ( !cb )
	{
		MessageBox( hwnd, "Failed to create constant buffer", "Error", MB_OK );
		goto cleanup;
	}

	size_t vsByteCodeLength;
	vsByteCode = io_read_bin_content( "vertex.cso", &vsByteCodeLength );
	if ( !vsByteCode )
	{
		MessageBox( hwnd, "Failed to load vertex.cso", "Error", MB_OK );
		goto cleanup;
	}

	size_t psByteCodeLength;
	psByteCode = io_read_bin_content( "pixel.cso", &psByteCodeLength );
	if ( !psByteCode )
	{
		MessageBox( hwnd, "Failed to load pixel.cso", "Error", MB_OK );
		goto cleanup;
	}

	vs = r_create_vertex_shader_from_bytecode( ctx, vsByteCode, vsByteCodeLength, &result );
	if ( !vs )
	{
		MessageBox( hwnd, "Failed to create vertex shader", "Error", MB_OK );
		goto cleanup;
	}

	ps = r_create_pixel_shader_from_bytecode( ctx, psByteCode, psByteCodeLength, &result );
	if ( !ps )
	{
		MessageBox( hwnd, "Failed to create pixel shader", "Error", MB_OK );
		goto cleanup;
	}

	uint32_t descCount = Geometry2D_Vertex_desc_count;
	il                 = r_create_input_layout( ctx, Geometry2D_Vertex_desc, descCount, vs, &result );
	if ( !il )
	{
		MessageBox( hwnd, "Failed to create input layout", "Error", MB_OK );
		goto cleanup;
	}

	pipe = r_create_pipeline( ctx, vs, ps, il, &result );
	if ( !pipe )
	{
		MessageBox( hwnd, "Failed to create pipeline", "Error", MB_OK );
		goto cleanup;
	}

	DWORD startTime = GetTickCount();
	MSG   msg       = { 0 };

	while ( msg.message != WM_QUIT )
	{
		if ( PeekMessage( &msg, 0, 0, 0, PM_REMOVE ) )
		{
			DispatchMessage( &msg );
		}

		DWORD currentTime = GetTickCount();
		float time        = ( currentTime - startTime ) / 1000.0f;

		Geometry2D_Transform transform = {
		    .time    = time,
		    .scale   = 0.8f + 0.2f * sinf( time * 0.5f ),
		    .padding = { 0, 0 },
		};
		r_update_buffer( ctx, cb, &transform, sizeof( transform ) );

		r_clear_render_target( ctx, 0.1f, 0.1f, 0.2f, 1.0f );
		r_bind_pipeline( ctx, pipe );
		r_bind_constant_buffer( ctx, cb, 0 );
		r_set_vertex_buffer( ctx, vb, sizeof( Geometry2D_Vertex ), 0 );
		r_set_primitive_topology( ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
		r_draw( ctx, 3, 0 );
		r_present( ctx );
	}

cleanup:
	r_destroy_pipeline( pipe );

	r_destroy_input_layout( il );
	r_destroy_vertex_shader( vs );
	r_destroy_pixel_shader( ps );

	free( vsByteCode );
	free( psByteCode );

	r_destroy_buffer( cb );
	r_destroy_buffer( vb );
	r_destroy_context( ctx );

	return 0;
}
