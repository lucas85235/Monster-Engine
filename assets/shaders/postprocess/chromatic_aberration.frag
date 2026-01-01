#version 460 core
// =============================================================================
// Chromatic Aberration
// =============================================================================
// Simulates lens imperfection by shifting RGB channels differently.

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;
uniform float uIntensity;     // Aberration strength (0.002 typical)
uniform vec2 uDirection;      // Direction of aberration (default: radial from center)

void main() {
    vec2 centered = vTexCoord - 0.5;
    float dist = length(centered);
    
    // Radial direction for aberration
    vec2 dir = normalize(centered) * uIntensity * dist;
    
    // Sample each channel with offset
    float r = texture(uSceneTexture, vTexCoord + dir).r;
    float g = texture(uSceneTexture, vTexCoord).g;
    float b = texture(uSceneTexture, vTexCoord - dir).b;
    
    FragColor = vec4(r, g, b, 1.0);
}
