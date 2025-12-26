@echo off
setlocal

echo ========================================
echo   UI Editor - Run Script (Windows)
echo ========================================
echo.

:: Navigate to project root
cd /d "%~dp0..\..\..\"

echo Project root: %cd%
echo.

:: Check if executable exists
if not exist "build\tools\ui_editor\Debug\ui_editor.exe" (
    echo ERROR: ui_editor.exe not found!
    echo Please run build_windows.bat first.
    pause
    exit /b 1
)

:: Change to executable directory (where assets are located)
cd /d "build\tools\ui_editor\Debug"
echo Starting UI Editor from: %cd%
echo.

:: Run the editor
ui_editor.exe

echo.
echo UI Editor closed.
pause

