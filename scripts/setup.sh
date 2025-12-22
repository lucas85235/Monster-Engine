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
# Vulkan (rendering)
# ============================================
echo "[3/9] Installing Vulkan SDK..."
sudo apt install -y \
    libvulkan-dev \
    libvulkan1 \
    vulkan-tools

# ============================================
# SPIRV-Cross (shader cross-compilation)
# ============================================
echo "[4/9] Installing SPIRV-Cross (optional)..."
sudo apt install -y \
    libspirv-cross-c-shared-dev \
    spirv-tools \
    glslang-tools || echo "SPIRV-Cross not available - SPIR-V shaders will be disabled"

# ============================================
# Wayland (wayland support)
# ============================================
echo "[5/9] Installing Wayland..."
sudo apt install -y \
    libwayland-dev \
    wayland-protocols

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
# Audio (optional)
# ============================================
echo "[8/9] Installing audio libraries..."
sudo apt install -y \
    libasound2-dev \
    libpulse-dev || echo "Audio libraries not installed"

# ============================================
# Debug Tools (optional)
# ============================================
echo "[9/9] Installing debug tools..."
sudo apt install -y \
    gdb \
    valgrind || echo "Debug tools not installed"

echo
echo "=================================================="
echo "  Setup complete!"
echo "=================================================="
echo
echo "To build the project, run:"
echo "  ./scripts/run.sh"
echo