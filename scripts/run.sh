#!/bin/bash
set -e

# Diretório de build
BUILD_DIR="build"

# Se não existir, cria
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

# Detecta a plataforma e configura o compilador apropriado
CMAKE_EXTRA_FLAGS=""

if [[ "$(uname)" == "Darwin" ]]; then
    # macOS — usa o Clang do sistema (Xcode) com libc++ (padrão no Mac)
    echo "Detected macOS — using system Clang"
    CMAKE_EXTRA_FLAGS="-DCMAKE_BUILD_TYPE=Debug"
else
    # Linux — usa clang-17 com libc++ (instalado via setup.sh)
    echo "Detected Linux — using clang-17 with libc++"
    CMAKE_EXTRA_FLAGS="-DCMAKE_C_COMPILER=clang-17 -DCMAKE_CXX_COMPILER=clang++-17 -DCMAKE_CXX_FLAGS=-stdlib=libc++"
fi

# Gera os arquivos de build com CMake
cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -G Ninja \
    $CMAKE_EXTRA_FLAGS

# Compila o projeto
cmake --build "$BUILD_DIR"

# Executa o binário
"./$BUILD_DIR/apps/sandbox/sandbox"
