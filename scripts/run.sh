#!/bin/bash

# Diretório de build
BUILD_DIR="build"

# Build type (Debug or Release)
BUILD_TYPE="${1:-Release}"

# Valida o build type
if [ "$BUILD_TYPE" != "Debug" ] && [ "$BUILD_TYPE" != "Release" ]; then
    echo "Usage: $0 [Debug|Release]"
    echo "Default: Release"
    exit 1
fi

echo "Building in $BUILD_TYPE mode..."

# Se não existir, cria
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

# Gera os arquivos de build com CMake
cmake -S . -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja

# Compila o projeto
cmake --build $BUILD_DIR

if [ $? -ne 0 ]; then
    echo "Build failed. Aborting."
    exit 1
fi

# Executa o binário
./$BUILD_DIR/apps/sandbox/sandbox
