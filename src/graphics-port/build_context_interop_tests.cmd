@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /wd4201 /O2 /MT test_context_interop.c fm5_performance_context.c ..\audio-tls\performance-device\fm5_performance_device.c ..\..\analysis\audio-tls\performance-device\device_forwarders.obj ..\allocation-port\fm5_msaa4.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_context_interop.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_context_interop.exe"
