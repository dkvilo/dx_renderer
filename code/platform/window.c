#include "window.h"

PlatformResult P_CreateWindow( PWindowDescriptor *desc )
{
	WNDCLASS wndClass      = { 0 };
	wndClass.lpfnWndProc   = desc->lpfnWndProc;
	wndClass.hInstance     = desc->hInst;
	wndClass.lpszClassName = desc->lpszClassName;
	RegisterClass( &wndClass );

	HWND hwnd = CreateWindow( wndClass.lpszClassName,
	                          desc->Title,
	                          WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                          CW_USEDEFAULT,
	                          CW_USEDEFAULT,
	                          desc->Width,
	                          desc->Height,
	                          0,
	                          0,
	                          desc->hInst,
	                          0 );
	if ( !hwnd )
	{
		return MAKE_ERR( PlatformResult, -1, "Unable to cerate window" );
	}

	return MAKE_OK( PlatformResult, hwnd );
}
