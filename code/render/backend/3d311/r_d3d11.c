#include "../api.h"

#include <d3dcompiler.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxgi.lib" )

struct R_Context
{
	ID3D11Device           *device;
	ID3D11DeviceContext    *ctx;
	IDXGISwapChain         *swap;
	ID3D11RenderTargetView *rtv;
	D3D11_VIEWPORT          vp;
	bool                    vsync;
	R_Pipeline             *lastPipeline;
	UINT                    lastStride;
	UINT                    lastOffset;
};

struct R_Buffer
{
	ID3D11Buffer *buf;
	size_t        size;
};

struct R_Shader
{
	ID3DBlob           *blob;
	ID3D11VertexShader *vs;
	ID3D11PixelShader  *ps;
	const void         *bytecode;
	size_t              bytecodeSize;
};

struct R_InputLayout
{
	ID3D11InputLayout *layout;
};

struct R_Pipeline
{
	ID3D11VertexShader *vs;
	ID3D11PixelShader  *ps;
	R_InputLayout      *layout;
};

static void safe_release( IUnknown **p )
{
	if ( p && *p )
	{
		( *p )->lpVtbl->Release( *p );
		*p = NULL;
	}
}

R_Context *r_create_context( HWND hwnd, int width, int height, bool vsync )
{
	UINT createFlags = 0;
#if defined( _DEBUG )
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	D3D_FEATURE_LEVEL chosen          = D3D_FEATURE_LEVEL_11_0;

	IDXGISwapChain      *swap   = NULL;
	ID3D11Device        *device = NULL;
	ID3D11DeviceContext *ctx    = NULL;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount       = 1;
	sd.BufferDesc.Width  = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow      = hwnd;
	sd.SampleDesc.Count  = 1;
	sd.Windowed          = TRUE;
	sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

	HRESULT hr = D3D11CreateDeviceAndSwapChain( NULL,
	                                            D3D_DRIVER_TYPE_HARDWARE,
	                                            NULL,
	                                            createFlags,
	                                            featureLevels,
	                                            ARRAYSIZE( featureLevels ),
	                                            D3D11_SDK_VERSION,
	                                            &sd,
	                                            &swap,
	                                            &device,
	                                            &chosen,
	                                            &ctx );

	if ( FAILED( hr ) )
	{
		return NULL;
	}

	ID3D11Texture2D *backbuf = NULL;
	hr                       = swap->lpVtbl->GetBuffer( swap, 0, &IID_ID3D11Texture2D, (void **)&backbuf );
	if ( FAILED( hr ) )
	{
		safe_release( (IUnknown **)&swap );
		safe_release( (IUnknown **)&device );
		safe_release( (IUnknown **)&ctx );
		return NULL;
	}

	ID3D11RenderTargetView *rtv = NULL;
	hr = device->lpVtbl->CreateRenderTargetView( device, (ID3D11Resource *)backbuf, NULL, &rtv );
	backbuf->lpVtbl->Release( backbuf );
	if ( FAILED( hr ) )
	{
		safe_release( (IUnknown **)&swap );
		safe_release( (IUnknown **)&device );
		safe_release( (IUnknown **)&ctx );
		return NULL;
	}

	R_Context *r = (R_Context *)malloc( sizeof( R_Context ) );
	memset( r, 0, sizeof( *r ) );
	r->device = device;
	r->ctx    = ctx;
	r->swap   = swap;
	r->rtv    = rtv;
	r->vsync  = vsync;

	// default viewport
	r->vp.TopLeftX = 0.0f;
	r->vp.TopLeftY = 0.0f;
	r->vp.Width    = (FLOAT)width;
	r->vp.Height   = (FLOAT)height;
	r->vp.MinDepth = 0.0f;
	r->vp.MaxDepth = 1.0f;

	ctx->lpVtbl->OMSetRenderTargets( ctx, 1, &r->rtv, NULL );
	ctx->lpVtbl->RSSetViewports( ctx, 1, &r->vp );

	return r;
}

void r_destroy_context( R_Context *ctx )
{
	if ( !ctx )
		return;
	safe_release( (IUnknown **)&ctx->rtv );
	safe_release( (IUnknown **)&ctx->swap );
	safe_release( (IUnknown **)&ctx->ctx );
	safe_release( (IUnknown **)&ctx->device );
	free( ctx );
}

