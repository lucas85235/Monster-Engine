@echo off
setlocal enableextensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "GENERATOR=Visual Studio 17 2022"

set "BUILD_TYPE=%BUILD_TYPE%"
if not defined BUILD_TYPE set "BUILD_TYPE=RelWithDebInfo"
if not "%~1"=="" set "BUILD_TYPE=%~1"

set "FILAMENT_DIR=%PROJECT_ROOT%\engine\third_party\filament"

echo === Monster Engine Build ===
echo   Generator: %GENERATOR%
echo   Config:    %BUILD_TYPE%
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH.
    echo Install CMake and run this script again.
    exit /b 1
)

if not exist "%FILAMENT_DIR%\include" (
    echo ERROR: Filament SDK headers were not found at "%FILAMENT_DIR%\include".
    echo Run scripts\setup.bat first.
    exit /b 1
)

if not exist "%FILAMENT_DIR%\lib" (
    echo ERROR: Filament SDK libraries were not found at "%FILAMENT_DIR%\lib".
    echo Run scripts\setup.bat first.
    exit /b 1
)

cd /d "%PROJECT_ROOT%" || exit /b 1

set "SCCACHE_FLAGS="
where sccache >nul 2>&1
if not errorlevel 1 (
    set "SCCACHE_FLAGS=-DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache"
    echo sccache detected: enabling compiler launcher.
    echo.
)

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -G "%GENERATOR%" ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
      %SCCACHE_FLAGS%
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    exit /b %errorlevel%
)

cmake --build "%BUILD_DIR%" --config "%BUILD_TYPE%" --parallel
if errorlevel 1 (
    echo ERROR: Build failed.
    exit /b %errorlevel%
)

set "SANDBOX_EXE=%BUILD_DIR%\apps\sandbox\%BUILD_TYPE%\sandbox.exe"
echo.
echo Build completed successfully.
if exist "%SANDBOX_EXE%" (
    echo Sandbox executable: "%SANDBOX_EXE%"
) else (
    echo WARN: Sandbox executable not found at expected location:
    echo       "%SANDBOX_EXE%"
)
echo.
exit /b 0
