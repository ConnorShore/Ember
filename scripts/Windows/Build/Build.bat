@echo off
setlocal EnableExtensions

:: ---------------------------------------------------------------------------
:: Builds Ember from the command line, so a build no longer requires opening
:: the Visual Studio IDE. MSBuild still comes from the VS installation.
::
::   Build.bat                            Release, whole solution
::   Build.bat Debug                      Debug, whole solution
::   Build.bat Release Ember-Test         only Ember-Test (and what it depends on)
::   Build.bat Debug Ember-Forge rebuild  full rebuild instead of incremental
::   Build.bat Dist clean                 delete this configuration's build output
::
:: Arguments are order-independent: Debug/Release/Dist/Profile picks the
:: configuration (default Release, matching RunTests.bat), "rebuild"/"clean"
:: picks the target, and anything else is treated as a project name.
::
:: Set MSBUILD_PATH to override MSBuild discovery.
:: Exits with MSBuild's exit code: 0 = success.
:: ---------------------------------------------------------------------------

set "ROOT=%~dp0..\..\.."

set CONFIG=Release
set PROJECT=
set TARGET=Build

call :classify "%~1"
call :classify "%~2"
call :classify "%~3"

:: ---------------------------------------------------------------
:: Locate MSBuild
:: ---------------------------------------------------------------
set "MSBUILD=%MSBUILD_PATH%"
if defined MSBUILD goto :have_msbuild

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo.
    echo [Build] Could not find vswhere.exe, so MSBuild cannot be located.
    echo         Install Visual Studio 2026 with the "Desktop development with C++"
    echo         workload, or set MSBUILD_PATH to the full path of MSBuild.exe.
    echo.
    exit /b 9009
)

:: -prerelease so a Preview install is found too; -products * covers Community and Build Tools.
:: Microsoft.Component.MSBuild is the canonical component id and genuinely has no "VisualStudio"
:: infix, unlike most others.
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"

if not defined MSBUILD (
    echo.
    echo [Build] Visual Studio was found but MSBuild was not.
    echo         Install the "Desktop development with C++" workload, or set
    echo         MSBUILD_PATH to the full path of MSBuild.exe.
    echo.
    exit /b 9009
)

:have_msbuild
if not exist "%MSBUILD%" (
    echo.
    echo [Build] MSBuild not found at "%MSBUILD%"
    echo.
    exit /b 9009
)

:: ---------------------------------------------------------------
:: Locate the solution. Premake's vs2026 action emits Ember.slnx;
:: older actions emit Ember.sln.
:: ---------------------------------------------------------------
set "SOLUTION=%ROOT%\Ember.sln"
if not exist "%SOLUTION%" set "SOLUTION=%ROOT%\Ember.slnx"

if not exist "%SOLUTION%" (
    echo.
    echo [Build] No solution found in the repository root.
    echo         Run scripts\Windows\Build\GenerateProjects.bat first.
    echo.
    exit /b 1
)

:: ---------------------------------------------------------------
:: Pick what to build. A named project builds its .vcxproj directly rather
:: than a solution target: MSBuild rewrites project names in the generated
:: solution metaproj, so "/t:Ember-Test:Build" does not resolve. Project
:: references still pull in everything it depends on.
:: ---------------------------------------------------------------
set "BUILD_WHAT=%SOLUTION%"
set "WHAT_LABEL=whole solution"
if not defined PROJECT goto :target_resolved

:: Main projects follow <Name>\<Name>.vcxproj.
set "BUILD_WHAT=%ROOT%\%PROJECT%\%PROJECT%.vcxproj"
set "WHAT_LABEL=%PROJECT%"
if exist "%BUILD_WHAT%" goto :target_resolved

:: Vendor projects live deeper, so search before giving up.
for /r "%ROOT%" %%F in ("%PROJECT%.vcxproj") do if exist "%%F" set "BUILD_WHAT=%%F"

:target_resolved
if not exist "%BUILD_WHAT%" (
    echo.
    echo [Build] No project file found for "%PROJECT%".
    echo         Main projects: Ember, Ember-Forge, Ember-Runtime, Ember-Tools, Ember-Test
    echo.
    exit /b 1
)

echo.
echo [Build] %TARGET% %WHAT_LABEL% ^| %CONFIG% ^| x64
echo.

:: /m builds independent projects in parallel; premake already sets /MP for files within a project.
"%MSBUILD%" "%BUILD_WHAT%" /nologo /m /v:minimal /p:Configuration=%CONFIG% /p:Platform=x64 /t:%TARGET%
set RESULT=%ERRORLEVEL%

echo.
if "%RESULT%"=="0" (
    echo [Build] SUCCEEDED - binaries in bin\%CONFIG%-windows-x86_64
) else (
    echo [Build] FAILED ^(exit code %RESULT%^)
    echo         If the error mentions an unknown project or a missing file, re-run
    echo         scripts\Windows\Build\GenerateProjects.bat - the generated .vcxproj
    echo         file lists are not auto-synced with the filesystem.
)
echo.

:: Keep the window open only when double-clicked, so this stays usable from other scripts.
echo %cmdcmdline% | find /i "%~nx0" >nul && if not defined EMBER_NO_PAUSE pause

exit /b %RESULT%

:: ---------------------------------------------------------------
:: Classifies one argument. Order-independent so "Debug rebuild" and
:: "rebuild Debug" behave the same.
:: ---------------------------------------------------------------
:: Every set is quoted: "set CONFIG=Debug & goto" would capture the space before the & into the value,
:: which then reaches MSBuild as Configuration=Debug<space>.
:classify
if "%~1"=="" goto :eof
if /i "%~1"=="Debug"   ( set "CONFIG=Debug"   & goto :eof )
if /i "%~1"=="Release" ( set "CONFIG=Release" & goto :eof )
if /i "%~1"=="Dist"    ( set "CONFIG=Dist"    & goto :eof )
if /i "%~1"=="Profile" ( set "CONFIG=Profile" & goto :eof )
if /i "%~1"=="rebuild" ( set "TARGET=Rebuild" & goto :eof )
if /i "%~1"=="clean"   ( set "TARGET=Clean"   & goto :eof )
set "PROJECT=%~1"
goto :eof
