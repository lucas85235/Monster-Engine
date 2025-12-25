@echo off
setlocal

echo ========================================
echo   Map Editor - Build Script (Windows)
echo ========================================
echo.

:: Navigate to project root (two levels up from scripts folder)
cd /d "%~dp0..\..\..\"

echo [1/3] Project root: %cd%
echo.

:: Check if build directory exists
if not exist "build" (
    echo [2/3] Creating build directory and generating project files...
    cmake -B build -G "Visual Studio 17 2022" -A x64
) else (
    echo [2/3] Regenerating CMake project files...
    cmake -B build
)

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo [3/3] Building map_editor (Debug)...
cmake --build build --target map_editor --config Debug

if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ========================================
echo   Build completed successfully!
echo ========================================
echo.
echo Executable: build\tools\map_editor\Debug\map_editor.exe
echo.

pause
