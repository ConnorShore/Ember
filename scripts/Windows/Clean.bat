@echo off
echo Cleaning Visual Studio and Premake generated files...

pushd "%~dp0..\.."

echo Removing .sln and .vcxproj files...
del /s /q /f *.sln >nul 2>&1
del /s /q /f *.vcxproj >nul 2>&1
del /s /q /f *.vcxproj.user >nul 2>&1
del /s /q /f *.vcxproj.filters >nul 2>&1

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
)

echo.
echo Clean complete!