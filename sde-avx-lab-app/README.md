# SDE AVX Lab — Game Compatibility Launcher

The launcher is an original Windows application for the SDE AVX Lab independent runtime. It provides a practical front end for selecting applications, storing compatibility profiles, running diagnostics, and choosing between the project runtime and an optional external comparison backend.

## Features

- WinForms interface for Windows 10/11 x64;
- executable and argument selection;
- working-directory selection;
- automatic discovery of `intel-sde/windows-9.58/sde.exe`;
- CPU profiles `-snb`, `-ivb`, `-hsw`, `-bdw`, and `-skl` for the optional external backend;
- optional `-mix -mix_format json` telemetry;
- optional experimental `-emu-xinuse 0` flag;
- per-application profile save/load;
- system and path diagnostics;
- event log at `%APPDATA%\SDEAvxLab\launcher.log`;
- independent SDE AVX Lab Runtime as the default backend;
- Intel SDE 9.58 retained only as a separate fallback backend.

The independent SDE AVX Lab backend is the primary development target. It is currently a self-test/core runtime and does not launch arbitrary games yet. Intel SDE 9.58 remains available as a separate fallback for users who need to launch a target while the own process runtime is being completed.

## Build on Windows

Requirements:

- Windows 10 or Windows 11 x64;
- .NET Framework 4.x C# compiler;
- CMake and a Visual Studio x64 C++ toolchain for the independent runtime.

Build and start the launcher:

```bat
Build.bat
Start.bat
```

When launching from `cmd.exe`, quote the executable name because it contains spaces:

```bat
start "SDE AVX Lab" "SDE AVX Lab.exe"
```

Build the independent runtime first:

```bat
cd ..\avx-runtime
Build-Windows.bat
```

The launcher automatically searches for `avx_runtime.exe` in `avx-runtime\build-windows\Release`.

## First external comparison test

1. Keep the official package at `..\windows-9.58` in this repository checkout or select `sde.exe` manually.
2. Start the launcher.
3. Select the target executable, such as `xenia_canary.exe`.
4. Select **Intel SDE 9.58 (fallback externo)** only if the own backend cannot yet launch the target.
5. Start with the `-snb` profile.
6. Keep `-emu-xinuse 0` disabled initially.
7. Enable `-mix` only when instruction telemetry is required.

A process opening is not proof of playable compatibility. Record initialization, menus, loaded content, frame rate, crashes, visual behavior, and logs separately.

## Project boundary

The launcher does not modify or embed Intel SDE binaries. The independent runtime is developed under `avx-runtime/` and the AVX1-to-SSE proof of concept is maintained under `avx1-sse-poc/`.
