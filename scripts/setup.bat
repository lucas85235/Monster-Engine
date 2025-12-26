@echo off
setlocal enabledelayedexpansion

:: ===========================================================
:: SIMPLE ENGINE - WINDOWS SETUP SCRIPT
:: This script checks and helps configure the required packages
::
:: Equivalent to setup.sh for Linux, but for Windows
:: Run this on a clean system before building the project
:: ===========================================================

echo.
echo ========================================
echo    SIMPLE ENGINE - SETUP SCRIPT
echo ========================================
echo.

set "ALL_OK=1"
set "WARNINGS=0"

:: ===========================================================
:: CHECK VISUAL STUDIO 2022 (equivalent to Clang/GCC on Linux)
:: ===========================================================
echo [CHECK] Visual Studio 2022 (C++ compiler)...

set "VS_FOUND=0"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\" (
    set "VS_FOUND=1"
    echo    [OK] Visual Studio 2022 Community found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\" (
    set "VS_FOUND=1"
    echo    [OK] Visual Studio 2022 Professional found
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\" (
    set "VS_FOUND=1"
    echo    [OK] Visual Studio 2022 Enterprise found
)

if "!VS_FOUND!"=="0" (
    set "ALL_OK=0"
    echo    [MISSING] Visual Studio 2022 not found
    echo              Download from: https://visualstudio.microsoft.com/vs/
    echo              Required workloads:
    echo                - Desktop development with C++
    echo                - Windows SDK
)
echo.

:: ===========================================================
:: CHECK CMAKE (equivalent to cmake on Linux)
:: ===========================================================
echo [CHECK] CMake (build system generator)...
where cmake >nul 2>&1
if errorlevel 1 (
    set "ALL_OK=0"
    echo    [MISSING] CMake not found in PATH
    echo              Download from: https://cmake.org/download/
    echo              Or: winget install Kitware.CMake
) else (
    for /f "tokens=3" %%v in ('cmake --version 2^>nul ^| findstr /i "version"') do (
        echo    [OK] CMake %%v found
    )
)
echo.

:: ===========================================================
:: CHECK VULKAN SDK (equivalent to libvulkan-dev on Linux)
:: ===========================================================
echo [CHECK] Vulkan SDK...

set "VULKAN_OK=0"
if defined VULKAN_SDK (
    if exist "%VULKAN_SDK%\Include\vulkan\vulkan.h" (
        set "VULKAN_OK=1"
        echo    [OK] Vulkan SDK found at: %VULKAN_SDK%
        
        :: Check SPIRV-Cross in Vulkan SDK
        if exist "%VULKAN_SDK%\Include\spirv_cross\spirv_glsl.hpp" (
            echo    [OK] SPIRV-Cross found in Vulkan SDK
        ) else (
            set "WARNINGS=1"
            echo    [WARN] SPIRV-Cross not found in Vulkan SDK
            echo              Shader cross-compilation will be disabled
        )
        
        :: Check glslangValidator for shader compilation
        if exist "%VULKAN_SDK%\Bin\glslangValidator.exe" (
            echo    [OK] glslangValidator found for shader compilation
        ) else (
            set "WARNINGS=1"
            echo    [WARN] glslangValidator not found
        )
    ) else (
        echo    [ERROR] VULKAN_SDK set but vulkan.h not found
        set "ALL_OK=0"
    )
) else (
    set "VK_FOUND=0"
    for /d %%d in ("C:\VulkanSDK\*") do (
        if exist "%%d\Include\vulkan\vulkan.h" (
            set "VK_FOUND=1"
            set "WARNINGS=1"
            echo    [WARN] Vulkan SDK found at %%d but VULKAN_SDK not set
            echo              Run this command then restart your terminal:
            echo              setx VULKAN_SDK "%%d"
        )
    )
    if "!VK_FOUND!"=="0" (
        set "ALL_OK=0"
        echo    [MISSING] Vulkan SDK not found
        echo              Download from: https://vulkan.lunarg.com/sdk/home
        echo              Or: winget install LunarG.VulkanSDK
        echo              Make sure to select SPIRV-Cross component during install
    )
)
echo.

:: ===========================================================
:: CHECK GIT
:: ===========================================================
echo [CHECK] Git (version control)...
where git >nul 2>&1
if errorlevel 1 (
    set "ALL_OK=0"
    echo    [MISSING] Git not found in PATH
    echo              Download from: https://git-scm.com/download/win
    echo              Or: winget install Git.Git
) else (
    echo    [OK] Git found
)
echo.

:: ===========================================================
:: CHECK NINJA (optional - equivalent to ninja-build on Linux)
:: ===========================================================
echo [CHECK] Ninja (optional - faster builds)...
where ninja >nul 2>&1
if errorlevel 1 (
    echo    [OPTIONAL] Ninja not found - using MSBuild instead
    echo              To install: winget install Ninja-build.Ninja
) else (
    echo    [OK] Ninja found
)
echo.

:: ===========================================================
:: CHECK SCCACHE (optional - faster compilation cache)
:: ===========================================================
echo [CHECK] sccache (optional - compilation cache)...
where sccache >nul 2>&1
if errorlevel 1 (
    echo    [OPTIONAL] sccache not found - recommended for faster rebuilds
    echo              To install: winget install Mozilla.sccache
) else (
    echo    [OK] sccache found
)
echo.

:: ===========================================================
:: CHECK WINDOWS SDK (comes with Visual Studio)
:: Contains OpenGL headers, DirectX, and system libraries
:: Equivalent to mesa-common-dev, libgl1-mesa-dev on Linux
:: ===========================================================
echo [CHECK] Windows SDK (OpenGL/DirectX headers)...
if exist "C:\Program Files (x86)\Windows Kits\10\Include" (
    echo    [OK] Windows SDK found
) else (
    set "WARNINGS=1"
    echo    [WARN] Windows SDK not detected at default location
    echo              Usually installed with Visual Studio
)
echo.

:: ===========================================================
:: SUMMARY
:: ===========================================================
echo ========================================
echo    SETUP SUMMARY
echo ========================================
echo.

if "!ALL_OK!"=="1" (
    if "!WARNINGS!"=="1" (
        echo Some optional components have warnings - see above.
        echo.
    )
    echo All required dependencies are installed!
    echo.
    echo == Windows vs Linux Dependency Mapping ==
    echo   Visual Studio 2022  = Clang-17 / GCC
    echo   CMake               = CMake
    echo   Vulkan SDK          = libvulkan-dev
    echo   SPIRV-Cross         = spirv-cross ^(in Vulkan SDK^)
    echo   Windows SDK         = mesa-common-dev, libgl1-mesa-dev
    echo   MSBuild/Ninja       = ninja-build
    echo.
    echo Note: On Windows, OpenGL, windowing ^(equivalent to X11/Wayland^),
    echo       and system libraries are included in Visual Studio/Windows SDK.
    echo.
    echo Next steps:
    echo   1. Run: scripts\generate_visual_studio_files_and_build.bat
    echo   2. Or open the generated build\SimpleEngineProject.sln in Visual Studio
) else (
    echo Some required dependencies are missing!
    echo Please install them and run this script again.
    echo.
    echo Quick install commands ^(run in PowerShell as Admin^):
    echo   winget install Kitware.CMake
    echo   winget install LunarG.VulkanSDK
    echo   winget install Git.Git
)
echo.

echo Press any key to exit...
pause >nul
exit /b 0
