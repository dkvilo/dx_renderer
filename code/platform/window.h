#ifndef __P_WINDOW_H__
#define __P_WINDOW_H__

#include <windows.h>
#include "../base/result.h"

typedef struct PWindowDescriptor
{
	HINSTANCE   hInst;
	WNDPROC     lpfnWndProc;
	const char *lpszClassName;
	const char *Title;
	uint32_t    Width;
	uint32_t    Height;
} PWindowDescriptor;

DEFINE_RESULT_TYPE( PlatformResult, HWND );

PlatformResult P_CreateWindow( PWindowDescriptor *desc );

#endif // __P_WINDOW_H__
