#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dxguid.lib" )

typedef struct
{
	float x, y, z;
	float r, g, b, a;
} Vertex;

HWND                    g_hWnd              = NULL;
ID3D11Device           *g_pd3dDevice        = NULL;
ID3D11DeviceContext    *g_pImmediateContext = NULL;
IDXGISwapChain         *g_pSwapChain        = NULL;
ID3D11RenderTargetView *g_pRenderTargetView = NULL;
ID3D11VertexShader     *g_pVertexShader     = NULL;
ID3D11PixelShader      *g_pPixelShader      = NULL;
ID3D11InputLayout      *g_pVertexLayout     = NULL;
ID3D11Buffer           *g_pVertexBuffer     = NULL;

const int windowWidth  = 800;
const int windowHeight = 600;

const char *vertexShaderSource = "struct VS_INPUT\n"
                                 "{\n"
                                 "    float4 Pos : POSITION;\n"
                                 "    float4 Color : COLOR;\n"
                                 "};\n"
                                 "\n"
                                 "struct PS_INPUT\n"
                                 "{\n"
                                 "    float4 Pos : SV_POSITION;\n"
                                 "    float4 Color : COLOR;\n"
                                 "};\n"
                                 "\n"
                                 "PS_INPUT VS(VS_INPUT input)\n"
                                 "{\n"
                                 "    PS_INPUT output = (PS_INPUT)0;\n"
                                 "    output.Pos = input.Pos;\n"
                                 "    output.Color = input.Color;\n"
                                 "    return output;\n"
                                 "}\n";

const char *pixelShaderSource = "struct PS_INPUT\n"
                                "{\n"
                                "    float4 Pos : SV_POSITION;\n"
                                "    float4 Color : COLOR;\n"
                                "};\n"
                                "\n"
                                "float4 PS(PS_INPUT input) : SV_Target\n"
                                "{\n"
                                "    return input.Color;\n"
                                "}\n";

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_DESTROY:
		PostQuitMessage( 0 );
		return 0;
	}
	return DefWindowProc( hWnd, msg, wParam, lParam );
}

HRESULT InitD3D()
{
	HRESULT hr = S_OK;

	DXGI_SWAP_CHAIN_DESC sd               = { 0 };
	sd.BufferCount                        = 1;
	sd.BufferDesc.Width                   = windowWidth;
	sd.BufferDesc.Height                  = windowHeight;
	sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
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
	vp.Width          = (float)windowWidth;
	vp.Height         = (float)windowHeight;
	vp.MinDepth       = 0.0f;
	vp.MaxDepth       = 1.0f;
	vp.TopLeftX       = 0;
	vp.TopLeftY       = 0;
	g_pImmediateContext->lpVtbl->RSSetViewports( g_pImmediateContext, 1, &vp );

	return S_OK;
}

HRESULT InitGeometry()
{
	HRESULT   hr         = S_OK;
	ID3DBlob *pVSBlob    = NULL;
	ID3DBlob *pErrorBlob = NULL;
	hr                   = D3DCompile( vertexShaderSource,
                     strlen( vertexShaderSource ),
                     "VS",
                     NULL,
                     NULL,
                     "VS",
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
	    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	hr = g_pd3dDevice->lpVtbl->CreateInputLayout( g_pd3dDevice,
	                                              layout,
	                                              2,
	                                              pVSBlob->lpVtbl->GetBufferPointer( pVSBlob ),
	                                              pVSBlob->lpVtbl->GetBufferSize( pVSBlob ),
	                                              &g_pVertexLayout );
	pVSBlob->lpVtbl->Release( pVSBlob );
	if ( FAILED( hr ) )
		return hr;

	g_pImmediateContext->lpVtbl->IASetInputLayout( g_pImmediateContext, g_pVertexLayout );

	ID3DBlob *pPSBlob = NULL;
	hr                = D3DCompile( pixelShaderSource,
                     strlen( pixelShaderSource ),
                     "PS",
                     NULL,
                     NULL,
                     "PS",
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

	Vertex vertices[] = {
	    { 0.0f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },  // T
	    { 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f }, // BR
	    { -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f } // BL
	};

	D3D11_BUFFER_DESC bd = { 0 };
	bd.Usage             = D3D11_USAGE_DEFAULT;
	bd.ByteWidth         = sizeof( vertices );
	bd.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags    = 0;

	D3D11_SUBRESOURCE_DATA InitData = { 0 };
	InitData.pSysMem                = vertices;

	hr = g_pd3dDevice->lpVtbl->CreateBuffer( g_pd3dDevice, &bd, &InitData, &g_pVertexBuffer );
	if ( FAILED( hr ) )
		return hr;

	UINT stride = sizeof( Vertex );
	UINT offset = 0;
	g_pImmediateContext->lpVtbl->IASetVertexBuffers( g_pImmediateContext, 0, 1, &g_pVertexBuffer, &stride, &offset );
	g_pImmediateContext->lpVtbl->IASetPrimitiveTopology( g_pImmediateContext, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return S_OK;
}

void Render()
{
	float ClearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
	g_pImmediateContext->lpVtbl->ClearRenderTargetView( g_pImmediateContext, g_pRenderTargetView, ClearColor );

	g_pImmediateContext->lpVtbl->VSSetShader( g_pImmediateContext, g_pVertexShader, NULL, 0 );
	g_pImmediateContext->lpVtbl->PSSetShader( g_pImmediateContext, g_pPixelShader, NULL, 0 );

	g_pImmediateContext->lpVtbl->Draw( g_pImmediateContext, 3, 0 );
	g_pSwapChain->lpVtbl->Present( g_pSwapChain, 1, 0 );
}

void CleanupDevice()
{
	if ( g_pImmediateContext )
		g_pImmediateContext->lpVtbl->ClearState( g_pImmediateContext );
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

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
{
	WNDCLASSEX wc = { sizeof( WNDCLASSEX ),
	                  CS_CLASSDC,
	                  WndProc,
	                  0L,
	                  0L,
	                  GetModuleHandle( NULL ),
	                  NULL,
	                  NULL,
	                  NULL,
	                  NULL,
	                  "D3D11Triangle",
	                  NULL };
	RegisterClassEx( &wc );

	g_hWnd = CreateWindow( "D3D11Triangle",
	                       "D3D11 Triangle",
	                       WS_OVERLAPPEDWINDOW,
	                       100,
	                       100,
	                       800,
	                       800,
	                       NULL,
	                       NULL,
	                       wc.hInstance,
	                       NULL );

	if ( SUCCEEDED( InitD3D() ) )
	{
		if ( SUCCEEDED( InitGeometry() ) )
		{
			ShowWindow( g_hWnd, nCmdShow );

			MSG msg = { 0 };
			while ( WM_QUIT != msg.message )
			{
				if ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
				{
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
				else
				{
					Render();
				}
			}
		}
	}

	CleanupDevice();
	UnregisterClass( L"D3D11Triangle", wc.hInstance );
	return 0;
}
