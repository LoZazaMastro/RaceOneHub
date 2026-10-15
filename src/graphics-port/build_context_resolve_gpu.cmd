@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT /c fm5_performance_context.c /Fo..\..\analysis\graphics-port\fm5_performance_context_gpu.obj
if errorlevel 1 exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /MT /EHsc test_context_resolve_gpu.cpp ..\..\analysis\graphics-port\fm5_performance_context_gpu.obj /Fo..\..\analysis\graphics-port\ /Fe..\..\analysis\graphics-port\test_context_resolve_gpu.exe /link d3d11.lib dxguid.lib /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_context_resolve_gpu.exe"
if not "%errorlevel%"=="0" exit /b %errorlevel%
