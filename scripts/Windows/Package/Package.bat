@echo off
setlocal

:: Builds the portable zip from the staging tree. Unzipping it anywhere gives the same layout the
:: installer lays down, so both distributions behave identically.

pushd "%~dp0..\..\.."

call "%~dp0Stage.bat"
if errorlevel 1 goto :fail

:: Version values come from Stage.bat so they are extracted from Version.h exactly once.
call build\version.cmd

set OUT=EmberForge-%EMBER_VERSION_FULL%
set DIST=build\dist

if exist "%DIST%\%OUT%"     rmdir /s /q "%DIST%\%OUT%"
if exist "%DIST%\%OUT%.zip" del /q "%DIST%\%OUT%.zip"
mkdir "%DIST%" 2>nul

:: Copied under a versioned folder so the archive expands into one directory instead of scattering
:: files wherever the user unzipped it.
xcopy /e /i /y "build\stage" "%DIST%\%OUT%" > nul

echo Creating %DIST%\%OUT%.zip ...
powershell -NoProfile -Command "Compress-Archive -Force -Path '%DIST%\%OUT%' -DestinationPath '%DIST%\%OUT%.zip'"
if errorlevel 1 goto :fail

rmdir /s /q "%DIST%\%OUT%"

echo.
echo Done: %DIST%\%OUT%.zip
echo.
popd
endlocal
exit /b 0

:fail
echo.
echo Packaging FAILED.
echo.
popd
endlocal
PAUSE
exit /b 1
