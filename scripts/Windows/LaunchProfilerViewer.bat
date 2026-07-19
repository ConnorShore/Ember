@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Open all Chrome-tracing JSON profiles from Profiles\ in https://ui.perfetto.dev/
rem Uses Google's official Perfetto helper (vendored as open_trace_in_ui.py):
rem   https://raw.githubusercontent.com/google/perfetto/main/tools/open_trace_in_ui
rem
rem Each file is opened in a new browser tab. The helper serves the trace from
rem http://127.0.0.1:9001 (required by Perfetto's CSP) until the UI fetches it.

pushd "%~dp0..\.."

set "PROFILES_DIR=%CD%\Profiles"
set "OPEN_TRACE_UI=%~dp0..\vendor\open_trace_in_ui.py"

if not exist "%OPEN_TRACE_UI%" (
    echo Missing helper script: "%OPEN_TRACE_UI%"
    popd
    exit /b 1
)

if not exist "%PROFILES_DIR%" (
    echo Profiles folder not found: "%PROFILES_DIR%"
    echo Run the engine once with EB_PROFILE enabled to generate Startup/Runtime/Shutdown traces.
    popd
    exit /b 1
)

dir /b "%PROFILES_DIR%\*.json" >nul 2>&1
if errorlevel 1 (
    echo No .json profile files found in "%PROFILES_DIR%"
    popd
    exit /b 1
)

rem Prefer the Windows py launcher, then python on PATH.
set "PYTHON_CMD="
where py >nul 2>&1
if not errorlevel 1 (
    set "PYTHON_CMD=py -3"
) else (
    where python >nul 2>&1
    if not errorlevel 1 set "PYTHON_CMD=python"
)

if not defined PYTHON_CMD (
    echo Python 3 is required to run open_trace_in_ui.py
    echo Install Python from https://www.python.org/downloads/ or the Microsoft Store, then retry.
    popd
    exit /b 1
)

rem Prefer Chrome when available (Python webbrowser respects BROWSER).
set "BROWSER="
if exist "%ProgramFiles%\Google\Chrome\Application\chrome.exe" (
    set "BROWSER=%ProgramFiles%\Google\Chrome\Application\chrome.exe"
) else if exist "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" (
    set "BROWSER=%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe"
) else if exist "%LocalAppData%\Google\Chrome\Application\chrome.exe" (
    set "BROWSER=%LocalAppData%\Google\Chrome\Application\chrome.exe"
)

if defined BROWSER (
    echo Using Chrome: %BROWSER%
) else (
    echo Chrome not found; using the default browser.
)

echo.
echo Opening Profiles\ traces with Perfetto open_trace_in_ui...
echo Leave this window open until each file has finished loading.
echo.

set "EXIT_CODE=0"
set "OPENED=0"

rem Preferred Ember session order first.
for %%F in (Startup.json Runtime.json Shutdown.json) do (
    if exist "%PROFILES_DIR%\%%F" (
        call :OpenOne "%PROFILES_DIR%\%%F"
        if errorlevel 1 set "EXIT_CODE=1"
    )
)

rem Then any other JSON traces that were not already opened above.
for %%F in ("%PROFILES_DIR%\*.json") do (
    set "NAME=%%~nxF"
    if /I not "!NAME!"=="Startup.json" if /I not "!NAME!"=="Runtime.json" if /I not "!NAME!"=="Shutdown.json" (
        call :OpenOne "%%~fF"
        if errorlevel 1 set "EXIT_CODE=1"
    )
)

echo.
if "%OPENED%"=="0" (
    echo No profile files were opened.
    set "EXIT_CODE=1"
) else if "%EXIT_CODE%"=="0" (
    echo Done. Opened %OPENED% profile(s) in Perfetto.
) else (
    echo Finished with errors after opening %OPENED% profile(s^).
)

popd
pause
exit /b %EXIT_CODE%

:OpenOne
set "TRACE=%~1"
echo --------------------------------------------
echo Opening "%TRACE%"
echo --------------------------------------------
%PYTHON_CMD% "%OPEN_TRACE_UI%" "%TRACE%"
if errorlevel 1 (
    echo Failed to open "%TRACE%"
    exit /b 1
)
set /a OPENED+=1
exit /b 0
