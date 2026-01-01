#version 460 core
// =============================================================================
// Vignette Effect
// =============================================================================

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;
uniform float uIntensity;     // 0 = off, 1 = full
uniform float uSoftness;      // Falloff curve (0.1 = hard, 0.5 = soft)
uniform float uRoundness;     // 1.0 = circular, 0.0 = rectangular

void main() {
    vec3 color = texture(uSceneTexture, vTexCoord).rgb;
    
    // Compute distance from center
    vec2 centered = vTexCoord - 0.5;
    
    // Apply roundness (blend between max and length for shape)
    float dist;
    if (uRoundness >= 0.99) {
        dist = length(centered);
    } else {
        float maxDist = max(abs(centered.x), abs(centered.y));
        float lenDist = length(centered);
        dist = mix(maxDist, lenDist, uRoundness);
    }
    
    // Apply vignette
    float vignette = 1.0 - smoothstep(uSoftness, uSoftness + 0.5, dist);
    vignette = mix(1.0, vignette, uIntensity);
    
    FragColor = vec4(color * vignette, 1.0);
}
