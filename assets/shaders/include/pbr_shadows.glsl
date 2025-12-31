// =============================================================================
// PBR Shadow Functions - Cascaded Shadow Maps
// =============================================================================
// Based on Filament shadow implementation with practical split scheme.
// =============================================================================

#ifndef PBR_SHADOWS_GLSL
#define PBR_SHADOWS_GLSL

#define CASCADE_COUNT 4

// Cascade uniforms
uniform sampler2DArray uShadowCascades;
uniform mat4 uCascadeMatrices[CASCADE_COUNT];
uniform float uCascadeSplits[CASCADE_COUNT];
uniform int uCascadeCount;

// Get cascade index based on view-space depth
int getCascadeIndex(float viewDepth) {
    for (int i = 0; i < CASCADE_COUNT; i++) {
        if (viewDepth < uCascadeSplits[i]) return i;
    }
    return CASCADE_COUNT - 1;
}

// PCF 3x3 filtering for a specific cascade
float shadowPCF(vec4 shadowCoord, int cascade, float bias) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowCascades, 0).xy);
    
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, float(cascade));
            float pcfDepth = texture(uShadowCascades, sampleCoord).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

// Calculate shadow with cascade selection
float calculateCascadedShadow(vec3 worldPos, float viewDepth, vec3 normal, vec3 lightDir) {
    int cascade = getCascadeIndex(viewDepth);
    
    vec4 shadowCoord = uCascadeMatrices[cascade] * vec4(worldPos, 1.0);
    
    // Bias based on slope and cascade
    float ndotl = max(dot(normal, lightDir), 0.0);
    float baseBias = 0.0005 * (1.0 + float(cascade) * 0.5);  // Increase bias for far cascades
    float slopeBias = baseBias * (1.0 - ndotl);
    float bias = max(slopeBias, baseBias * 0.1);
    
    float shadow = shadowPCF(shadowCoord, cascade, bias);
    
    // Fade out shadow at cascade edges
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    float fadeStart = 0.85;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    float edgeFade = maxCoord > fadeStart ? 1.0 - smoothstep(fadeStart, 1.0, maxCoord) : 1.0;
    
    return shadow * edgeFade;
}

// Debug: Get cascade color for visualization
vec3 getCascadeDebugColor(int cascade) {
    vec3 colors[4] = vec3[4](
        vec3(1.0, 0.0, 0.0),  // Red - Near
        vec3(0.0, 1.0, 0.0),  // Green
        vec3(0.0, 0.0, 1.0),  // Blue
        vec3(1.0, 1.0, 0.0)   // Yellow - Far
    );
    return colors[clamp(cascade, 0, 3)];
}

#endif // PBR_SHADOWS_GLSL
