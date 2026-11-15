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
"%CSC%" /nologo /target:winexe /platform:x64 /optimize+ /win32manifest:app.manifest /out:"bin\SDE AVX Lab.exe" /reference:System.dll /reference:System.Drawing.dll /reference:System.Windows.Forms.dll SDEAvxLabLauncher.cs
if errorlevel 1 (
  echo Build failed.
  exit /b 1
)
echo Build complete: "%CD%\bin\SDE AVX Lab.exe"
