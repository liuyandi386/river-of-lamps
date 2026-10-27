@echo off
if not exist "%~dp0build\RiverOfLamps.exe" (
  echo build\RiverOfLamps.exe not found. Run build_cpp.bat first.
  pause
  exit /b 1
)
start "" "%~dp0build\RiverOfLamps.exe"
