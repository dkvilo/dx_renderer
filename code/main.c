#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <string.h>
#include <math.h>

#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

#include "base/hm.c"
#include "base/c_string.c"
#include "base/c_string_builder.c"

#define HANDMADE_MATH_NO_SSE
#include "extern/hmmath.h"

#define SHADER_CODE( ... ) #__VA_ARGS__
#define FRAME_COUNT 2

typedef struct
{
	HMM_Mat4 worldMatrix;
	HMM_Vec4 color;
} InstanceData;

typedef struct
{
	float position[3];
} Vertex;

HWND                       hwnd;
ID3D12Device              *device           = NULL;
ID3D12CommandQueue        *commandQueue     = NULL;
IDXGISwapChain3           *swapChain        = NULL;
ID3D12DescriptorHeap      *rtvHeap          = NULL;
ID3D12CommandAllocator    *commandAllocator = NULL;
ID3D12GraphicsCommandList *commandList      = NULL;
ID3D12RootSignature       *rootSignature    = NULL;
ID3D12PipelineState       *pipelineState    = NULL;
D3D12_VERTEX_BUFFER_VIEW   vertexBufferView = { 0 };
ID3D12Fence               *fence            = NULL;
HANDLE                     fenceEvent;
UINT64                     fenceValue                 = 0;
UINT                       frameIndex                 = 0;
UINT                       rtvDescriptorSize          = 0;
ID3D12Resource            *renderTargets[FRAME_COUNT] = { NULL };
ID3D12Resource            *vertexBuffer               = NULL;
ID3D12Resource            *instanceBuffer;
InstanceData              *mappedInstancePtr = NULL;
D3D12_VERTEX_BUFFER_VIEW   instanceBufferView;

#define MAX_INSTANCES 100000
InstanceData instances[MAX_INSTANCES];

Vertex unitGeometryVertices[] = {
    { .position = { -0.5f, 0.5f, 0.0f } },  // TL
    { .position = { 0.5f, 0.5f, 0.0f } },   // TR
    { .position = { -0.5f, -0.5f, 0.0f } }, // BL
    { .position = { -0.5f, -0.5f, 0.0f } }, // BL
    { .position = { 0.5f, 0.5f, 0.0f } },   // TR
    { .position = { 0.5f, -0.5f, 0.0f } },  // BR
};

const char *vertexShaderSource = SHADER_CODE(
    struct VSInput {
	    float3             position : POSITION;
	    row_major float4x4 world : WORLD;
	    float4             instanceColor : INSTANCECOLOR;
    };

    struct PSInput {
	    float4 position : SV_POSITION;
	    float4 color : COLOR;
    };

    PSInput VSMain( VSInput input ) {
	    PSInput result;
	    result.position = mul( float4( input.position, 1.0f ), input.world );
	    result.color    = input.instanceColor;
	    return result;
    }

);

const char *pixelShaderSource = SHADER_CODE(
    struct PSInput {
	    float4 position : SV_POSITION;
	    float4 color : COLOR;
    };

    float4 PSMain( PSInput input ) : SV_TARGET { return input.color; }

);

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}
	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

void WaitForPreviousFrame()
{
	const UINT64 currentFenceValue = fenceValue;
	commandQueue->lpVtbl->Signal( commandQueue, fence, currentFenceValue );
	fenceValue++;

	if ( fence->lpVtbl->GetCompletedValue( fence ) < currentFenceValue )
	{
		fence->lpVtbl->SetEventOnCompletion( fence, currentFenceValue, fenceEvent );
		WaitForSingleObject( fenceEvent, INFINITE );
	}

	frameIndex = swapChain->lpVtbl->GetCurrentBackBufferIndex( swapChain );
}

