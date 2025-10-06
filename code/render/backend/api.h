#ifndef R_API_H
#define R_API_H

#include <windows.h>
#include <d3d11.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct R_Context      R_Context;
	typedef struct R_Buffer       R_Buffer;
	typedef struct R_VertexShader R_VertexShader;
	typedef struct R_PixelShader  R_PixelShader;
	typedef struct R_InputLayout  R_InputLayout;
	typedef struct R_Pipeline     R_Pipeline;

	typedef enum
	{
		R_OK = 0,
		R_ERROR_DEVICE_CREATION_FAILED,
		R_ERROR_SWAP_CHAIN_FAILED,
		R_ERROR_BUFFER_CREATION_FAILED,
		R_ERROR_SHADER_COMPILATION_FAILED,
		R_ERROR_SHADER_CREATION_FAILED,
		R_ERROR_INPUT_LAYOUT_FAILED,
		R_ERROR_INVALID_PARAMETER,
		R_ERROR_OUT_OF_MEMORY,
	} R_Result;

	const char *r_result_to_string( R_Result result );

	R_Context *r_create_context( HWND hwnd, int width, int height, bool vsync, R_Result *outResult );
	void       r_destroy_context( R_Context *ctx );
	void       r_present( R_Context *ctx );
	void       r_clear_render_target( R_Context *ctx, float r, float g, float b, float a );
	void       r_set_viewport( R_Context *ctx, float x, float y, float w, float h );

	R_Buffer *r_create_buffer( R_Context  *ctx,
	                           const void *data,
	                           size_t      bytes,
	                           bool        dynamic,
	                           UINT        bindFlags,
	                           R_Result   *outResult );
	R_Buffer *r_create_constant_buffer( R_Context *ctx, size_t size, R_Result *outResult );
	void      r_update_buffer( R_Context *ctx, R_Buffer *buf, const void *data, size_t bytes );
	void      r_bind_constant_buffer( R_Context *ctx, R_Buffer *cb, int slot );
	void      r_destroy_buffer( R_Buffer *buf );

	R_VertexShader *r_create_vertex_shader_from_bytecode( R_Context  *ctx,
	                                                      const void *bytecode,
	                                                      size_t      bytecodeSize,
	                                                      R_Result   *outResult );
	R_PixelShader  *r_create_pixel_shader_from_bytecode( R_Context  *ctx,
	                                                     const void *bytecode,
	                                                     size_t      bytecodeSize,
	                                                     R_Result   *outResult );
	R_VertexShader *r_create_vertex_shader_from_source( R_Context  *ctx,
	                                                    const char *src,
	                                                    const char *entry,
	                                                    const char *profile,
	                                                    R_Result   *outResult );
	R_PixelShader  *r_create_pixel_shader_from_source( R_Context  *ctx,
	                                                   const char *src,
	                                                   const char *entry,
	                                                   const char *profile,
	                                                   R_Result   *outResult );

	const void *r_vertex_shader_get_bytecode( const R_VertexShader *shader, size_t *outSize );
	void        r_destroy_vertex_shader( R_VertexShader *sh );
	void        r_destroy_pixel_shader( R_PixelShader *sh );

	R_InputLayout *r_create_input_layout( R_Context                      *ctx,
	                                      const D3D11_INPUT_ELEMENT_DESC *desc,
	                                      UINT                            numDesc,
	                                      const R_VertexShader           *vs,
	                                      R_Result                       *outResult );
	void           r_destroy_input_layout( R_InputLayout *layout );

	R_Pipeline *r_create_pipeline( R_Context      *ctx,
	                               R_VertexShader *vs,
	                               R_PixelShader  *ps,
	                               R_InputLayout  *layout,
	                               R_Result       *outResult );
	void        r_bind_pipeline( R_Context *ctx, R_Pipeline *pipe );
	void        r_destroy_pipeline( R_Pipeline *pipe );

	void r_set_vertex_buffer( R_Context *ctx, R_Buffer *vb, UINT stride, UINT offset );
	void r_set_index_buffer( R_Context *ctx, R_Buffer *ib, DXGI_FORMAT fmt, UINT offset );
	void r_set_primitive_topology( R_Context *ctx, D3D11_PRIMITIVE_TOPOLOGY prim );
	void r_draw( R_Context *ctx, UINT vertexCount, UINT startVertex );
	void r_draw_indexed( R_Context *ctx, UINT indexCount, UINT startIndex, INT baseVertex );

	ID3D11Device        *r_get_device( R_Context *ctx );
	ID3D11DeviceContext *r_get_imm_context( R_Context *ctx );

#ifdef __cplusplus
}
#endif
#endif // R_API_H
