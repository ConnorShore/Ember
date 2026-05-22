@echo off
setlocal

:: Navigate to repo root regardless of where the script is launched from
pushd "%~dp0..\.."

:: ---------------------------------------------------------------
:: Configuration
:: Edit VERSION before each release.
:: ---------------------------------------------------------------
set VERSION=v0.1.1-alpha
set CONFIG=Dist-windows-x86_64
set OUT=EmberForge-%VERSION%

:: ---------------------------------------------------------------
:: Clean previous package
:: ---------------------------------------------------------------
if exist "%OUT%" (
    echo Removing old package folder...
    rmdir /s /q "%OUT%"
)
if exist "%OUT%.zip" (
    echo Removing old zip...
    del /q "%OUT%.zip"
)

:: ---------------------------------------------------------------
:: Check that the Dist build exists
:: ---------------------------------------------------------------
set EXE=bin\%CONFIG%\Ember-Forge\Ember-Forge.exe
set RUNTIME_EXE=bin\%CONFIG%\Ember-Runtime\Ember-Runtime.exe
if not exist "%EXE%" (
    echo ERROR: %EXE% not found.
    echo Build the project in Dist configuration first.
    goto :fail
)
if not exist "%RUNTIME_EXE%" (
    echo ERROR: %RUNTIME_EXE% not found.
    echo Build the project in Dist configuration first.
    goto :fail
)

:: ---------------------------------------------------------------
:: Gather files
:: ---------------------------------------------------------------
echo Packaging %OUT%...

mkdir "%OUT%"
copy /y "%EXE%"         "%OUT%\" > nul
mkdir "%OUT%\bin\%CONFIG%\Ember-Runtime"
copy /y "%RUNTIME_EXE%" "%OUT%\bin\%CONFIG%\Ember-Runtime\" > nul

xcopy /e /i /y "Ember\assets"       "%OUT%\Ember\assets"       > nul
xcopy /e /i /y "Ember-Forge\assets" "%OUT%\Ember-Forge\assets" > nul
copy  /y        "imgui.ini"          "%OUT%\"                   > nul

:: ---------------------------------------------------------------
:: Create zip
:: ---------------------------------------------------------------
echo Creating %OUT%.zip...
powershell -NoProfile -Command "Compress-Archive -Force -Path '%OUT%' -DestinationPath '%OUT%.zip'"
if errorlevel 1 goto :fail

echo Cleaning up...
rmdir /s /q "%OUT%"

echo.
echo Done: %OUT%.zip
echo.
goto :end

:fail
echo.
echo Packaging FAILED.
echo.
popd
exit /b 1

:end
popd
endlocal
PAUSE
