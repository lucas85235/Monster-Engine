#!/bin/bash

echo "========================================"
echo "  Map Editor - Run Script (Linux)"
echo "========================================"
echo

# Navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
cd "$PROJECT_ROOT"

echo "Starting Map Editor from: $PROJECT_ROOT"
echo

# Check if executable exists
if [ ! -f "build/tools/map_editor/map_editor" ]; then
    echo "ERROR: map_editor not found!"
    echo "Please run build_linux.sh first."
    exit 1
fi

# Run the editor from project root (so assets are found)
./build/tools/map_editor/map_editor

echo
echo "Map Editor closed."
