@echo off
setlocal

:: Builds the Windows installer from the staging tree with Inno Setup.
::
:: Pass /DSIGN through to sign the output once a code-signing certificate exists:
::   BuildInstaller.bat /DSIGN

pushd "%~dp0..\..\.."

:: ---------------------------------------------------------------
:: Locate the Inno Setup compiler
:: ---------------------------------------------------------------
set ISCC=%INNO_SETUP%
if not defined ISCC set ISCC=%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe
if not exist "%ISCC%" set ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe

if not exist "%ISCC%" (
    echo ERROR: Could not find ISCC.exe, the Inno Setup command line compiler.
    echo.
    echo Install Inno Setup 7 from https://jrsoftware.org/isdl.php, or set INNO_SETUP
    echo to the full path of ISCC.exe if it lives somewhere non-standard.
    goto :fail
)

:: ---------------------------------------------------------------
:: Stage, then compile
:: ---------------------------------------------------------------
call "%~dp0Stage.bat"
if errorlevel 1 goto :fail

if not exist "Ember-Forge\assets\images\EmberIcon.ico" (
    echo ERROR: Ember-Forge\assets\images\EmberIcon.ico is required to build the installer.
    echo It is used for Setup.exe, the shortcuts and the .ebproj association.
    goto :fail
)

mkdir build\installer 2>nul

echo Compiling installer with "%ISCC%" ...
"%ISCC%" %* "installer\EmberForge.iss"
if errorlevel 1 goto :fail

echo.
echo Done. Installer written to build\installer
echo.
popd
endlocal
exit /b 0

:fail
echo.
echo Installer build FAILED.
echo.
popd
endlocal
PAUSE
exit /b 1
