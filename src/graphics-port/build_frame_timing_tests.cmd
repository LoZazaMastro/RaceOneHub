@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /DFM5_FRAME_TIMING_TEST /Fe:analysis\graphics-port\test_frame_timing.exe /Fo:analysis\graphics-port\ src\graphics-port\fm5_frame_timing.c src\graphics-port\test_frame_timing.c /link kernel32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
analysis\graphics-port\test_frame_timing.exe
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /c /Fo:analysis\graphics-port\fm5_frame_timing.production.obj src\graphics-port\fm5_frame_timing.c
if not "%errorlevel%"=="0" exit /b %errorlevel%
