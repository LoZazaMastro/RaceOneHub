@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT test_context_resolve.c fm5_performance_context.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_context_resolve.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_context_resolve.exe"
if not "%errorlevel%"=="0" exit /b %errorlevel%
