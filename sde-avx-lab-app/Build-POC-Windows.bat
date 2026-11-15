@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "POC=%~dp0..\avx1-sse-poc"
if not exist "%POC%\CMakeLists.txt" (
  echo ERROR: avx1-sse-poc was not found at "%POC%"
  exit /b 2
)
where cmake >nul 2>nul
if errorlevel 1 (
  echo ERROR: CMake is required. Install Visual Studio with C++ or CMake.
  exit /b 3
)
cmake -S "%POC%" -B "%POC%\build\windows" -A x64
if errorlevel 1 exit /b 1
cmake --build "%POC%\build\windows" --config Release
if errorlevel 1 exit /b 1
copy /Y "%POC%\build\windows\Release\avx1_sse_poc.exe" "%~dp0avx1_sse_poc.exe" >nul
if errorlevel 1 exit /b 1
echo POC copied to "%~dp0avx1_sse_poc.exe"
