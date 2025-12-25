#!/bin/bash

echo "========================================"
echo "  UI Editor - Run Script (Linux)"
echo "========================================"
echo

# Navigate to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../../.."

echo "Starting UI Editor from: $(pwd)"
echo

# Check if executable exists
if [ ! -f "build/tools/ui_editor/ui_editor" ]; then
    echo "ERROR: ui_editor not found!"
    echo "Please run build_linux.sh first."
    exit 1
fi

# Run the editor from project root (so assets are found)
./build/tools/ui_editor/ui_editor

echo
echo "UI Editor closed."
