@echo off
:: Cleans, then regenerates the Visual Studio solution and project files with Premake.
:: Run this whenever a premake5.lua changes or source files are added/removed - the generated
:: .vcxproj file lists are not auto-synced with the filesystem.

:: %~dp0-qualified so this works when launched from any directory, not just its own.
call "%~dp0Clean.bat"

pushd "%~dp0..\..\.."
call vendor\premake\bin\premake5.exe vs2026
popd
PAUSE
