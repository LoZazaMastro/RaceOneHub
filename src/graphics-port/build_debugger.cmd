@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /O2 /MT fm5_debugger.c /Fo"..\..\analysis\graphics-port\fm5_debugger.obj" /Fe"..\..\analysis\graphics-port\fm5_debugger.exe" /link /Brepro user32.lib
if errorlevel 1 exit /b %errorlevel%
cl /nologo /W4 /O2 /MT debug_fixture.c /Fo"..\..\analysis\graphics-port\debug_fixture.obj" /Fe"..\..\analysis\graphics-port\debug_fixture.exe" /link /Brepro
exit /b %errorlevel%
