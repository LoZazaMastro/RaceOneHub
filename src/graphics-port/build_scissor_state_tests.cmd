@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%~dp0"
cl /nologo /W4 /WX /O2 /MT /DFM5_SCISSOR_STATE_TEST test_scissor_state.c fm5_scissor_state.c /Fo..\..\analysis\graphics-port\ /Fe..\..\analysis\graphics-port\test_scissor_state.exe /link /Brepro
if errorlevel 1 exit /b %errorlevel%
"..\..\analysis\graphics-port\test_scissor_state.exe"
if not "%errorlevel%"=="0" exit /b %errorlevel%
