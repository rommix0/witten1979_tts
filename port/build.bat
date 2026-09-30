@echo off
rem Build ISP with MSVC (run from any prompt; sets up the x64 toolchain itself)
setlocal
set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" set VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat
call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"
cl /nologo /O2 /w /TC isp.c /Fe:isp.exe || exit /b 1
del isp.obj 2>nul
