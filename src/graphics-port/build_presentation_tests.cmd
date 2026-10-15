@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT /DFM5_PRESENTATION_TEST test_presentation.c fm5_presentation.c /Fo..\..\analysis\graphics-port\ /Fe"..\..\analysis\graphics-port\test_presentation.exe" /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_presentation.exe"
if errorlevel 1 exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /MT /c fm5_presentation.c /Fo..\..\analysis\graphics-port\fm5_presentation.production.obj