void r_present( R_Context *ctx )
{
	if ( !ctx )
		return;
	ctx->swap->lpVtbl->Present( ctx->swap, ctx->vsync ? 1 : 0, 0 );
}

void r_clear_render_target( R_Context *ctx, float r, float g, float b, float a )
{
	if ( !ctx )
		return;
	float clearColor[4] = { r, g, b, a };
	ctx->ctx->lpVtbl->ClearRenderTargetView( ctx->ctx, ctx->rtv, clearColor );
}

void r_set_viewport( R_Context *ctx, float x, float y, float w, float h )
{
	if ( !ctx )
		return;

	ctx->vp.TopLeftX = x;
	ctx->vp.TopLeftY = y;
	ctx->vp.Width    = w;
	ctx->vp.Height   = h;
	ctx->vp.MinDepth = 0.0f;
	ctx->vp.MaxDepth = 1.0f;

	ctx->ctx->lpVtbl->RSSetViewports( ctx->ctx, 1, &ctx->vp );
}

R_Buffer *r_create_buffer( R_Context *ctx, const void *data, size_t bytes, bool dynamic, UINT bindFlags )
{
	if ( !ctx )
		return NULL;

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ) );
	bd.ByteWidth           = (UINT)bytes;
	bd.BindFlags           = bindFlags;
	bd.CPUAccessFlags      = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	bd.Usage               = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	bd.MiscFlags           = 0;
	bd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory( &initData, sizeof( initData ) );
	initData.pSysMem = data;

	ID3D11Buffer *buf = NULL;
	HRESULT       hr  = ctx->device->lpVtbl->CreateBuffer( ctx->device, &bd, data ? &initData : NULL, &buf );
	if ( FAILED( hr ) )
		return NULL;

	R_Buffer *b = (R_Buffer *)malloc( sizeof( R_Buffer ) );
	b->buf      = buf;
	b->size     = bytes;
	return b;
}

R_Buffer *r_create_constant_buffer( R_Context *ctx, size_t size )
{
	if ( !ctx )
		return NULL;

	// 16 byte guaranteed alignment
	size_t alignedSize = ( size + 15 ) & ~15;
	return r_create_buffer( ctx, NULL, alignedSize, true, D3D11_BIND_CONSTANT_BUFFER );
}

void r_update_buffer( R_Context *ctx, R_Buffer *buf, const void *data, size_t bytes, size_t offset )
{
	if ( !ctx || !buf || !data )
		return;
	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT hr = ctx->ctx->lpVtbl->Map( ctx->ctx, (ID3D11Resource *)buf->buf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped );
	if ( SUCCEEDED( hr ) )
	{
		memcpy( (uint8_t *)mapped.pData + offset, data, bytes );
		ctx->ctx->lpVtbl->Unmap( ctx->ctx, (ID3D11Resource *)buf->buf, 0 );
	}
	else
	{
		// fallback
		ctx->ctx->lpVtbl->UpdateSubresource( ctx->ctx, (ID3D11Resource *)buf->buf, 0, NULL, data, 0, 0 );
	}
}

void r_bind_constant_buffer( R_Context *ctx, R_Buffer *cb, int slot )
{
	if ( !ctx )
		return;

	ID3D11Buffer *buf = cb ? cb->buf : NULL;
	ctx->ctx->lpVtbl->VSSetConstantBuffers( ctx->ctx, slot, 1, &buf );
	ctx->ctx->lpVtbl->PSSetConstantBuffers( ctx->ctx, slot, 1, &buf );
}

void r_destroy_buffer( R_Buffer *buf )
{
	if ( !buf )
		return;
	safe_release( (IUnknown **)&buf->buf );
	free( buf );
}

static ID3DBlob *compile_hlsl( const char *src, const char *entry, const char *profile )
{
	ID3DBlob *blob = NULL;
	ID3DBlob *err  = NULL;
	HRESULT   hr   = D3DCompile( src,
                             strlen( src ),
                             NULL,
                             NULL,
                             NULL,
                             entry,
                             profile,
                             D3DCOMPILE_OPTIMIZATION_LEVEL3,
                             0,
                             &blob,
                             &err );
	if ( FAILED( hr ) )
	{
		if ( err )
		{
			OutputDebugStringA( (const char *)err->lpVtbl->GetBufferPointer( err ) );
			err->lpVtbl->Release( err );
		}
		safe_release( (IUnknown **)&blob );
		return NULL;
	}
	safe_release( (IUnknown **)&err );
	return blob;
}

