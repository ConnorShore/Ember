@echo off
REM ---------------------------------------------------------------------------
REM Runs the Ember-Test suite from the workspace root, which is where the engine
REM expects Ember/assets, Ember-Test/golden, Logs/ and Profiles/ to live.
REM
REM   RunTests.bat                        Release, every test
REM   RunTests.bat Debug                  Debug, every test
REM   RunTests.bat --filter=unit          Release, only the unit tests
REM   RunTests.bat Debug --run=Physics    Debug, only tests whose Suite::Name contains "Physics"
REM   RunTests.bat --list                 list the registered tests without running them
REM   RunTests.bat --xml=Profiles\tests.xml --perf-csv=Profiles\perf.csv
REM
REM A leading Debug/Release/Dist/Profile selects the build configuration; every
REM other argument is passed straight through to the executable.
REM Exits with the executable's exit code: 0 = all passed, 1 = failures.
REM ---------------------------------------------------------------------------

setlocal

REM Resolve the workspace root before anything else touches the arguments.
set "ROOT=%~dp0..\..\.."

REM Only the first argument can be a configuration; everything else is passed straight through.
REM The pass-through set comes from %* rather than being rebuilt token by token, because cmd splits
REM batch arguments on "=" as well as on spaces - rebuilding turns --run=Physics into --run Physics,
REM which the executable does not recognise and silently ignores.
set CONFIG=Release
set "ARGS=%*"
set CONFIGGIVEN=

if /i "%~1"=="Debug"   set CONFIGGIVEN=1
if /i "%~1"=="Release" set CONFIGGIVEN=1
if /i "%~1"=="Dist"    set CONFIGGIVEN=1
if /i "%~1"=="Profile" set CONFIGGIVEN=1

if defined CONFIGGIVEN (
    set "CONFIG=%~1"
    call set "ARGS=%%ARGS:*%~1=%%"
)

:run
pushd "%ROOT%"

set EXE=bin\%CONFIG%-windows-x86_64\Ember-Test\Ember-Test.exe

if not exist "%EXE%" (
    echo.
    echo [Ember-Test] Could not find "%EXE%"
    echo              Build the Ember-Test project in the %CONFIG% configuration first.
    echo              Build it with scripts\Windows\Build\Build.bat %CONFIG% Ember-Test
    echo              If you have added or removed test source files, re-run
    echo              scripts\Windows\Build\GenerateProjects.bat so the .vcxproj picks them up.
    echo.
    popd
    endlocal
    if not defined EMBER_NO_PAUSE pause
    exit /b 2
)

REM Debug builds run several times slower than Release, so loosen the performance budgets to match.
REM Without this the perf tests report the configuration difference rather than a real regression.
if /i "%CONFIG%"=="Debug" if "%EMBER_TEST_PERF_SCALE%"=="" set EMBER_TEST_PERF_SCALE=8

echo.
echo [Ember-Test] Running %CONFIG% build... %ARGS%
echo.

"%EXE%" %ARGS%
set RESULT=%ERRORLEVEL%

echo.
if "%RESULT%"=="0" (
    echo [Ember-Test] PASSED
) else (
    echo [Ember-Test] FAILED ^(exit code %RESULT%^)
    echo              Per-test progress: Logs\test-progress.log
    echo              Engine log:        Logs\test.txt
)
echo.

popd
endlocal
if not defined EMBER_NO_PAUSE pause
exit /b %RESULT%
