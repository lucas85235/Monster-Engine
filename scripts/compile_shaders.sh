#!/bin/bash
# Compile unified .shader files to SPIR-V
# Parses #shader vertex and #shader fragment sections

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SHADER_DIR="$PROJECT_ROOT/assets/shaders"
OUTPUT_DIR="$SHADER_DIR/spirv"
TEMP_DIR="/tmp/shader_compile_$$"

mkdir -p "$OUTPUT_DIR"
mkdir -p "$TEMP_DIR"

# Cleanup on exit
trap "rm -rf $TEMP_DIR" EXIT

if ! command -v glslangValidator &> /dev/null; then
    echo "Error: glslangValidator not found. Install with:"
    echo "  sudo apt install glslang-tools"
    exit 1
fi

echo "Compiling unified shaders from $SHADER_DIR"

# Function to extract shader section from .shader file
extract_shader_section() {
    local input="$1"
    local section="$2"  # "vertex" or "fragment"
    local output="$3"
    
    awk -v section="$section" '
        BEGIN { in_section = 0 }
        /^#shader/ {
            if ($2 == section) {
                in_section = 1
                next
            } else {
                in_section = 0
            }
        }
        in_section { print }
    ' "$input" > "$output"
}

compile_unified_shader() {
    local shader_file="$1"
    local basename=$(basename "$shader_file" .shader)
    
    echo "  Processing: $basename.shader"
    
    # Extract vertex shader
    local vert_temp="$TEMP_DIR/${basename}.vert"
    extract_shader_section "$shader_file" "vertex" "$vert_temp"
    
    if [ -s "$vert_temp" ]; then
        echo "    Compiling vertex stage..."
        glslangValidator -V "$vert_temp" -o "$OUTPUT_DIR/${basename}.vert.spv" -S vert
        if [ $? -ne 0 ]; then
            echo "    FAILED: vertex shader"
            return 1
        fi
    else
        echo "    WARNING: No vertex shader section found"
    fi
    
    # Extract fragment shader
    local frag_temp="$TEMP_DIR/${basename}.frag"
    extract_shader_section "$shader_file" "fragment" "$frag_temp"
    
    if [ -s "$frag_temp" ]; then
        echo "    Compiling fragment stage..."
        glslangValidator -V "$frag_temp" -o "$OUTPUT_DIR/${basename}.frag.spv" -S frag
        if [ $? -ne 0 ]; then
            echo "    FAILED: fragment shader"
            return 1
        fi
    else
        echo "    WARNING: No fragment shader section found"
    fi
    
    echo "    SUCCESS"
    return 0
}

# Compile all .shader files
for shader in "$SHADER_DIR"/*.shader; do
    if [ -f "$shader" ]; then
        compile_unified_shader "$shader"
    fi
done

# Also compile standalone .vert and .frag files (for compatibility)
for shader in "$SHADER_DIR"/*.vert; do
    if [ -f "$shader" ]; then
        basename=$(basename "$shader" .vert)
        echo "  Compiling standalone: ${basename}.vert"
        glslangValidator -V "$shader" -o "$OUTPUT_DIR/${basename}.vert.spv" -S vert
    fi
done

for shader in "$SHADER_DIR"/*.frag; do
    if [ -f "$shader" ]; then
        basename=$(basename "$shader" .frag)
        echo "  Compiling standalone: ${basename}.frag"
        glslangValidator -V "$shader" -o "$OUTPUT_DIR/${basename}.frag.spv" -S frag
    fi
done

echo ""
echo "Shader compilation complete!"
echo "Output directory: $OUTPUT_DIR"
echo "SPIR-V files:"
ls -lh "$OUTPUT_DIR"/*.spv 2>/dev/null || echo "  (none generated)"
