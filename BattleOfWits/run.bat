@echo off
setlocal ENABLEDELAYEDEXPANSION

cd /d "%~dp0"

REM Ensure MSVC environment (optional)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
  call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
)

set "ROOT=%~dp0"
set "VCPKG_BIN=%ROOT%vcpkg\installed\x64-windows\bin"
set "PATH=%VCPKG_BIN%;%PATH%"

REM Locate cmake if not on PATH
set "CMAKE=cmake"
where /q cmake || set "CMAKE=%ProgramFiles%\CMake\bin\cmake.exe"
if not exist "%CMAKE%" set "CMAKE=%ProgramFiles(x86)%\CMake\bin\cmake.exe"

REM Always (re)configure to ensure toolchain and SFML are picked up
"%CMAKE%" -S . -B build ^
  -DCMAKE_TOOLCHAIN_FILE="%ROOT%vcpkg\scripts\buildsystems\vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DSFML_DIR="%ROOT%vcpkg\installed\x64-windows\share\sfml"
if errorlevel 1 (
  echo CMake configure failed.
  exit /b 1
)

REM Build (try Debug multi-config first; fallback to single-config and Release)
"%CMAKE%" --build build --config Debug
if errorlevel 1 (
  "%CMAKE%" --build build
  if errorlevel 1 (
    "%CMAKE%" --build build --config Release
    if errorlevel 1 (
      echo Build failed.
      exit /b 1
    )
  )
)

REM Locate executable across common layouts
set "EXE="
if exist "build\Debug\battle_of_wits.exe" set "EXE=build\Debug\battle_of_wits.exe"
if not defined EXE if exist "build\Release\battle_of_wits.exe" set "EXE=build\Release\battle_of_wits.exe"
if not defined EXE if exist "build\RelWithDebInfo\battle_of_wits.exe" set "EXE=build\RelWithDebInfo\battle_of_wits.exe"
if not defined EXE if exist "build\MinSizeRel\battle_of_wits.exe" set "EXE=build\MinSizeRel\battle_of_wits.exe"
if not defined EXE if exist "build\battle_of_wits.exe" set "EXE=build\battle_of_wits.exe"

if not defined EXE (
  echo Could not find built executable. Build may have failed.
  exit /b 1
)

REM Ensure data directory exists for leaderboard persistence
if not exist "data" mkdir "data"

"%EXE%"
