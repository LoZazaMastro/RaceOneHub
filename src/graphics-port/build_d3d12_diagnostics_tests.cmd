@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT test_d3d12_diagnostics.cpp fm5_d3d12_diagnostics.cpp /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_d3d12_diagnostics.exe" /link /Brepro dxgi.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
set FM5_D3D12_DEBUG=0
"..\..\analysis\graphics-port\test_d3d12_diagnostics.exe" >"..\..\analysis\graphics-port\d3d12-diagnostics-disabled.stdout" 2>"..\..\analysis\graphics-port\d3d12-diagnostics-disabled.jsonl"
if not "%errorlevel%"=="0" exit /b %errorlevel%
set FM5_D3D12_DEBUG=1
"..\..\analysis\graphics-port\test_d3d12_diagnostics.exe" >"..\..\analysis\graphics-port\d3d12-diagnostics-enabled.stdout" 2>"..\..\analysis\graphics-port\d3d12-diagnostics-enabled.jsonl"
if not "%errorlevel%"=="0" exit /b %errorlevel%
type "..\..\analysis\graphics-port\d3d12-diagnostics-disabled.stdout"
type "..\..\analysis\graphics-port\d3d12-diagnostics-enabled.stdout"