R_Shader *r_create_vertex_shader_from_source( R_Context *ctx, const char *src, const char *entry, const char *profile )
{
	if ( !ctx )
		return NULL;
	ID3DBlob *blob = compile_hlsl( src, entry, profile );
	if ( !blob )
		return NULL;

	void               *pShaderBytecode = blob->lpVtbl->GetBufferPointer( blob );
	size_t              byteCodeLength  = blob->lpVtbl->GetBufferSize( blob );
	ID3D11VertexShader *vs              = NULL;
	HRESULT hr = ctx->device->lpVtbl->CreateVertexShader( ctx->device, pShaderBytecode, byteCodeLength, NULL, &vs );
	if ( FAILED( hr ) )
	{
		blob->lpVtbl->Release( blob );
		return NULL;
	}
	R_Shader *s = (R_Shader *)malloc( sizeof( R_Shader ) );
	s->blob     = blob;
	s->vs       = vs;
	s->ps       = NULL;
	return s;
}

R_Shader *r_create_pixel_shader_from_source( R_Context *ctx, const char *src, const char *entry, const char *profile )
{
	if ( !ctx )
		return NULL;
	ID3DBlob *blob = compile_hlsl( src, entry, profile );
	if ( !blob )
		return NULL;

	const void *pShaderBytecode = blob->lpVtbl->GetBufferPointer( blob );
	size_t      byteCodeLength  = blob->lpVtbl->GetBufferSize( blob );

	ID3D11PixelShader *ps = NULL;
	HRESULT hr = ctx->device->lpVtbl->CreatePixelShader( ctx->device, pShaderBytecode, byteCodeLength, NULL, &ps );
	if ( FAILED( hr ) )
	{
		blob->lpVtbl->Release( blob );
		return NULL;
	}
	R_Shader *s = (R_Shader *)malloc( sizeof( R_Shader ) );
	s->blob     = blob;
	s->vs       = NULL;
	s->ps       = ps;
	return s;
}

R_Shader *r_create_vertex_shader_from_cso( R_Context *ctx, const void *bytecode, size_t bytecodeSize )
{
	if ( !ctx || !bytecode || bytecodeSize == 0 )
		return NULL;

	ID3D11VertexShader *vs = NULL;
	HRESULT             hr = ctx->device->lpVtbl->CreateVertexShader( ctx->device, bytecode, bytecodeSize, NULL, &vs );
	if ( FAILED( hr ) )
		return NULL;

	R_Shader *s     = (R_Shader *)malloc( sizeof( R_Shader ) );
	s->blob         = NULL;
	s->vs           = vs;
	s->ps           = NULL;
	s->bytecode     = bytecode;
	s->bytecodeSize = bytecodeSize;
	return s;
}

R_Shader *r_create_pixel_shader_from_cso( R_Context *ctx, const void *bytecode, size_t bytecodeSize )
{
	if ( !ctx || !bytecode || bytecodeSize == 0 )
		return NULL;

	ID3D11PixelShader *ps = NULL;
	HRESULT            hr = ctx->device->lpVtbl->CreatePixelShader( ctx->device, bytecode, bytecodeSize, NULL, &ps );

	if ( FAILED( hr ) )
		return NULL;

	R_Shader *s     = (R_Shader *)malloc( sizeof( R_Shader ) );
	s->blob         = NULL;
	s->vs           = NULL;
	s->ps           = ps;
	s->bytecode     = bytecode;
	s->bytecodeSize = bytecodeSize;
	return s;
}

const void *r_shader_get_blob( const R_Shader *shader, size_t *outSize )
{
	if ( !shader )
		return NULL;

	if ( shader->blob )
	{
		if ( outSize )
			*outSize = shader->blob->lpVtbl->GetBufferSize( shader->blob );
		return shader->blob->lpVtbl->GetBufferPointer( shader->blob );
	}
	else
	{
		if ( outSize )
			*outSize = shader->bytecodeSize;
		return shader->bytecode;
	}
}

void r_destroy_shader( R_Shader *sh )
{
	if ( !sh )
		return;
	safe_release( (IUnknown **)&sh->blob );
	safe_release( (IUnknown **)&sh->vs );
	safe_release( (IUnknown **)&sh->ps );
	free( sh );
}

