@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT /DFM5_DXGI_TEST test_dxgi_bridge.c fm5_dxgi_bridge.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_dxgi_bridge.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_dxgi_bridge.exe"