HRESULT InitD3D12()
{
	HRESULT hr;

	// device
	hr = D3D12CreateDevice( NULL, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// command queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = { 0 };
	queueDesc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = device->lpVtbl->CreateCommandQueue( device, &queueDesc, &IID_ID3D12CommandQueue, (void **)&commandQueue );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// swap chain
	IDXGIFactory4 *factory;
	hr = CreateDXGIFactory1( &IID_IDXGIFactory4, (void **)&factory );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.BufferCount           = FRAME_COUNT;
	swapChainDesc.Width                 = 800;
	swapChainDesc.Height                = 800;
	swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count      = 1;

	IDXGISwapChain1 *swapChain1;
	hr = factory->lpVtbl->CreateSwapChainForHwnd( factory,
	                                              (IUnknown *)commandQueue,
	                                              hwnd,
	                                              &swapChainDesc,
	                                              NULL,
	                                              NULL,
	                                              &swapChain1 );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	hr = swapChain1->lpVtbl->QueryInterface( swapChain1, &IID_IDXGISwapChain3, (void **)&swapChain );
	swapChain1->lpVtbl->Release( swapChain1 );
	factory->lpVtbl->Release( factory );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	frameIndex = swapChain->lpVtbl->GetCurrentBackBufferIndex( swapChain );

	// descriptor heap for render target views
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { 0 };
	rtvHeapDesc.NumDescriptors             = FRAME_COUNT;
	rtvHeapDesc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->lpVtbl->CreateDescriptorHeap( device, &rtvHeapDesc, &IID_ID3D12DescriptorHeap, (void **)&rtvHeap );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	rtvDescriptorSize = device->lpVtbl->GetDescriptorHandleIncrementSize( device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV );

	// render target views
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	rtvHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart( rtvHeap, &rtvHandle );

	for ( UINT n = 0; n < FRAME_COUNT; n++ )
	{
		hr = swapChain->lpVtbl->GetBuffer( swapChain, n, &IID_ID3D12Resource, (void **)&renderTargets[n] );
		if ( FAILED( hr ) )
			return hr;
		device->lpVtbl->CreateRenderTargetView( device, renderTargets[n], NULL, rtvHandle );
		rtvHandle.ptr += rtvDescriptorSize;
	}

	// command allocator
	hr = device->lpVtbl->CreateCommandAllocator( device,
	                                             D3D12_COMMAND_LIST_TYPE_DIRECT,
	                                             &IID_ID3D12CommandAllocator,
	                                             (void **)&commandAllocator );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// root signature
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = { 0 };
	rootSignatureDesc.NumParameters             = 0;
	rootSignatureDesc.pParameters               = NULL;
	rootSignatureDesc.NumStaticSamplers         = 0;
	rootSignatureDesc.pStaticSamplers           = NULL;
	rootSignatureDesc.Flags                     = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob *signature;
	ID3DBlob *error;
	hr = D3D12SerializeRootSignature( &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error );
	if ( FAILED( hr ) )
		return hr;

	hr = device->lpVtbl->CreateRootSignature( device,
	                                          0,
	                                          signature->lpVtbl->GetBufferPointer( signature ),
	                                          signature->lpVtbl->GetBufferSize( signature ),
	                                          &IID_ID3D12RootSignature,
	                                          (void **)&rootSignature );
	signature->lpVtbl->Release( signature );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// shader  compilation
	ID3DBlob *vertexShader;
	ID3DBlob *pixelShader;

	hr = D3DCompile( vertexShaderSource,
	                 strlen( vertexShaderSource ),
	                 NULL,
	                 NULL,
	                 NULL,
	                 "VSMain",
	                 "vs_5_0",
	                 0,
	                 0,
	                 &vertexShader,
	                 &error );
	if ( FAILED( hr ) )
	{
		if ( error )
		{
			OutputDebugStringA( (char *)error->lpVtbl->GetBufferPointer( error ) );
			error->lpVtbl->Release( error );
		}
		return hr;
	}

	hr = D3DCompile( pixelShaderSource,
	                 strlen( pixelShaderSource ),
	                 NULL,
	                 NULL,
	                 NULL,
	                 "PSMain",
	                 "ps_5_0",
	                 0,
	                 0,
	                 &pixelShader,
	                 &error );
	if ( FAILED( hr ) )
	{
		if ( error )
		{
			OutputDebugStringA( (char *)error->lpVtbl->GetBufferPointer( error ) );
			error->lpVtbl->Release( error );
		}
		return hr;
	}

	// input layout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
	    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	    { "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	    { "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	    { "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	    { "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	    { "INSTANCECOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
	};

	// pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc               = { 0 };
	psoDesc.InputLayout.pInputElementDescs                   = inputElementDescs;
	psoDesc.InputLayout.NumElements                          = _countof( inputElementDescs );
	psoDesc.pRootSignature                                   = rootSignature;
	psoDesc.VS.pShaderBytecode                               = vertexShader->lpVtbl->GetBufferPointer( vertexShader );
	psoDesc.VS.BytecodeLength                                = vertexShader->lpVtbl->GetBufferSize( vertexShader );
	psoDesc.PS.pShaderBytecode                               = pixelShader->lpVtbl->GetBufferPointer( pixelShader );
	psoDesc.PS.BytecodeLength                                = pixelShader->lpVtbl->GetBufferSize( pixelShader );
	psoDesc.RasterizerState.FillMode                         = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode                         = D3D12_CULL_MODE_BACK;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.DepthStencilState.DepthEnable                    = FALSE;
	psoDesc.DepthStencilState.StencilEnable                  = FALSE;
	psoDesc.SampleMask                                       = UINT_MAX;
	psoDesc.PrimitiveTopologyType                            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets                                 = 1;
	psoDesc.RTVFormats[0]                                    = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count                                 = 1;

	psoDesc.BlendState.AlphaToCoverageEnable                 = FALSE;
	psoDesc.BlendState.IndependentBlendEnable                = FALSE;
	psoDesc.BlendState.RenderTarget[0].BlendEnable           = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	hr = device->lpVtbl->CreateGraphicsPipelineState( device,
	                                                  &psoDesc,
	                                                  &IID_ID3D12PipelineState,
	                                                  (void **)&pipelineState );
	vertexShader->lpVtbl->Release( vertexShader );
	pixelShader->lpVtbl->Release( pixelShader );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// command list
	hr = device->lpVtbl->CreateCommandList( device,
	                                        0,
	                                        D3D12_COMMAND_LIST_TYPE_DIRECT,
	                                        commandAllocator,
	                                        pipelineState,
	                                        &IID_ID3D12GraphicsCommandList,
	                                        (void **)&commandList );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	commandList->lpVtbl->Close( commandList );

	D3D12_HEAP_PROPERTIES heapProps = { 0 };
	heapProps.Type                  = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC vertexBufferDesc = { 0 };
	vertexBufferDesc.Dimension           = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexBufferDesc.Width               = sizeof( unitGeometryVertices );
	vertexBufferDesc.Height              = 1;
	vertexBufferDesc.DepthOrArraySize    = 1;
	vertexBufferDesc.MipLevels           = 1;
	vertexBufferDesc.Format              = DXGI_FORMAT_UNKNOWN;
	vertexBufferDesc.SampleDesc.Count    = 1;
	vertexBufferDesc.Layout              = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = device->lpVtbl->CreateCommittedResource( device,
	                                              &heapProps,
	                                              D3D12_HEAP_FLAG_NONE,
	                                              &vertexBufferDesc,
	                                              D3D12_RESOURCE_STATE_GENERIC_READ,
	                                              NULL,
	                                              &IID_ID3D12Resource,
	                                              (void **)&vertexBuffer );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	// copy vertex data to buffer
	UINT8      *pVertexDataBegin;
	D3D12_RANGE readRange = { 0, 0 };
	hr                    = vertexBuffer->lpVtbl->Map( vertexBuffer, 0, &readRange, (void **)&pVertexDataBegin );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	memcpy( pVertexDataBegin, unitGeometryVertices, sizeof( unitGeometryVertices ) );
	vertexBuffer->lpVtbl->Unmap( vertexBuffer, 0, NULL );

	UINT instanceBufferSize = sizeof( instances );
	vertexBufferDesc.Width  = instanceBufferSize;
	hr                      = device->lpVtbl->CreateCommittedResource( device,
                                                  &heapProps,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &vertexBufferDesc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  NULL,
                                                  &IID_ID3D12Resource,
                                                  (void **)&instanceBuffer );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	UINT8 *pInstanceDataBegin;
	hr = instanceBuffer->lpVtbl->Map( instanceBuffer, 0, &readRange, (void **)&pInstanceDataBegin );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	memcpy( pInstanceDataBegin, instances, sizeof( instances ) );
	mappedInstancePtr                 = (InstanceData *)pInstanceDataBegin;
	instanceBufferView.BufferLocation = instanceBuffer->lpVtbl->GetGPUVirtualAddress( instanceBuffer );
	instanceBufferView.StrideInBytes  = sizeof( InstanceData );
	instanceBufferView.SizeInBytes    = sizeof( instances );

	// vertex buffer view
	vertexBufferView.BufferLocation = vertexBuffer->lpVtbl->GetGPUVirtualAddress( vertexBuffer );
	vertexBufferView.StrideInBytes  = sizeof( Vertex );
	vertexBufferView.SizeInBytes    = sizeof( unitGeometryVertices );

	// fence
	hr = device->lpVtbl->CreateFence( device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence );
	if ( FAILED( hr ) )
	{
		return hr;
	}

	fenceValue = 1;
	fenceEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	if ( fenceEvent == NULL )
	{
		return E_FAIL;
	}

	return S_OK;
}

void Render()
{
	// record commands
	commandAllocator->lpVtbl->Reset( commandAllocator );
	commandList->lpVtbl->Reset( commandList, commandAllocator, pipelineState );

	commandList->lpVtbl->SetGraphicsRootSignature( commandList, rootSignature );
	commandList->lpVtbl->RSSetViewports( commandList, 1, &(D3D12_VIEWPORT){ 0.0f, 0.0f, 800.0f, 800.0f, 0.0f, 1.0f } );
	commandList->lpVtbl->RSSetScissorRects( commandList, 1, &(D3D12_RECT){ 0, 0, 800, 800 } );

	// transition render target to render target state
	D3D12_RESOURCE_BARRIER barrier = { 0 };
	barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource   = renderTargets[frameIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->lpVtbl->ResourceBarrier( commandList, 1, &barrier );

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	rtvHeap->lpVtbl->GetCPUDescriptorHandleForHeapStart( rtvHeap, &rtvHandle );
	rtvHandle.ptr += frameIndex * rtvDescriptorSize;
	commandList->lpVtbl->OMSetRenderTargets( commandList, 1, &rtvHandle, FALSE, NULL );

	// clear and draw
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	commandList->lpVtbl->ClearRenderTargetView( commandList, rtvHandle, clearColor, 0, NULL );
	commandList->lpVtbl->IASetPrimitiveTopology( commandList, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	D3D12_VERTEX_BUFFER_VIEW vbViews[] = {
	    vertexBufferView,  // Slot 0: geometry
	    instanceBufferView // Slot 1: instance data
	};

	commandList->lpVtbl->IASetVertexBuffers( commandList, 0, 2, vbViews );

	UINT vertexCountPerInstance = ( sizeof( unitGeometryVertices ) / sizeof( Vertex ) );
	// Only draw the actual number of instances we have
	const UINT GRID_WIDTH          = 12;
	const UINT GRID_HEIGHT         = 12;
	UINT       actualInstanceCount = GRID_WIDTH * GRID_HEIGHT + 1; // +1 for the rotating square
	commandList->lpVtbl->DrawInstanced( commandList, vertexCountPerInstance, actualInstanceCount, 0, 0 );

	// transition render target to present state
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	commandList->lpVtbl->ResourceBarrier( commandList, 1, &barrier );
	commandList->lpVtbl->Close( commandList );

	// execute commands
	ID3D12CommandList *ppCommandLists[] = { (ID3D12CommandList *)commandList };
	commandQueue->lpVtbl->ExecuteCommandLists( commandQueue, 1, ppCommandLists );

	// present
	swapChain->lpVtbl->Present( swapChain, 1, 0 );

	WaitForPreviousFrame();
}

void Cleanup()
{
	WaitForPreviousFrame();

	if ( instanceBuffer && mappedInstancePtr )
	{
		instanceBuffer->lpVtbl->Unmap( instanceBuffer, 0, NULL );
		mappedInstancePtr = NULL;
	}

	CloseHandle( fenceEvent );

	if ( fence )
	{
		fence->lpVtbl->Release( fence );
	}

	if ( instanceBuffer )
	{
		instanceBuffer->lpVtbl->Release( instanceBuffer );
	}

	if ( vertexBuffer )
	{
		vertexBuffer->lpVtbl->Release( vertexBuffer );
	}

	if ( pipelineState )
	{
		pipelineState->lpVtbl->Release( pipelineState );
	}

	if ( rootSignature )
	{
		rootSignature->lpVtbl->Release( rootSignature );
	}

	if ( commandList )
	{
		commandList->lpVtbl->Release( commandList );
	}

	if ( commandAllocator )
	{
		commandAllocator->lpVtbl->Release( commandAllocator );
	}

	for ( UINT n = 0; n < FRAME_COUNT; n++ )
	{
		if ( renderTargets[n] )
		{
			renderTargets[n]->lpVtbl->Release( renderTargets[n] );
		}
	}

	if ( rtvHeap )
	{
		rtvHeap->lpVtbl->Release( rtvHeap );
	}

	if ( swapChain )
	{
		swapChain->lpVtbl->Release( swapChain );
	}

	if ( commandQueue )
	{
		commandQueue->lpVtbl->Release( commandQueue );
	}

	if ( device )
	{
		device->lpVtbl->Release( device );
	}
}

static HMM_Vec4 HSLToRGB( float h, float s, float l )
{
	float r, g, b;

	float c = ( 1.0f - fabsf( 2.0f * l - 1.0f ) ) * s;
	float x = c * ( 1.0f - fabsf( fmodf( h * 6.0f, 2.0f ) - 1.0f ) );
	float m = l - c / 2.0f;

	if ( h < 1.0f / 6.0f )
	{
		r = c;
		g = x;
		b = 0;
	}
	else if ( h < 2.0f / 6.0f )
	{
		r = x;
		g = c;
		b = 0;
	}
	else if ( h < 3.0f / 6.0f )
	{
		r = 0;
		g = c;
		b = x;
	}
	else if ( h < 4.0f / 6.0f )
	{
		r = 0;
		g = x;
		b = c;
	}
	else if ( h < 5.0f / 6.0f )
	{
		r = x;
		g = 0;
		b = c;
	}
	else
	{
		r = c;
		g = 0;
		b = x;
	}

	return (HMM_Vec4){ r + m, g + m, b + m, 1.0f };
}

int32_t WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow )
{
	WNDCLASSEX wc    = { 0 };
	wc.cbSize        = sizeof( WNDCLASSEX );
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszClassName = "D3D12RenderEngine";
	RegisterClassEx( &wc );

	const UINT WindowWidth  = 800;
	const UINT WindowHeight = 800;

	hwnd = CreateWindowEx( 0,
	                       "D3D12RenderEngine",
	                       "Hello D3D12",
	                       WS_OVERLAPPEDWINDOW,
	                       CW_USEDEFAULT,
	                       CW_USEDEFAULT,
	                       WindowWidth,
	                       WindowHeight,
	                       NULL,
	                       NULL,
	                       hInstance,
	                       NULL );

	if ( !hwnd )
	{
		return -1;
	}

	ShowWindow( hwnd, nCmdShow );

	const UINT  GRID_WIDTH  = 12;
	const UINT  GRID_HEIGHT = 12;
	const float SPACING     = 0.15f;
	const float SCALE       = 0.08f;
	memset( instances, 0, sizeof( instances ) );

	for ( int32_t y = 0; y < GRID_HEIGHT; y++ )
	{
		for ( int32_t x = 0; x < GRID_WIDTH; x++ )
		{
			int32_t index = y * GRID_WIDTH + x;
			float   xpos  = x * SPACING - ( GRID_WIDTH / 2.0f ) * SPACING + SPACING / 2.0f;
			float   ypos  = y * SPACING - ( GRID_HEIGHT / 2.0f ) * SPACING + SPACING / 2.0f;

			HMM_Mat4 scale               = HMM_Scale( (HMM_Vec3){ SCALE, SCALE, SCALE } );
			HMM_Mat4 translation         = HMM_Translate( (HMM_Vec3){ xpos, ypos, 0.0f } );
			instances[index].worldMatrix = HMM_MulM4( translation, scale );

			float hue              = (float)rand() / RAND_MAX;
			float saturation       = 0.6f + ( (float)rand() / RAND_MAX ) * 0.4f;
			float lightness        = 0.4f + ( (float)rand() / RAND_MAX ) * 0.4f;
			instances[index].color = HSLToRGB( hue, saturation, lightness );
		}
	}

	if ( FAILED( InitD3D12() ) )
	{
		MessageBox( NULL, "Failed to initialize D3D12", "Error", MB_OK );
		return -1;
	}

	MSG msg = { 0 };

	LARGE_INTEGER frequency;
	LARGE_INTEGER lastTime;
	QueryPerformanceFrequency( &frequency );
	QueryPerformanceCounter( &lastTime );
	float deltaTime = 0.0f;

	while ( msg.message != WM_QUIT )
	{
		if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			LARGE_INTEGER currentTime;
			QueryPerformanceCounter( &currentTime );
			deltaTime = (float)( currentTime.QuadPart - lastTime.QuadPart ) / frequency.QuadPart;
			lastTime  = currentTime;

			static float t = 0.0f;
			t += deltaTime;

			int32_t  index               = GRID_WIDTH * GRID_HEIGHT;
			float    angle               = t;
			HMM_Mat4 scale               = HMM_Scale( (HMM_Vec3){ SCALE * 2.0f, SCALE * 2.0f, SCALE * 2.0f } );
			HMM_Mat4 rotation            = HMM_Rotate_RH( angle, (HMM_Vec3){ 0.0f, 0.0f, 1.0f } );
			HMM_Mat4 translation         = HMM_Translate( (HMM_Vec3){ 0.0f, 0.0f, 0.0f } );
			instances[index].worldMatrix = HMM_MulM4( HMM_MulM4( translation, rotation ), scale );
			instances[index].color       = (HMM_Vec4){ 1.0f, 0.0f, 0.0f, 1.0f };

			// write to GPU mapped upload buffer
			if ( mappedInstancePtr )
			{
				mappedInstancePtr[index] = instances[index];
			}

			Render();
		}
	}

	Cleanup();
	return 0;
}
