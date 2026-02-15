@echo off
setlocal enableextensions enabledelayedexpansion

set "FILAMENT_VERSION=%FILAMENT_VERSION%"
if not defined FILAMENT_VERSION set "FILAMENT_VERSION=v1.69.2"

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "FILAMENT_DIR=%PROJECT_ROOT%\engine\third_party\filament"
set "FILAMENT_ASSET=filament-%FILAMENT_VERSION%-windows.tgz"
set "FILAMENT_URL=https://github.com/google/filament/releases/download/%FILAMENT_VERSION%/%FILAMENT_ASSET%"

set "RAW_ARCH=%PROCESSOR_ARCHITECTURE%"
if defined PROCESSOR_ARCHITEW6432 set "RAW_ARCH=%PROCESSOR_ARCHITEW6432%"
set "ARCH=%RAW_ARCH%"
if /i "%RAW_ARCH%"=="AMD64" set "ARCH=x86_64"
if /i "%RAW_ARCH%"=="ARM64" set "ARCH=arm64"

echo === Monster Engine Setup ===
echo   Platform: Windows (%ARCH%)
echo   Filament: %FILAMENT_VERSION%
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo ERROR: CMake not found in PATH.
    echo Install CMake first and run this script again.
    exit /b 1
)

where tar >nul 2>&1
if errorlevel 1 (
    echo ERROR: tar not found in PATH.
    echo Use Windows 10/11 tar.exe or install bsdtar.
    exit /b 1
)

call :find_vcvars
if defined VCVARS_PATH (
    echo Found Visual Studio build environment: %VCVARS_PATH%
) else (
    echo WARN: Visual Studio 2022 Build Tools not detected in default locations.
    echo       Build may fail until MSVC tools are installed.
)

if exist "%FILAMENT_DIR%\lib" if exist "%FILAMENT_DIR%\include" (
    echo.
    echo Filament SDK already present at "%FILAMENT_DIR%"
    set "REDOWNLOAD="
    set /p "REDOWNLOAD=Re-download? [y/N] "
    if /i "!REDOWNLOAD!"=="y" (
        call :download_filament
        if errorlevel 1 exit /b 1
    ) else (
        echo Keeping existing Filament SDK.
    )
) else (
    call :download_filament
    if errorlevel 1 exit /b 1
)

echo.
echo === Setup Complete ===
if exist "%FILAMENT_DIR%\lib\x86_64" (
    echo   [OK] Filament SDK (x86_64)
) else if exist "%FILAMENT_DIR%\lib\arm64" (
    echo   [OK] Filament SDK (arm64)
) else (
    echo   [WARN] Filament SDK present, but no expected library directory was found.
)
echo.
echo To build:
echo   scripts\build.bat
echo.
exit /b 0

:download_filament
echo.
echo Downloading Filament SDK %FILAMENT_VERSION%...
echo   URL: %FILAMENT_URL%

set "TEMP_DIR=%TEMP%\monster-engine-setup-%RANDOM%%RANDOM%"
set "ARCHIVE_PATH=%TEMP_DIR%\%FILAMENT_ASSET%"
mkdir "%TEMP_DIR%" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to create temporary directory "%TEMP_DIR%".
    exit /b 1
)

call :download_file "%FILAMENT_URL%" "%ARCHIVE_PATH%"
if errorlevel 1 (
    rd /s /q "%TEMP_DIR%" >nul 2>&1
    echo ERROR: Failed to download Filament SDK.
    exit /b 1
)

if exist "%FILAMENT_DIR%" (
    echo   Removing existing Filament SDK...
    rd /s /q "%FILAMENT_DIR%"
    if exist "%FILAMENT_DIR%" (
        rd /s /q "%TEMP_DIR%" >nul 2>&1
        echo ERROR: Failed to remove "%FILAMENT_DIR%".
        exit /b 1
    )
)

mkdir "%FILAMENT_DIR%" >nul 2>&1
if errorlevel 1 (
    rd /s /q "%TEMP_DIR%" >nul 2>&1
    echo ERROR: Failed to create "%FILAMENT_DIR%".
    exit /b 1
)

echo   Extracting SDK...
tar -xzf "%ARCHIVE_PATH%" -C "%FILAMENT_DIR%" --strip-components=1
if errorlevel 1 (
    rd /s /q "%TEMP_DIR%" >nul 2>&1
    echo ERROR: Failed to extract "%ARCHIVE_PATH%".
    exit /b 1
)

rd /s /q "%TEMP_DIR%" >nul 2>&1

rem ── Reorganize Windows SDK layout ───────────────────────────────────
rem The Windows Filament SDK extracts with a flat layout:
rem   filament/Engine.h, backend/DriverEnums.h, x86_64/md/*.lib ...
rem But our CMake expects the macOS-style layout:
rem   include/filament/Engine.h, lib/x86_64/*.lib
rem
rem Move header directories into include/ and libs into lib/x86_64/.

if exist "%FILAMENT_DIR%\x86_64" (
    echo   Reorganizing Windows SDK layout...

    rem Create include/ and move all header directories into it
    mkdir "%FILAMENT_DIR%\include" >nul 2>&1

    rem Known Filament header directories to move into include/
    for %%D in (
        backend
        camutils
        filamat
        filament
        filament-generatePrefilterMipmap
        filament-iblprefilter
        filament-matp
        filameshio
        geometry
        gltfio
        ibl
        image
        imageio-lite
        ktxreader
        math
        mathio
        mikktspace
        tsl
        uberz
        utils
        viewer
    ) do (
        if exist "%FILAMENT_DIR%\%%D" (
            move "%FILAMENT_DIR%\%%D" "%FILAMENT_DIR%\include\%%D" >nul 2>&1
        )
    )

    rem Create lib/x86_64/ and copy the /MD runtime variant (matches our MSVC config)
    mkdir "%FILAMENT_DIR%\lib" >nul 2>&1
    mkdir "%FILAMENT_DIR%\lib\x86_64" >nul 2>&1

    if exist "%FILAMENT_DIR%\x86_64\md" (
        xcopy "%FILAMENT_DIR%\x86_64\md\*.lib" "%FILAMENT_DIR%\lib\x86_64\" /Q /Y >nul 2>&1
    )

    rem Also keep debug libs available
    if exist "%FILAMENT_DIR%\x86_64\mdd" (
        mkdir "%FILAMENT_DIR%\lib\x86_64_debug" >nul 2>&1
        xcopy "%FILAMENT_DIR%\x86_64\mdd\*.lib" "%FILAMENT_DIR%\lib\x86_64_debug\" /Q /Y >nul 2>&1
    )

    echo   Windows SDK reorganized into include/ + lib/ layout.
)

echo   Filament SDK %FILAMENT_VERSION% installed successfully.
exit /b 0

:download_file
set "DOWNLOAD_URL=%~1"
set "OUTPUT_PATH=%~2"

where curl >nul 2>&1
if not errorlevel 1 (
    curl -L --fail --progress-bar -o "%OUTPUT_PATH%" "%DOWNLOAD_URL%"
    exit /b %errorlevel%
)

where powershell >nul 2>&1
if not errorlevel 1 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "try { Invoke-WebRequest -Uri \"%DOWNLOAD_URL%\" -OutFile \"%OUTPUT_PATH%\" -UseBasicParsing } catch { exit 1 }"
    exit /b %errorlevel%
)

echo ERROR: Neither curl nor PowerShell is available for download.
exit /b 1

:find_vcvars
set "VCVARS_PATH="
for %%P in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if not defined VCVARS_PATH if exist %%~P set "VCVARS_PATH=%%~P"
)
exit /b 0
