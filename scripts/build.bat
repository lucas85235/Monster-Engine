@echo off
setlocal enableextensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "PROJECT_ROOT=%%~fI"
set "BUILD_DIR=%PROJECT_ROOT%\build"
set "GENERATOR=Visual Studio 17 2022"

if /I "%~1"=="/?" goto :usage
if /I "%~1"=="-h" goto :usage
if /I "%~1"=="--help" goto :usage

set "BUILD_TYPE=%BUILD_TYPE%"
if not defined BUILD_TYPE set "BUILD_TYPE=RelWithDebInfo"
if not "%~1"=="" set "BUILD_TYPE=%~1"

set "BUILD_TARGET=%BUILD_TARGET%"
if not defined BUILD_TARGET set "BUILD_TARGET=sandbox"
if not "%~2"=="" set "BUILD_TARGET=%~2"
if /I "%BUILD_TARGET%"=="animation" set "BUILD_TARGET=animation_test"
if /I "%BUILD_TARGET%"=="anim_test" set "BUILD_TARGET=animation_test"

set "CMAKE_APP_FLAGS=-DSE_BUILD_APP_SANDBOX=ON -DSE_BUILD_APP_ANIMATION_TEST=OFF"
if /I "%BUILD_TARGET%"=="animation_test" (
    set "CMAKE_APP_FLAGS=-DSE_BUILD_APP_SANDBOX=OFF -DSE_BUILD_APP_ANIMATION_TEST=ON"
) else if /I "%BUILD_TARGET%"=="all" (
    set "CMAKE_APP_FLAGS=-DSE_BUILD_APP_SANDBOX=ON -DSE_BUILD_APP_ANIMATION_TEST=ON"
) else if /I "%BUILD_TARGET%"=="sandbox" (
    set "CMAKE_APP_FLAGS=-DSE_BUILD_APP_SANDBOX=ON -DSE_BUILD_APP_ANIMATION_TEST=OFF"
) else (
    rem Unknown custom target: keep known apps enabled so generated targets stay available.
    set "CMAKE_APP_FLAGS=-DSE_BUILD_APP_SANDBOX=ON -DSE_BUILD_APP_ANIMATION_TEST=ON"
)

set "FILAMENT_DIR=%PROJECT_ROOT%\engine\third_party\filament"

echo === Monster Engine Build ===
echo   Generator: %GENERATOR%
echo   Config:    %BUILD_TYPE%
echo   Target:    %BUILD_TARGET%
echo   AppFlags:  %CMAKE_APP_FLAGS%
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

rem ── Auto-detect latest MSVC toolset ──────────────────────────────────
rem The Filament SDK is built with a recent MSVC. If multiple toolsets
rem are installed, CMake may pick an older one whose STL is missing
rem vectorized algorithm symbols (__std_find_end_1, etc.), causing
rem linker errors. Force the latest installed toolset.
set "TOOLSET_FLAG="
set "LATEST_VER="
for /d %%D in ("%ProgramFiles%\Microsoft Visual Studio\2022\*") do (
    for /d %%V in ("%%D\VC\Tools\MSVC\*") do (
        set "LATEST_VER=%%~nxV"
    )
)
if defined LATEST_VER (
    set "TOOLSET_FLAG=-T version=%LATEST_VER%"
    echo Using MSVC toolset: %LATEST_VER%
)

cmake -S "%PROJECT_ROOT%" -B "%BUILD_DIR%" -G "%GENERATOR%" ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
      %CMAKE_APP_FLAGS% ^
      %TOOLSET_FLAG% ^
      %SCCACHE_FLAGS%
if errorlevel 1 (
    echo ERROR: CMake configuration failed.
    exit /b %errorlevel%
)

if /I not "%BUILD_TARGET%"=="all" (
    dir /s /b "%BUILD_DIR%\*.vcxproj" | findstr /I /R /C:"\\%BUILD_TARGET%\.vcxproj$" >nul
    if errorlevel 1 (
        echo ERROR: Target "%BUILD_TARGET%" was not generated in this build directory.
        echo HINT: if this is a new app target, ensure it is added in the root CMakeLists.txt.
        echo HINT: known app targets for this script are: sandbox, animation_test, all.
        exit /b 1
    )
)

set "TARGET_ARGS="
if /I not "%BUILD_TARGET%"=="all" (
    set "TARGET_ARGS=--target %BUILD_TARGET%"
)

cmake --build "%BUILD_DIR%" --config "%BUILD_TYPE%" %TARGET_ARGS% --parallel
if errorlevel 1 (
    echo ERROR: Build failed.
    if /I not "%BUILD_TARGET%"=="all" (
        echo HINT: check if target "%BUILD_TARGET%" exists and is enabled in CMake configure step.
    )
    exit /b %errorlevel%
)

echo.
echo Build completed successfully.
if /I "%BUILD_TARGET%"=="all" (
    echo Built target set: all
) else (
    set "TARGET_EXE=%BUILD_DIR%\apps\%BUILD_TARGET%\%BUILD_TYPE%\%BUILD_TARGET%.exe"
    if exist "%TARGET_EXE%" (
        echo Executable: "%TARGET_EXE%"
    ) else (
        echo WARN: Executable not found at expected location:
        echo       "%TARGET_EXE%"
    )
)
echo.
exit /b 0

:usage
echo Usage:
echo   scripts\build.bat [config] [target]
echo.
echo Examples:
echo   scripts\build.bat
echo   scripts\build.bat Debug sandbox
echo   scripts\build.bat RelWithDebInfo animation_test
echo   scripts\build.bat RelWithDebInfo animation
echo   scripts\build.bat RelWithDebInfo all
echo.
echo Environment override:
echo   set BUILD_TARGET=animation_test ^&^& scripts\build.bat
echo   set BUILD_TYPE=Debug ^&^& scripts\build.bat
exit /b 0
