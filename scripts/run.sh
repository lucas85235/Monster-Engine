#!/bin/bash

# Diretório de build
BUILD_DIR="build"

# Se não existir, cria
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

# Gera os arquivos de build com CMake usando clang-17 e libc++
cmake -S . -B $BUILD_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja \
    -DCMAKE_C_COMPILER=clang-17 \
    -DCMAKE_CXX_COMPILER=clang++-17 \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++"

# Compila o projeto
cmake --build $BUILD_DIR

if [ $? -ne 0 ]; then
    echo "Build failed. Aborting."
    exit 1
fi


# Executa o binário
./$BUILD_DIR/apps/sandbox/sandbox
