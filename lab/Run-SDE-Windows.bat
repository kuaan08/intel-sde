@echo off
setlocal EnableExtensions

rem SDE AVX Lab - launcher for the official Windows 9.58 kit.
rem This script does not modify the Intel SDE distribution.
set "SCRIPT_DIR=%~dp0"
set "KIT_DIR=%SCRIPT_DIR%..\windows-9.58"
set "SDE=%KIT_DIR%\sde.exe"

if not exist "%SDE%" (
  echo ERROR: Windows SDE 9.58 was not found at:
  echo        "%SDE%"
  exit /b 2
)

if "%~1"=="" (
  echo Usage:
  echo   %~nx0 "C:\path\to\application.exe" [-snb ^| -ivb ^| -hsw ...]
  echo.
  echo Example:
  echo   %~nx0 "C:\path\to\xenia_canary.exe" -snb
  exit /b 64
)

set "APP=%~1"
shift
if not exist "%APP%" (
  echo ERROR: application was not found: "%APP%"
  exit /b 3
)

"%SDE%" -version
if errorlevel 1 exit /b %errorlevel%

rem The profile is supplied by the caller; -snb is the project's first reference.
"%SDE%" %* -- "%APP%"
exit /b %errorlevel%
