@echo off
setlocal enabledelayedexpansion

:: Assembles the shipped layout into build\stage. Both Package.bat (portable zip) and
:: BuildInstaller.bat (Inno Setup) consume this one tree so the two cannot drift apart.
::
:: The layout is flat and anchored to the executable, which is what Ember::Paths expects of an
:: installed build:
::
::   Ember-Forge.exe / Ember-Runtime.exe / EmberProject.ico / EmberCore\ / EditorAssets\ / LICENSE.txt

:: Navigate to repo root regardless of where the script is launched from
pushd "%~dp0..\..\.."

set CONFIG=Dist-windows-x86_64
set STAGE=build\stage
set VERSION_HEADER=Ember\src\Ember\Core\Version.h
:: Repo-relative rather than %~dp0-based: xcopy /exclude cannot take a quoted path, so an absolute
:: path would break on a clone whose directory contains a space.
set EXCLUDE=scripts\Windows\Package\StageExclude.txt
set ICON_SRC=Ember-Forge\assets\images\EmberIcon.ico

:: ---------------------------------------------------------------
:: Check that the Dist build exists. There is no CLI build for this
:: project, so the binaries have to already be there.
:: ---------------------------------------------------------------
set EXE=bin\%CONFIG%\Ember-Forge\Ember-Forge.exe
set RUNTIME_EXE=bin\%CONFIG%\Ember-Runtime\Ember-Runtime.exe
if not exist "%EXE%" (
    echo ERROR: %EXE% not found.
    echo Build Ember-Forge in the Dist configuration first.
    goto :fail
)
if not exist "%RUNTIME_EXE%" (
    echo ERROR: %RUNTIME_EXE% not found.
    echo Build Ember-Runtime in the Dist configuration first.
    goto :fail
)

:: ---------------------------------------------------------------
:: Clean previous staging tree
:: ---------------------------------------------------------------
if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%" 2>nul

:: ---------------------------------------------------------------
:: Extract the version so the installer never carries a second copy.
:: Values are the third whitespace-separated token; %%~i strips the quotes.
:: ---------------------------------------------------------------
set VER_FULL=
set VER_NUM=
for /f "tokens=3" %%i in ('findstr /b /c:"#define EMBER_VERSION_FULL" "%VERSION_HEADER%"') do set VER_FULL=%%~i
for /f "tokens=3" %%i in ('findstr /b /c:"#define EMBER_VERSION_STRING" "%VERSION_HEADER%"') do set VER_NUM=%%~i

if "%VER_FULL%"=="" (
    echo ERROR: could not read EMBER_VERSION_FULL from %VERSION_HEADER%.
    goto :fail
)
if "%VER_NUM%"=="" (
    echo ERROR: could not read EMBER_VERSION_STRING from %VERSION_HEADER%.
    goto :fail
)

:: version.iss is #included by the Inno script; version.cmd lets the calling .bat reuse the same
:: values without repeating the extraction (this script's endlocal would otherwise hide them).
> build\version.iss echo #define AppVersionFull "%VER_FULL%"
>> build\version.iss echo #define AppVersionNumeric "%VER_NUM%"

> build\version.cmd echo set EMBER_VERSION_FULL=%VER_FULL%
>> build\version.cmd echo set EMBER_VERSION_NUMERIC=%VER_NUM%

:: ---------------------------------------------------------------
:: Gather files
:: ---------------------------------------------------------------
echo Staging Ember Forge %VER_FULL% ...

copy /y "%EXE%"         "%STAGE%\" > nul
copy /y "%RUNTIME_EXE%" "%STAGE%\" > nul
copy /y "LICENSE"       "%STAGE%\LICENSE.txt" > nul

:: EXCLUDE drops leftover test content, the developer's local asset registry, and the cooked .bin
:: sidecars. Sidecars are uncompressed pixel data (DefaultSkybox.bin alone is 67 MB against a 17 MB
:: source) and TextureImporter::Load falls back to decoding the source, so shipping sources only
:: trades a little load time for a 4x smaller download. Drop the ".bin" line to ship them anyway.
:: Patterns are case-insensitive substrings of the full path, so they are \-qualified to stop
:: "skybox.hdr" from also matching "DefaultSkybox.hdr".
xcopy /e /i /y /exclude:%EXCLUDE% "Ember\assets"       "%STAGE%\EmberCore"    > nul
xcopy /e /i /y /exclude:%EXCLUDE% "Ember-Forge\assets" "%STAGE%\EditorAssets" > nul

:: Copied to a fixed name at the install root so the shortcut and .ebproj registry entries point at a
:: stable path, rather than one that moves if the editor's asset folders are ever reorganised.
if exist "%ICON_SRC%" (
    copy /y "%ICON_SRC%" "%STAGE%\EmberProject.ico" > nul
) else (
    echo WARNING: %ICON_SRC% is missing - shortcuts and .ebproj files will have no icon.
)

echo.
echo Staged to %STAGE%
echo.
popd
endlocal
exit /b 0

:fail
echo.
echo Staging FAILED.
echo.
popd
endlocal
exit /b 1
