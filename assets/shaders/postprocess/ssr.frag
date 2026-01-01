#version 460 core
// =============================================================================
// Screen Space Reflections (SSR)
// =============================================================================
// Hierarchical ray marching for real-time reflections.
// Should be blended with IBL for surfaces outside screen bounds.

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uColorTexture;      // Scene color
uniform sampler2D uDepthTexture;      // Scene depth
uniform sampler2D uNormalTexture;     // World-space normals
uniform sampler2D uRoughnessTexture;  // Roughness (optional)

uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;
uniform mat4 uInvViewMatrix;
uniform mat4 uInvProjectionMatrix;

uniform vec2 uResolution;
uniform float uMaxDistance;       // Maximum ray distance
uniform float uThickness;         // Depth comparison thickness
uniform int uMaxSteps;            // Ray march steps (64 typical)
uniform float uRoughnessThreshold;// Don't do SSR above this roughness

// Convert screen UV + depth to view-space position
vec3 getViewPosition(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = uInvProjectionMatrix * clipPos;
    return viewPos.xyz / viewPos.w;
}

// Convert view-space position to screen UV
vec3 viewToScreen(vec3 viewPos) {
    vec4 clipPos = uProjectionMatrix * vec4(viewPos, 1.0);
    clipPos.xyz /= clipPos.w;
    return vec3(clipPos.xy * 0.5 + 0.5, clipPos.z * 0.5 + 0.5);
}

// Binary search refinement
vec3 binarySearch(vec3 rayOrigin, vec3 rayDir, float hitT) {
    float lo = hitT - (uMaxDistance / float(uMaxSteps));
    float hi = hitT;
    
    for (int i = 0; i < 8; i++) {
        float mid = (lo + hi) * 0.5;
        vec3 testPos = rayOrigin + rayDir * mid;
        vec3 testScreen = viewToScreen(testPos);
        
        if (testScreen.x < 0.0 || testScreen.x > 1.0 ||
            testScreen.y < 0.0 || testScreen.y > 1.0) {
            hi = mid;
            continue;
        }
        
        float testDepth = texture(uDepthTexture, testScreen.xy).r;
        vec3 testViewPos = getViewPosition(testScreen.xy, testDepth);
        
        if (testPos.z < testViewPos.z) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    
    return rayOrigin + rayDir * hi;
}

// Ray march in view space
vec4 rayMarch(vec3 rayOrigin, vec3 rayDir) {
    float stepSize = uMaxDistance / float(uMaxSteps);
    
    for (int i = 1; i <= uMaxSteps; i++) {
        float t = stepSize * float(i);
        vec3 testPos = rayOrigin + rayDir * t;
        
        // Project to screen
        vec3 screenPos = viewToScreen(testPos);
        
        // Check bounds
        if (screenPos.x < 0.0 || screenPos.x > 1.0 ||
            screenPos.y < 0.0 || screenPos.y > 1.0 ||
            screenPos.z < 0.0 || screenPos.z > 1.0) {
            return vec4(0.0);
        }
        
        // Sample depth at this screen position
        float sampledDepth = texture(uDepthTexture, screenPos.xy).r;
        vec3 sampledViewPos = getViewPosition(screenPos.xy, sampledDepth);
        
        // Check for intersection
        float depthDiff = testPos.z - sampledViewPos.z;
        
        if (depthDiff > 0.0 && depthDiff < uThickness) {
            // Hit! Refine with binary search
            vec3 hitPos = binarySearch(rayOrigin, rayDir, t);
            vec3 hitScreen = viewToScreen(hitPos);
            
            // Fade at screen edges
            vec2 edgeFade = smoothstep(0.0, 0.1, hitScreen.xy) * 
                            smoothstep(0.0, 0.1, 1.0 - hitScreen.xy);
            float fade = edgeFade.x * edgeFade.y;
            
            // Fade based on ray distance
            fade *= 1.0 - smoothstep(uMaxDistance * 0.5, uMaxDistance, t);
            
            vec3 color = texture(uColorTexture, hitScreen.xy).rgb;
            return vec4(color, fade);
        }
    }
    
    return vec4(0.0);
}

void main() {
    // Get fragment data
    float depth = texture(uDepthTexture, vTexCoord).r;
    
    // Skip sky
    if (depth >= 1.0) {
        FragColor = vec4(0.0);
        return;
    }
    
    // Check roughness threshold
    float roughness = texture(uRoughnessTexture, vTexCoord).r;
    if (roughness > uRoughnessThreshold) {
        FragColor = vec4(0.0);
        return;
    }
    
    // Get view-space position
    vec3 viewPos = getViewPosition(vTexCoord, depth);
    
    // Get world-space normal and convert to view-space
    vec3 worldNormal = texture(uNormalTexture, vTexCoord).rgb * 2.0 - 1.0;
    vec3 viewNormal = normalize((uViewMatrix * vec4(worldNormal, 0.0)).xyz);
    
    // Compute reflection direction in view space
    vec3 viewDir = normalize(viewPos);
    vec3 reflectDir = reflect(viewDir, viewNormal);
    
    // Ray march
    vec4 ssrResult = rayMarch(viewPos + reflectDir * 0.01, reflectDir);
    
    // Fade based on roughness (rough surfaces get less SSR)
    float roughnessFade = 1.0 - smoothstep(0.0, uRoughnessThreshold, roughness);
    ssrResult.a *= roughnessFade;
    
    // Fade based on view angle (glancing angles get more reflection)
    float fresnelFade = pow(1.0 - abs(dot(-viewDir, viewNormal)), 2.0);
    ssrResult.a *= mix(0.3, 1.0, fresnelFade);
    
    FragColor = ssrResult;
}
