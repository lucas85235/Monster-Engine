#!/usr/bin/env bash
set -e

# Atualiza lista de pacotes
sudo apt update

sudo apt install -y pkg-config

# Instala Clang 17 e libc++ (para evitar bugs do libstdc++ do GCC 15 com C++23)
sudo apt install -y \
    clang-17 \
    libc++-17-dev \
    libc++abi-17-dev \
    ninja-build \
    cmake

# Instala pacotes de OpenGL e utilitários
sudo apt install -y \
    mesa-common-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    freeglut3-dev \
    mesa-utils

# Vulkan SDK
sudo apt install -y \
    libvulkan-dev \
    vulkan-tools \
    vulkan-validationlayers

# Wayland (wayland-scanner)
sudo apt install -y \
    libwayland-dev \
    wayland-protocols

# XKB (xkbcommon >= 0.5.0)
sudo apt install -y \
    libxkbcommon-dev

# Xrandr
sudo apt install -y \
    libxrandr-dev

# Xinerama
sudo apt install -y \
    libxinerama-dev

# Xcursor
sudo apt install -y \
    libxcursor-dev

# XInput (libXi)
sudo apt install -y \
    libxi-dev

# zlib (necessário para Assimp)
sudo apt install -y \
    zlib1g-dev

echo
echo "Setup complete!"
echo "Build is configured to use clang++-17 with libc++ for C++23 compatibility."