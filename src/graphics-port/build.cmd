@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /O2 /GS- /Zl /c fm5_graphics_bridge.c /Fo"..\..\analysis\graphics-port\fm5_graphics_bridge.obj"
if errorlevel 1 exit /b %errorlevel%
link /nologo /dll /noentry /nodefaultlib /machine:x64 /dynamicbase /nxcompat /Brepro "..\..\analysis\graphics-port\fm5_graphics_bridge.obj" /out:"..\..\analysis\graphics-port\fm5_graphics_bridge.dll" /implib:"..\..\analysis\graphics-port\fm5_graphics_bridge.lib"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /W4 /O2 /MT /DFM5_BRIDGE_TEST test_bridge.c fm5_graphics_bridge.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_bridge.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_bridge.exe"
exit /b %errorlevel%
