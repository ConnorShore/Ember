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
for /d /r %%x in (bin) do @if exist "%%x" rmdir /s /q "%%x"

echo Removing bin-int directories...
for /d /r %%x in (bin-int) do @if exist "%%x" rmdir /s /q "%%x"

echo.
echo Clean complete!