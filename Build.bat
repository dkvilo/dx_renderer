@echo off
setlocal enabledelayedexpansion

set CXX=cl
set VTXC=out\vtxgen.exe

set VTX_SOURCE_DIR=code\render\vtx
set VTX_DEST_DIR=code\render\generated
set MODULES=geometry_3d_pass.vtx ui_pass.vtx

for %%m in (%MODULES%) do (
  %VTXC% "%VTX_SOURCE_DIR%\%%m" "%VTX_DEST_DIR%\%%m"
)

%CXX% code\main.c /link /out:out/out.exe

fxc.exe /T vs_4_0 /E main /Fo vertex.cso code\vertex.hlsl
fxc.exe /T ps_4_0 /E main /Fo pixel.cso code\pixel.hlsl
