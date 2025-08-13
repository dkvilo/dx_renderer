#include <windows.h>
#include <math.h>
#include "render/backend/3d311/r_d3d11.c"
#include "render/generated/ui_pass.vtx.h"
#include "base/c_io.c"

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
	WNDCLASS wc      = { 0 };
	wc.lpfnWndProc   = wnd_proc;
	wc.hInstance     = hInst;
	wc.lpszClassName = "D3D11Example";
	RegisterClass( &wc );

	HWND hwnd = CreateWindow( wc.lpszClassName,
	                          "Hello DirectX 11",
	                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                          CW_USEDEFAULT,
	                          CW_USEDEFAULT,
	                          800,
	                          600,
	                          0,
	                          0,
	                          hInst,
	                          0 );

	R_Context *ctx = r_create_context( hwnd, 800, 600, true );
	if ( !ctx )
	{
		MessageBox( hwnd, "Failed to init D3D11", "Error", MB_OK );
		return -1;
	}

	Geometry2D_Vertex verts[] = {
	    { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	    { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	    { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
	};

	R_Buffer *vb = r_create_buffer( ctx, verts, sizeof( verts ), false, D3D11_BIND_VERTEX_BUFFER );
	R_Buffer *cb = r_create_constant_buffer( ctx, sizeof( Geometry2D_Transform ) );

	size_t    vsByteCodeLength;
	void     *vsByteCode = io_read_bin_content( "vertex.cso", &vsByteCodeLength );
	R_Shader *vs         = r_create_vertex_shader_from_cso( ctx, vsByteCode, vsByteCodeLength );

	size_t    psByteCodeLength;
	void     *psByteCode = io_read_bin_content( "pixel.cso", &psByteCodeLength );
	R_Shader *ps         = r_create_pixel_shader_from_cso( ctx, psByteCode, psByteCodeLength );

	if ( !vs || !ps )
	{
		MessageBox( hwnd, "Failed to create shaders", "Error", MB_OK );
		return -1;
	}

	uint32_t       descCount = Geometry2D_Vertex_desc_count;
	R_InputLayout *il   = r_create_input_layout( ctx, Geometry2D_Vertex_desc, descCount, vsByteCode, vsByteCodeLength );
	R_Pipeline    *pipe = r_create_pipeline( ctx, vs->vs, ps->ps, il );

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

		r_update_buffer( ctx, cb, &transform, sizeof( transform ), 0 );
		r_clear_render_target( ctx, 0.1f, 0.1f, 0.2f, 1.0f );

		r_bind_pipeline( ctx, pipe );
		r_bind_constant_buffer( ctx, cb, 0 );
		r_set_vertex_buffer( ctx, vb, sizeof( Geometry2D_Vertex ), 0 );
		r_set_primitive_topology( ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

		r_draw( ctx, 3, 0 );
		r_present( ctx );
	}

	goto cleanup;

cleanup:
	r_destroy_pipeline( pipe );
	r_destroy_input_layout( il );
	r_destroy_shader( vs );
	r_destroy_shader( ps );
	r_destroy_buffer( vb );
	r_destroy_buffer( cb );
	r_destroy_context( ctx );

	return 0;
}
