@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "PATH=C:\Program Files\RedPanda-Cpp\mingw64\bin;%PATH%"

where g++ >nul 2>nul
if errorlevel 1 (
  echo [ERROR] g++ not found. Install MinGW-w64 / RedPanda-C++.
  pause
  exit /b 1
)

set "CMAKE_BIN=D:\miniconda\envs\lerobot\Scripts\cmake.exe"
if not exist "%CMAKE_BIN%" set "CMAKE_BIN=cmake"
"%CMAKE_BIN%" --version >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cmake not found.
  pause
  exit /b 1
)

echo ===== Configure =====
"%CMAKE_BIN%" -S cpp -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  pause
  exit /b 1
)

echo ===== Build =====
"%CMAKE_BIN%" --build build -j 8
if errorlevel 1 (
  echo [ERROR] Build failed.
  pause
  exit /b 1
)

echo.
echo Build OK:
echo   build\RiverOfLamps.exe
echo   build\river_tests.exe
pause
