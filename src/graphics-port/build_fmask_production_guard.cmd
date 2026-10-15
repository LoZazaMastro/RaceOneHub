@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT test_fmask_production_guard.c fm5_fmask_bridge.c ..\allocation-port\fm5_msaa4.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_fmask_production_guard.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_fmask_production_guard.exe"
