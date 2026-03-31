@echo off
setlocal enabledelayedexpansion

REM ===============================================================
REM copy_files.bat
REM 
REM Usage (in Premake postbuildcommands):
REM     call copy_files.bat "<SrcDir>" "<DstDir>" "<Config>" "<Pattern>"
REM 
REM Examples:
REM     call copy_files.bat "D:\assimp\build" "D:\bin\assimp\Debug" "Debug" "*.dll"
REM     call copy_files.bat "D:\yaml-cpp\build" "D:\bin\yaml-cpp\Release" "Release" "*.lib"
REM 
REM Parameters:
REM   %1 = Source base directory
REM   %2 = Destination directory
REM   %3 = Config (Debug / Release / Profile / Dist)
REM   %4 = File pattern (*.dll, *.lib, *.pdb, etc)
REM ===============================================================

set SRC="%~1"
set DST="%~2"
set CFG="%~3"
set PATTERN=%~4

echo ==========================================
echo   COPY FILES SCRIPT
echo ==========================================
echo Source Base:  %SRC%
echo Destination:  %DST%
echo Config:       %CFG%
echo Pattern:      %PATTERN%
echo ==========================================

REM Convert to usable variables without quotes
set SRC_DIR=%~1
set DST_DIR=%~2
set CONFIG=%~3

echo.
echo --- Checking source directory ---
if not exist "%SRC_DIR%" (
    echo ERROR: Source directory does not exist:
    echo %SRC_DIR%
    exit /B 0
)

echo.
echo --- Creating destination directory if needed ---
if not exist "%DST_DIR%" (
    mkdir "%DST_DIR%"
)

echo.
echo --- Listing source contents ---
dir "%SRC_DIR%\%CONFIG%\%PATTERN%" /B 2>nul
echo.

echo --- Copying files ---
xcopy "%SRC_DIR%\%CONFIG%\%PATTERN%" "%DST_DIR%\" /I /Q /Y /R /C >nul

if %ERRORLEVEL% GEQ 1 (
    echo No files copied or nothing matched. Continuing.
) else (
    echo Files copied successfully.
)

echo ==========================================
echo DONE
echo ==========================================

exit /B 0
