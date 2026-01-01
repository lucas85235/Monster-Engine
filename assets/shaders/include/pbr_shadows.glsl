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
}

// =============================================================================
// DPCF (Distance-based Percentage Closer Filtering) - Contact Hardening Shadows
// =============================================================================
// Myers, "Shadow of Cold War" - Softer shadows near blockers, sharper shadows further away

// Poisson disk samples for blocker search and PCF
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

// Random rotation based on screen position
mat2 getRandomRotation(vec2 screenPos) {
    float angle = fract(sin(dot(screenPos, vec2(12.9898, 78.233))) * 43758.5453) * 2.0 * 3.14159;
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, s, -s, c);
}

// Blocker search - find average depth of occluding surfaces
float findBlockerDistance(vec3 shadowCoords, int cascade, vec2 texelSize, float lightSize, out float avgBlockerDepth, out float numBlockers) {
    avgBlockerDepth = 0.0;
    numBlockers = 0.0;
    
    float searchRadius = lightSize * shadowCoords.z;
    mat2 rotation = getRandomRotation(gl_FragCoord.xy);
    
    for (int i = 0; i < 16; i++) {
        vec2 offset = rotation * poissonDisk[i] * texelSize * searchRadius;
        float sampleDepth = texture(uShadowCascades, vec3(shadowCoords.xy + offset, float(cascade))).r;
        
        if (sampleDepth < shadowCoords.z) {
            avgBlockerDepth += sampleDepth;
            numBlockers += 1.0;
        }
    }
    
    if (numBlockers > 0.0) {
        avgBlockerDepth /= numBlockers;
    }
    
    return numBlockers;
}

// DPCF shadow sampling with contact hardening
float shadowDPCF(vec4 shadowCoord, int cascade, float bias, float lightSize) {
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowCascades, 0).xy);
    
    // Step 1: Blocker search
    float avgBlockerDepth, numBlockers;
    findBlockerDistance(projCoords, cascade, texelSize, lightSize, avgBlockerDepth, numBlockers);
    
    if (numBlockers < 1.0) {
        return 0.0;  // No blockers found - fully lit
    }
    
    // Step 2: Estimate penumbra size
    float penumbraRatio = (projCoords.z - avgBlockerDepth) / avgBlockerDepth;
    float filterRadius = penumbraRatio * lightSize * 20.0;  // Scale for visual appearance
    filterRadius = clamp(filterRadius, 1.0, 10.0);
    
    // Step 3: PCF with variable kernel size
    float shadow = 0.0;
    mat2 rotation = getRandomRotation(gl_FragCoord.xy);
    
    for (int i = 0; i < 16; i++) {
        vec2 offset = rotation * poissonDisk[i] * texelSize * filterRadius;
        float sampleDepth = texture(uShadowCascades, vec3(projCoords.xy + offset, float(cascade))).r;
        shadow += projCoords.z - bias > sampleDepth ? 1.0 : 0.0;
    }
    
    return shadow / 16.0;
}

// Calculate cascaded shadow with DPCF option
float calculateCascadedShadowDPCF(vec3 worldPos, float viewDepth, vec3 normal, vec3 lightDir, float lightSize, bool useDPCF) {
    int cascade = getCascadeIndex(viewDepth);
    vec4 shadowCoord = uCascadeMatrices[cascade] * vec4(worldPos, 1.0);
    
    // Compute bias
    float ndotl = max(dot(normal, lightDir), 0.0);
    float baseBias = 0.0005 * (1.0 + float(cascade) * 0.5);
    float slopeBias = baseBias * (1.0 - ndotl);
    float bias = max(slopeBias, baseBias * 0.1);
    
    float shadow;
    if (useDPCF && lightSize > 0.0) {
        shadow = shadowDPCF(shadowCoord, cascade, bias, lightSize);
    } else {
        shadow = shadowPCF(shadowCoord, cascade, bias);
    }
    
    // Fade at cascade edges
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    float fadeStart = 0.85;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    float edgeFade = maxCoord > fadeStart ? 1.0 - smoothstep(fadeStart, 1.0, maxCoord) : 1.0;
    
    return shadow * edgeFade;
}

#endif // PBR_SHADOWS_GLSL
