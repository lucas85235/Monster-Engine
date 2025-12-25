@echo off
setlocal

echo ========================================
echo   Map Editor - Run Script (Windows)
echo ========================================
echo.

:: Navigate to project root
cd /d "%~dp0..\..\..\"

echo Starting Map Editor from: %cd%
echo.

:: Check if executable exists
if not exist "build\tools\map_editor\Debug\map_editor.exe" (
    echo ERROR: map_editor.exe not found!
    echo Please run build_windows.bat first.
    pause
    exit /b 1
)

:: Run the editor from project root (so assets are found)
build\tools\map_editor\Debug\map_editor.exe

echo.
echo Map Editor closed.
pause
