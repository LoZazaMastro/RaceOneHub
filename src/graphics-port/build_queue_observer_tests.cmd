@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
ml64 /nologo /c /Foanalysis\graphics-port\queue_hook_abi_probe.obj src\graphics-port\queue_hook_abi_probe.asm
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /EHsc /DFM5_QUEUE_DIAGNOSTICS_TEST /Fe:analysis\graphics-port\test_queue_observer.exe /Fo:analysis\graphics-port\ src\graphics-port\test_queue_observer.cpp src\graphics-port\fm5_queue_diagnostics.cpp src\graphics-port\fm5_d3d12_diagnostics.cpp analysis\graphics-port\queue_hook_abi_probe.obj /link kernel32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
analysis\graphics-port\test_queue_observer.exe disabled 2>analysis\graphics-port\queue-observer-disabled.jsonl
if not "%errorlevel%"=="0" exit /b %errorlevel%
analysis\graphics-port\test_queue_observer.exe enabled 2>analysis\graphics-port\queue-observer-enabled.jsonl
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /EHsc /c /Fo:analysis\graphics-port\fm5_queue_diagnostics.production.obj src\graphics-port\fm5_queue_diagnostics.cpp
if not "%errorlevel%"=="0" exit /b %errorlevel%
