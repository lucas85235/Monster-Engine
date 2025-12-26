#!/bin/bash

echo "========================================"
echo "  UI Editor - Build Script (Linux)"
echo "========================================"
echo

# Navigate to project root (two levels up from scripts folder)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../../.."

echo "[1/3] Project root: $(pwd)"
echo

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "[2/3] Creating build directory and generating project files..."
    cmake -B build -DCMAKE_BUILD_TYPE=Debug
else
    echo "[2/3] Regenerating CMake project files..."
    cmake -B build
fi

if [ $? -ne 0 ]; then
    echo
    echo "ERROR: CMake configuration failed!"
    exit 1
fi

echo
echo "[3/3] Building ui_editor (Debug)..."
cmake --build build --target ui_editor

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
echo "Executable: build/tools/ui_editor/ui_editor"
echo
