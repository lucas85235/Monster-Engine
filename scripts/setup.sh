#!/usr/bin/env bash
set -e

echo "=================================================="
echo "  Monster Engine - Setup Script"
echo "  Installing all system dependencies..."
echo "=================================================="
echo

# Update package list
sudo apt update

# ============================================
# Essential Build Tools
# ============================================
echo "[1/9] Installing build tools..."
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    pkg-config

# ============================================
# OpenGL and Mesa (rendering)
# ============================================
echo "[2/9] Installing OpenGL/Mesa..."
sudo apt install -y \
    mesa-common-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    freeglut3-dev \
    mesa-utils

# ============================================
# Vulkan SDK and Validation Layers
# ============================================
echo "[3/9] Installing Vulkan SDK..."
sudo apt install -y \
    vulkan-headers \
    libvulkan-dev \
    libvulkan1 \
    vulkan-tools \
    vulkan-validationlayers \
    vulkan-validationlayers-dev \
    vulkan-utility-libraries-dev \
    libvulkan-volk-dev || echo "Some Vulkan packages may not be available"

# ============================================
# SPIRV-Cross and Shader Tools
# Required for SPIR-V to GLSL cross-compilation
# ============================================
echo "[4/9] Installing SPIRV-Cross and shader tools..."
sudo apt install -y \
    libspirv-cross-c-shared-dev \
    spirv-cross \
    spirv-tools \
    spirv-headers \
    glslang-tools \
    glslang-dev \
    shaderc || echo "Some shader tools not available - installing alternatives..."

# Verify Vulkan headers are installed
if [ ! -f "/usr/include/vulkan/vulkan.h" ]; then
    echo "WARNING: Vulkan headers not found. Attempting to install from LunarG PPA..."
    UBUNTU_CODENAME=$(lsb_release -cs)
    wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc > /dev/null
    # Try current Ubuntu version first, fallback to jammy if not available
    if wget -q --spider "https://packages.lunarg.com/vulkan/lunarg-vulkan-${UBUNTU_CODENAME}.list"; then
        sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan.list "https://packages.lunarg.com/vulkan/lunarg-vulkan-${UBUNTU_CODENAME}.list"
    else
        echo "Using jammy repository as fallback..."
        sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
    fi
    sudo apt update
    sudo apt install -y vulkan-headers vulkan-sdk || echo "Vulkan SDK installation failed - manual install may be required"
fi

# Alternative: Install shader tools if missing
if ! command -v glslangValidator &> /dev/null || ! command -v glslc &> /dev/null; then
    echo "Installing additional shader compiler tools..."
    sudo apt install -y shaderc || echo "Shader compiler tools not fully available"
fi

# ============================================
# Wayland (wayland support)
# ============================================
echo "[5/9] Installing Wayland..."
sudo apt install -y \
    libwayland-dev \
    wayland-protocols \
    libdecor-0-0 \
    libdecor-0-dev \
    libdecor-0-plugin-1-cairo

# ============================================
# X11 and XKB (X11 support)
# ============================================
echo "[6/9] Installing X11/XKB..."
sudo apt install -y \
    libxkbcommon-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libx11-dev \
    libxext-dev

# ============================================
# Compression Libraries
# ============================================
echo "[7/9] Installing compression libraries..."
sudo apt install -y \
    zlib1g-dev

# ============================================
# Model Loading (Assimp)
# ============================================
echo "[8/9] Installing model loading libraries..."
sudo apt install -y \
    libassimp-dev \
    assimp-utils || echo "Assimp not installed - will use bundled version"

# ============================================
# Audio (optional)
# ============================================
echo "[9/10] Installing audio libraries..."
sudo apt install -y \
    libasound2-dev \
    libpulse-dev || echo "Audio libraries not installed"

# ============================================
# Debug Tools (optional)
# ============================================
echo "[10/10] Installing debug tools..."
sudo apt install -y \
    gdb \
    valgrind || echo "Debug tools not installed"

echo
echo "=================================================="
echo "  Setup complete!"
echo "=================================================="
echo
echo "Verifying critical dependencies..."
echo -n "  • Vulkan headers: "
if [ -f "/usr/include/vulkan/vulkan.h" ]; then
    echo "✓ Found"
else
    echo "✗ Missing (CMake will fail)"
    echo "    Run: sudo apt install vulkan-headers"
fi

echo -n "  • Vulkan library: "
if pkg-config --exists vulkan; then
    echo "✓ Found ($(pkg-config --modversion vulkan))"
else
    echo "✗ Missing"
fi

echo -n "  • OpenGL: "
if pkg-config --exists gl; then
    echo "✓ Found"
else
    echo "✗ Missing"
fi

echo -n "  • Assimp: "
if pkg-config --exists assimp; then
    echo "✓ Found ($(pkg-config --modversion assimp))"
else
    echo "⚠ Not found (project may use bundled version)"
fi

echo
echo "To build the project, run:"
echo "  ./scripts/run.sh"
echo
