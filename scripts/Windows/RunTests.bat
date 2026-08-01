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

set CONFIG=Release
set ARGS=

:parse
if "%~1"=="" goto run
if /i "%~1"=="Debug"   ( set CONFIG=Debug   & shift & goto parse )
if /i "%~1"=="Release" ( set CONFIG=Release & shift & goto parse )
if /i "%~1"=="Dist"    ( set CONFIG=Dist    & shift & goto parse )
if /i "%~1"=="Profile" ( set CONFIG=Profile & shift & goto parse )
set ARGS=%ARGS% %1
shift
goto parse

:run
pushd "%~dp0..\.."

set EXE=bin\%CONFIG%-windows-x86_64\Ember-Test\Ember-Test.exe

if not exist "%EXE%" (
    echo.
    echo [Ember-Test] Could not find "%EXE%"
    echo              Build the Ember-Test project in the %CONFIG% configuration first.
    echo              If you have added or removed test source files, re-run
    echo              scripts\Windows\GenerateProjects.bat so the .vcxproj picks them up.
    echo.
    popd
    endlocal
    exit /b 2
)

REM Debug builds run several times slower than Release, so loosen the performance budgets to match.
REM Without this the perf tests report the configuration difference rather than a real regression.
if /i "%CONFIG%"=="Debug" if "%EMBER_TEST_PERF_SCALE%"=="" set EMBER_TEST_PERF_SCALE=8

echo.
echo [Ember-Test] Running %CONFIG% build...%ARGS%
echo.

"%EXE%"%ARGS%
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
endlocal & exit /b %RESULT%
