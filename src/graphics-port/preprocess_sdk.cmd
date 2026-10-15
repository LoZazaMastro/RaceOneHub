@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /TC /P /Fianalysis\graphics-port\sdk_types.i src\graphics-port\sdk_types.c
