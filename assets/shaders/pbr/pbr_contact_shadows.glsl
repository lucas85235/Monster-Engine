// =============================================================================
// PBR Contact Shadows - Screen-Space Raymarching
// =============================================================================
// Based on Filament's contact shadow implementation.
// Adds high-frequency shadow detail where shadow maps have insufficient resolution.
// Requires: pbr_common.glsl (for interleavedGradientNoise)
// =============================================================================

#ifndef PBR_CONTACT_SHADOWS_GLSL
#define PBR_CONTACT_SHADOWS_GLSL

// Linearize depth from depth buffer (reverse-Z compatible)
float linearizeDepth(float depth, float near, float far) {
    return near * far / (far - depth * (far - near));
}

// Screen-space contact shadow
// Returns 1.0 if occluded (in shadow), 0.0 if fully lit
float screenSpaceContactShadow(
    sampler2D depthBuffer,
    vec3 worldPos,
    vec3 lightDir,
    mat4 viewMatrix,
    mat4 projMatrix,
    vec2 screenSize,
    float cameraNear,
    float cameraFar,
    int stepCount,
    float maxDistance
) {
    // Transform world position to view space
    vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
    
    // Ray direction in view space
    vec3 viewLightDir = normalize((viewMatrix * vec4(lightDir, 0.0)).xyz);
    
    // Ray end point in view space
    vec3 rayEnd = viewPos.xyz + viewLightDir * maxDistance;
    
    // Project start and end to screen space
    vec4 startClip = projMatrix * viewPos;
    vec4 endClip = projMatrix * vec4(rayEnd, 1.0);
    
    vec3 startScreen = startClip.xyz / startClip.w;
    vec3 endScreen = endClip.xyz / endClip.w;
    
    // Convert from NDC [-1,1] to screen UV [0,1]
    startScreen.xy = startScreen.xy * 0.5 + 0.5;
    endScreen.xy = endScreen.xy * 0.5 + 0.5;
    
    // Ray in screen space
    vec3 rayScreen = endScreen - startScreen;
    
    // Step size
    float dt = 1.0 / float(stepCount);
    
    // Jitter start position to reduce banding
    float dither = interleavedGradientNoise(worldPos.xy * screenSize) - 0.5;
    float t = dt * dither + dt;
    
    // Tolerance for depth comparison (scales with distance)
    float baseZ = abs(viewPos.z);
    float tolerance = abs(rayEnd.z - viewPos.z) * dt * 2.0;
    
    // Raymarch along the screen-space ray
    for (int i = 0; i < stepCount; i++, t += dt) {
        vec3 samplePos = startScreen + rayScreen * t;
        
        // Early out if outside screen bounds
        if (samplePos.x < 0.0 || samplePos.x > 1.0 || 
            samplePos.y < 0.0 || samplePos.y > 1.0) {
            break;
        }
        
        // Sample depth buffer
        float sceneDepth = texture(depthBuffer, samplePos.xy).r;
        
        // Compare depths
        float depthDiff = samplePos.z - sceneDepth;
        
        // Check if we hit something (positive diff means we're behind scene geometry)
        if (depthDiff > 0.0 && depthDiff < tolerance) {
            // Soft falloff based on how far along the ray we are
            float falloff = 1.0 - t;
            return falloff;
        }
    }
    
    return 0.0;
}

// Simplified contact shadow for directional lights
// Uses view-space depth directly for better precision
float contactShadowDirectional(
    sampler2D depthBuffer,
    vec3 worldPos,
    vec3 lightDir,
    mat4 viewMatrix,
    mat4 projMatrix,
    vec2 screenSize,
    int stepCount,
    float maxDistance
) {
    // Transform world position to clip space
    vec4 viewPos = viewMatrix * vec4(worldPos, 1.0);
    vec4 clipPos = projMatrix * viewPos;
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    vec2 screenUV = ndcPos.xy * 0.5 + 0.5;
    
    // Light direction in view space
    vec3 viewLightDir = normalize((viewMatrix * vec4(lightDir, 0.0)).xyz);
    
    // Step size in world space
    float stepSize = maxDistance / float(stepCount);
    
    // Jitter to hide stepping artifacts
    float jitter = interleavedGradientNoise(screenUV * screenSize);
    
    // Current position along ray
    vec3 rayPos = viewPos.xyz + viewLightDir * stepSize * jitter;
    
    for (int i = 0; i < stepCount; i++) {
        // Project current ray position to screen
        vec4 rayClip = projMatrix * vec4(rayPos, 1.0);
        vec3 rayNDC = rayClip.xyz / rayClip.w;
        vec2 rayUV = rayNDC.xy * 0.5 + 0.5;
        
        // Check bounds
        if (rayUV.x < 0.0 || rayUV.x > 1.0 || rayUV.y < 0.0 || rayUV.y > 1.0) {
            break;
        }
        
        // Sample scene depth
        float sceneDepth = texture(depthBuffer, rayUV).r;
        
        // Compare depths (NDC z)
        float rayDepth = rayNDC.z;
        float depthDiff = rayDepth - sceneDepth;
        
        // Thickness threshold (thicker for distant objects)
        float thickness = 0.01 * (1.0 + abs(rayPos.z) * 0.1);
        
        if (depthDiff > 0.0 && depthDiff < thickness) {
            // Smooth falloff
            float t = float(i) / float(stepCount);
            return 1.0 - t * t;
        }
        
        // Advance ray
        rayPos += viewLightDir * stepSize;
    }
    
    return 0.0;
}

#endif // PBR_CONTACT_SHADOWS_GLSL