R_InputLayout *r_create_input_layout( R_Context                      *ctx,
                                      const D3D11_INPUT_ELEMENT_DESC *desc,
                                      UINT                            numDesc,
                                      const void                     *vsBlob,
                                      SIZE_T                          vsBlobSize )
{
	if ( !ctx )
		return NULL;

	ID3D11InputLayout *layout = NULL;
	HRESULT hr = ctx->device->lpVtbl->CreateInputLayout( ctx->device, desc, numDesc, vsBlob, vsBlobSize, &layout );
	if ( FAILED( hr ) )
		return NULL;

	R_InputLayout *l = (R_InputLayout *)malloc( sizeof( R_InputLayout ) );
	l->layout        = layout;
	return l;
}

void r_destroy_input_layout( R_InputLayout *layout )
{
	if ( !layout )
		return;
	safe_release( (IUnknown **)&layout->layout );
	free( layout );
}

R_Pipeline *r_create_pipeline( R_Context *ctx, ID3D11VertexShader *vs, ID3D11PixelShader *ps, R_InputLayout *layout )
{
	if ( !ctx )
		return NULL;
	R_Pipeline *p = (R_Pipeline *)malloc( sizeof( R_Pipeline ) );
	p->vs         = vs;
	p->ps         = ps;
	p->layout     = layout;
	return p;
}

void r_bind_pipeline( R_Context *ctx, R_Pipeline *pipe )
{
	if ( !ctx )
		return;
	if ( ctx->lastPipeline == pipe )
		return; // cheap cache
	if ( pipe )
	{
		ctx->ctx->lpVtbl->IASetInputLayout( ctx->ctx, pipe->layout ? pipe->layout->layout : NULL );
		ctx->ctx->lpVtbl->VSSetShader( ctx->ctx, pipe->vs, NULL, 0 );
		ctx->ctx->lpVtbl->PSSetShader( ctx->ctx, pipe->ps, NULL, 0 );
	}
	else
	{
		ctx->ctx->lpVtbl->IASetInputLayout( ctx->ctx, NULL );
		ctx->ctx->lpVtbl->VSSetShader( ctx->ctx, NULL, NULL, 0 );
		ctx->ctx->lpVtbl->PSSetShader( ctx->ctx, NULL, NULL, 0 );
	}
	ctx->lastPipeline = pipe;
}

void r_destroy_pipeline( R_Pipeline *pipe )
{
	if ( !pipe )
		return;
	// ~dkv
	free( pipe );
}

void r_set_vertex_buffer( R_Context *ctx, R_Buffer *vb, UINT stride, UINT offset )
{
	if ( !ctx )
		return;
	ID3D11Buffer *b = vb ? vb->buf : NULL;
	ctx->ctx->lpVtbl->IASetVertexBuffers( ctx->ctx, 0, 1, &b, &stride, &offset );
	ctx->lastStride = stride;
	ctx->lastOffset = offset;
}

void r_set_index_buffer( R_Context *ctx, R_Buffer *ib, DXGI_FORMAT fmt, UINT offset )
{
	if ( !ctx )
		return;
	ctx->ctx->lpVtbl->IASetIndexBuffer( ctx->ctx, ib ? ib->buf : NULL, fmt, offset );
}

void r_set_primitive_topology( R_Context *ctx, D3D11_PRIMITIVE_TOPOLOGY prim )
{
	if ( !ctx )
		return;
	ctx->ctx->lpVtbl->IASetPrimitiveTopology( ctx->ctx, prim );
}

void r_draw( R_Context *ctx, UINT vertexCount, UINT startVertex )
{
	if ( !ctx )
		return;
	ctx->ctx->lpVtbl->Draw( ctx->ctx, vertexCount, startVertex );
}

void r_draw_indexed( R_Context *ctx, UINT indexCount, UINT startIndex, INT baseVertex )
{
	if ( !ctx )
		return;
	ctx->ctx->lpVtbl->DrawIndexed( ctx->ctx, indexCount, startIndex, baseVertex );
}

ID3D11Device *r_get_device( R_Context *ctx )
{
	return ctx ? ctx->device : NULL;
}

ID3D11DeviceContext *r_get_imm_context( R_Context *ctx )
{
	return ctx ? ctx->ctx : NULL;
}
