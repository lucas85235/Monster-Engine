#!/bin/bash

# Diretório de build
BUILD_DIR="build"

# Navega para a raiz do projeto (três níveis acima da pasta scripts)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

echo "========================================"
echo "  Map Editor - Build Script (Linux)"
echo "========================================"
echo "Project root: $PROJECT_ROOT"
echo

# Se não existir, cria
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

# Gera os arquivos de build com CMake usando clang-17 e libc++
cmake -S . -B $BUILD_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja \
    -DCMAKE_C_COMPILER=clang-17 \
    -DCMAKE_CXX_COMPILER=clang++-17 \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++"

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

# Compila o projeto (apenas map_editor)
cmake --build $BUILD_DIR --target map_editor

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: Build failed!"
    exit 1
fi

echo
echo "========================================"
echo "  Build completed successfully!"
echo "========================================"
echo
echo "Executable: $BUILD_DIR/tools/map_editor/map_editor"
echo

# Executa o binário
./$BUILD_DIR/tools/map_editor/map_editor
