@echo off
setlocal enabledelayedexpansion

:: Enable ANSI color support on Windows 10+
for /f "tokens=2 delims=[]" %%a in ('ver') do set "winver=%%a"
reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 1 /f >nul 2>&1

:: Color codes for Windows console (ESC character)
for /f %%a in ('echo prompt $E ^| cmd') do set "ESC=%%a"
set "C_RESET=%ESC%[0m"
set "C_CYAN=%ESC%[96m"
set "C_GREEN=%ESC%[92m"
set "C_YELLOW=%ESC%[93m"
set "C_RED=%ESC%[91m"
set "C_BLUE=%ESC%[94m"
set "C_MAGENTA=%ESC%[95m"
set "C_BOLD=%ESC%[1m"

:: ===========================================
:: Build and Run Script - SimpleEngine
:: ===========================================

:: Navigate to project root (parent folder of Scripts)
cd /d "%~dp0\.."

echo.
echo %C_CYAN%========================================
echo    SIMPLE ENGINE - BUILD SCRIPT
echo ========================================%C_RESET%
echo.

:: ===========================================
:: STEP 1: Generate CMake Project
:: ===========================================
echo %C_BLUE%[STEP 1/4]%C_RESET% %C_YELLOW%Generating CMake project...%C_RESET%
cmake -S . -B build -G "Visual Studio 17 2022" ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
      -DCMAKE_C_COMPILER_LAUNCHER=sccache ^
      -DCMAKE_CXX_COMPILER_LAUNCHER=sccache

if %errorlevel% neq 0 (
    echo %C_RED%[ERROR] CMake generation failed!%C_RESET%
    pause
    exit /b %errorlevel%
)
echo %C_GREEN%[OK] CMake project generated successfully%C_RESET%
echo.

:: ===========================================
:: STEP 2: Setup Visual Studio Environment
:: ===========================================
echo %C_BLUE%[STEP 2/4]%C_RESET% %C_YELLOW%Setting up Visual Studio 2022 environment...%C_RESET%

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if %errorlevel% neq 0 (
    echo %C_YELLOW%[WARN] VS 2022 Community not found, trying Professional...%C_RESET%
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    
    if %errorlevel% neq 0 (
        echo %C_RED%[ERROR] Visual Studio 2022 not found!%C_RESET%
        echo Please make sure Visual Studio 2022 is installed.
        pause
        exit /b 1
    )
)

echo %C_GREEN%[OK] Environment configured successfully%C_RESET%
echo.

:: ===========================================
:: STEP 3: Build Project
:: ===========================================
echo %C_BLUE%[STEP 3/4]%C_RESET% %C_YELLOW%Building project (Debug x64)...%C_RESET%
echo.
msbuild "build\SimpleEngineProject.sln" /p:Configuration=Debug /p:Platform=x64 /m /nologo /verbosity:minimal

if %errorlevel% neq 0 (
    echo.
    echo %C_RED%[ERROR] Build failed!%C_RESET%
    pause
    exit /b %errorlevel%
)

echo.
echo %C_GREEN%[OK] Build completed successfully%C_RESET%
echo.

:: ===========================================
:: STEP 4: Find and Execute
:: ===========================================
echo %C_BLUE%[STEP 4/4]%C_RESET% %C_YELLOW%Looking for executable...%C_RESET%

set "EXE_PATH="
for /r "build" %%F in (*.exe) do (
    set "EXE_PATH=%%F"
    goto :found_exe
)

:found_exe
if not defined EXE_PATH (
    echo %C_RED%[ERROR] Executable not found in build folder!%C_RESET%
    echo Make sure the project generates an .exe file
    pause
    exit /b 1
)

echo %C_GREEN%[OK] Found: %C_CYAN%!EXE_PATH!%C_RESET%
echo.
echo %C_MAGENTA%========================================
echo    LAUNCHING SIMPLE ENGINE
echo ========================================%C_RESET%
echo.

start "" "!EXE_PATH!"

if %errorlevel% neq 0 (
    echo %C_RED%[ERROR] Failed to execute the program!%C_RESET%
    pause
    exit /b %errorlevel%
)

echo %C_GREEN%[SUCCESS] Program started successfully!%C_RESET%
echo.
echo Press any key to close this window...
pause >nul
exit /b 0