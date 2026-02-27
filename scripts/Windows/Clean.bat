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

echo.
echo Clean complete!