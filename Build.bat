@echo off

setlocal enabledelayedexpansion

@REM set MODULES=Math Main

set CXX=cl
%LINKER% %LINKFLAGS%

%CXX% /TC code/main.c /link d3d12.lib dxgi.lib d3dcompiler.lib dxguid.lib uuid.lib kernel32.lib user32.lib gdi32.lib /out:Out.exe
