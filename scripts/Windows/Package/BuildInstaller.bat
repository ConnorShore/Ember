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
:: Version 6 is checked too because that is what Chocolatey installs (and what CI uses); the script
:: only needs Inno 6.3+ for the x64compatible architecture identifier.
set ISCC=%INNO_SETUP%
if defined ISCC goto :have_iscc

set "ISCC=%ProgramFiles(x86)%\Inno Setup 7\ISCC.exe"
if exist "%ISCC%" goto :have_iscc
set "ISCC=%ProgramFiles%\Inno Setup 7\ISCC.exe"
if exist "%ISCC%" goto :have_iscc
set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if exist "%ISCC%" goto :have_iscc
set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"

:have_iscc
if not exist "%ISCC%" (
    echo ERROR: Could not find ISCC.exe, the Inno Setup command line compiler.
    echo.
    echo Install Inno Setup 6.3 or newer from https://jrsoftware.org/isdl.php, or set
    echo INNO_SETUP to the full path of ISCC.exe if it lives somewhere non-standard.
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
if not defined EMBER_NO_PAUSE PAUSE
exit /b 1
