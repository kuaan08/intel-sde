@echo off
setlocal EnableExtensions
cd /d "%~dp0"
if not exist "bin\SDE AVX Lab Native.exe" (
  call Build.bat
  if errorlevel 1 exit /b %errorlevel%
)
start "SDE AVX Lab Native" "bin\SDE AVX Lab Native.exe"
