@echo off
setlocal

echo ========================================
echo   Animation Editor - Run Script (Windows)
echo ========================================
echo.

:: Navigate to project root
cd /d "%~dp0..\..\..\"

echo Starting Animation Editor from: %cd%
echo Working directory: build\tools\animation_editor\Debug
echo.

:: Check if executable exists
if not exist "build\tools\animation_editor\Debug\animation_editor.exe" (
    echo ERROR: animation_editor.exe not found!
    echo Please run build_windows.bat first.
    pause
    exit /b 1
)

:: Change to the build directory and run
cd build\tools\animation_editor\Debug
animation_editor.exe

echo.
echo Animation Editor closed.
pause
