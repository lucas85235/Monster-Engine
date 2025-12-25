#version 330 core

in vec3 v_WorldPos;
out vec4 FragColor;

uniform float uGridSize;      // Size of one grid cell
uniform float uGridFade;      // Distance at which grid starts to fade
uniform vec3 uGridColor;      // Color of grid lines
uniform vec3 uAxisXColor;     // Color of X axis (red by default)
uniform vec3 uAxisZColor;     // Color of Z axis (blue by default)

float getGrid(vec2 coord, float size) {
    vec2 grid = abs(fract(coord / size - 0.5) - 0.5) / fwidth(coord / size);
    return 1.0 - min(min(grid.x, grid.y), 1.0);
}

void main() {
    // Calculate distance from camera for fading
    float distFromCenter = length(v_WorldPos.xz);
    float fade = 1.0 - smoothstep(uGridFade * 0.5, uGridFade, distFromCenter);
    
    // Main grid
    float grid = getGrid(v_WorldPos.xz, uGridSize);
    
    // Thicker lines every 10 cells
    float grid10 = getGrid(v_WorldPos.xz, uGridSize * 10.0);
    grid = max(grid * 0.3, grid10 * 0.6);
    
    // Calculate axis lines (thicker)
    float axisLineWidth = 0.05;
    bool onXAxis = abs(v_WorldPos.z) < axisLineWidth;
    bool onZAxis = abs(v_WorldPos.x) < axisLineWidth;
    
    vec3 color = uGridColor;
    float alpha = grid * 0.5;
    
    // Color the axes
    if (onXAxis && !onZAxis) {
        color = uAxisXColor;
        alpha = 0.8;
    } else if (onZAxis && !onXAxis) {
        color = uAxisZColor;
        alpha = 0.8;
    } else if (onXAxis && onZAxis) {
        // Origin - blend colors
        color = mix(uAxisXColor, uAxisZColor, 0.5);
        alpha = 0.9;
    }
    
    // Apply fade
    alpha *= fade;
    
    // Discard nearly transparent fragments for performance
    if (alpha < 0.01) discard;
    
    FragColor = vec4(color, alpha);
}
