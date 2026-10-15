@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
if not "%errorlevel%"=="0" exit /b %errorlevel%
cl /nologo /W4 /WX /O2 /EHsc /DFM5_QUEUE_DIAGNOSTICS_TEST /Fe:analysis\graphics-port\test_command_list_sync.exe /Fo:analysis\graphics-port\ src\graphics-port\test_command_list_sync.cpp src\graphics-port\fm5_d3d12_diagnostics.cpp src\graphics-port\fm5_queue_diagnostics.cpp /link dxgi.lib kernel32.lib
if not "%errorlevel%"=="0" exit /b %errorlevel%
analysis\graphics-port\test_command_list_sync.exe duplicate 2>analysis\graphics-port\command-list-sync-duplicate.jsonl
if not "%errorlevel%"=="0" exit /b %errorlevel%
analysis\graphics-port\test_command_list_sync.exe reuse 2>analysis\graphics-port\command-list-sync-reuse.jsonl
if not "%errorlevel%"=="0" exit /b %errorlevel%
