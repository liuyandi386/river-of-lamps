@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "VERSION=1.0.1"
set "PKGNAME=river-of-lamps-v%VERSION%-win64"
set "STAGE=dist\%PKGNAME%"
set "ZIP=dist\%PKGNAME%.zip"

if not exist "build\RiverOfLamps.exe" (
  echo [ERROR] build\RiverOfLamps.exe not found. Run build_cpp.bat first.
  exit /b 1
)
if not exist "README.md" (
  echo [ERROR] README.md not found.
  exit /b 1
)

echo ==== Packaging %PKGNAME% ====
if exist "dist" rmdir /s /q "dist"
mkdir "%STAGE%"
mkdir "%STAGE%\data"
mkdir "%STAGE%\cpp"
mkdir "%STAGE%\docs"
mkdir "%STAGE%\docs\design"
mkdir "%STAGE%\docs\rules"
mkdir "%STAGE%\licenses"

copy /y "build\RiverOfLamps.exe" "%STAGE%\" >nul
copy /y "README.md" "%STAGE%\" >nul
copy /y "update.md" "%STAGE%\" >nul
copy /y "THIRD_PARTY_NOTICES.md" "%STAGE%\" >nul

xcopy /e /i /y "data\levels" "%STAGE%\data\levels" >nul
xcopy /e /i /y "cpp\screenshots" "%STAGE%\cpp\screenshots" >nul
copy /y "cpp\README.md" "%STAGE%\cpp\" >nul

copy /y "docs\migration_cpp.md" "%STAGE%\docs\" >nul
copy /y "docs\design\outline.md" "%STAGE%\docs\design\" >nul
copy /y "docs\design\decisions.md" "%STAGE%\docs\design\" >nul
copy /y "docs\rules\element_rules.md" "%STAGE%\docs\rules\" >nul

copy /y "cpp\third_party\SFML-2.6.1\license.md" "%STAGE%\licenses\SFML-2.6.1.txt" >nul
copy /y "cpp\third_party\nlohmann\LICENSE.MIT" "%STAGE%\licenses\nlohmann-json-MIT.txt" >nul
copy /y "cpp\third_party\fonts\OFL-SarasaGothic.txt" "%STAGE%\licenses\SarasaGothic-OFL.txt" >nul
copy /y "cpp\third_party\imgui\LICENSE.txt" "%STAGE%\licenses\Dear-ImGui-MIT.txt" >nul
copy /y "cpp\third_party\imgui-sfml\LICENSE" "%STAGE%\licenses\ImGui-SFML-MIT.txt" >nul

> "%STAGE%\RUNNING.txt" echo River of Lamps v%VERSION% - Windows x64 portable
>>"%STAGE%\RUNNING.txt" echo.
>>"%STAGE%\RUNNING.txt" echo 1. Extract the whole folder (do not move the exe alone).
>>"%STAGE%\RUNNING.txt" echo 2. Double-click RiverOfLamps.exe.
>>"%STAGE%\RUNNING.txt" echo 3. No Godot, SFML, compiler, or runtime installation is required.
>>"%STAGE%\RUNNING.txt" echo 4. Progress is stored in %%APPDATA%%\RiverOfLamps\progress.json.
>>"%STAGE%\RUNNING.txt" echo 5. Display/splash settings are stored in %%APPDATA%%\RiverOfLamps\settings.json.
>>"%STAGE%\RUNNING.txt" echo 6. Read README.md and update.md for details.

where tar >nul 2>nul
if errorlevel 1 (
  echo [ERROR] tar.exe not found.
  exit /b 1
)
tar -a -c -f "%ZIP%" -C "dist" "%PKGNAME%"
if not exist "%ZIP%" (
  echo [ERROR] zip creation failed.
  exit /b 1
)

echo.
echo [OK] %ZIP%
for %%f in ("%ZIP%") do echo Size: %%~zf bytes
echo.
certutil -hashfile "%ZIP%" SHA256 | findstr /v /c:":"
echo.
echo Next: git tag -a v%VERSION% -m "v%VERSION%" ^&^& git push origin v%VERSION%
echo Then: gh release create v%VERSION% "%ZIP%" --title "v%VERSION%" --notes-file update.md
pause
