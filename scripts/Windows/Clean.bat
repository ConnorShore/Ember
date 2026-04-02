@echo off
echo Cleaning Visual Studio and Premake generated files...

pushd "%~dp0..\.."

echo Removing .sln and .vcxproj files...
for /r %%F in (*.sln) do (echo %%F | findstr /i /c:"imgui\\examples" >nul || del /q /f "%%F" >nul 2>&1)
for /r %%F in (*.vcxproj) do (echo %%F | findstr /i /c:"imgui\\examples" >nul || del /q /f "%%F" >nul 2>&1)
for /r %%F in (*.vcxproj.user) do (echo %%F | findstr /i /c:"imgui\\examples" >nul || del /q /f "%%F" >nul 2>&1)
for /r %%F in (*.vcxproj.filters) do (echo %%F | findstr /i /c:"imgui\\examples" >nul || del /q /f "%%F" >nul 2>&1)

echo Removing .vs folder...
IF EXIST ".vs" rmdir /s /q ".vs"

echo Removing bin directories...
IF EXIST "bin" rmdir /s /q "bin"

echo Removing bin-int directories...
IF EXIST "bin-int" rmdir /s /q "bin-int"

echo Removing bin and bin-int directories in Ember/vendor subdirectories...
for /d %%D in ("Ember\vendor\*") do (
    IF EXIST "%%D\bin" rmdir /s /q "%%D\bin"
    IF EXIST "%%D\bin-int" rmdir /s /q "%%D\bin-int"
    IF EXIST "%%D\obj" rmdir /s /q "%%D\obj"
)

echo Removing bin and bin-int directories in Ember-Forge/vendor subdirectories...
for /d %%D in ("Ember-Forge\vendor\*") do (
    IF EXIST "%%D\bin" rmdir /s /q "%%D\bin"
    IF EXIST "%%D\bin-int" rmdir /s /q "%%D\bin-int"
    IF EXIST "%%D\obj" rmdir /s /q "%%D\obj"
)

echo Removing bin and bin-int directories in Ember-Tools/vendor subdirectories...
for /d %%D in ("Ember-Tools\vendor\*") do (
    IF EXIST "%%D\bin" rmdir /s /q "%%D\bin"
    IF EXIST "%%D\bin-int" rmdir /s /q "%%D\bin-int"
    IF EXIST "%%D\obj" rmdir /s /q "%%D\obj"
)


echo.
echo Clean complete!