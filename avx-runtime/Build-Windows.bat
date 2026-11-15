@echo off
setlocal EnableExtensions
cd /d "%~dp0"
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake is required.
  exit /b 2
)
cmake -S . -B build-windows -A x64
if errorlevel 1 exit /b 1
cmake --build build-windows --config Release
if errorlevel 1 exit /b 1
build-windows\Release\avx_runtime.exe --self-test
