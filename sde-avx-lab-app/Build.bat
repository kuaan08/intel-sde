@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if not exist "%CSC%" set "CSC=%WINDIR%\Microsoft.NET\Framework\v4.0.30319\csc.exe"
if not exist "%CSC%" (
  echo ERROR: .NET Framework C# compiler not found.
  echo Install .NET Framework 4.x Developer Pack or build with Visual Studio.
  exit /b 2
)
if not exist bin mkdir bin
set "REFS=/reference:System.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll"
"%CSC%" /nologo /target:winexe /platform:x64 /optimize+ /win32manifest:app.manifest /out:"bin\SDE AVX Lab Native.exe" %REFS% SDEAvxLabLauncher.cs
if errorlevel 1 goto fail
"%CSC%" /nologo /target:winexe /platform:x64 /optimize+ /define:INTEL_SDE_ONLY /win32manifest:app.manifest /out:"bin\Intel SDE Launcher.exe" %REFS% SDEAvxLabLauncher.cs
if errorlevel 1 goto fail
echo Build complete:
echo   "%CD%\bin\SDE AVX Lab Native.exe"
echo   "%CD%\bin\Intel SDE Launcher.exe"
exit /b 0
:fail
echo Build failed.
exit /b 1
