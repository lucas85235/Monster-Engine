#!/bin/bash

# Diretório de build
BUILD_DIR="build"

# Se não existir, cria
if [ ! -d "$BUILD_DIR" ]; then
    mkdir $BUILD_DIR
fi

# Gera os arquivos de build com CMake
cmake -S . -B $BUILD_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -G Ninja

# Compila o projeto
cmake --build $BUILD_DIR

if [ $? -ne 0 ]; then
    echo "Build failed. Aborting."
    exit 1
fi


# Executa o binário
./$BUILD_DIR/apps/sandbox/sandbox
