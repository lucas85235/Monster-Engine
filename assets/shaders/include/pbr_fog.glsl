// =============================================================================
// PBR Fog - Atmospheric Effects
// Based on Google Filament - See pbr_document.md PARTE 6
// =============================================================================

#ifndef PBR_FOG_GLSL
#define PBR_FOG_GLSL

// =============================================================================
// Fog Parameters
// =============================================================================

struct FogParams {
    vec3  color;              // Fog color (linear)
    float density;            // Base density
    float heightFalloff;      // Density falloff with height
    float start;              // Distance where fog starts
    float maxOpacity;         // Maximum fog opacity [0-1]
    float cutOffDistance;     // Distance where fog stops
    
    // Sun inscattering
    float inscatteringSize;   // Sun inscattering power
    float inscatteringStart;  // Distance where inscattering starts
    vec3  sunColor;           // Sun color for inscattering
    vec3  sunDirection;       // Direction to sun
};

// =============================================================================
// Exponential Height Fog with Inscattering
// =============================================================================
// Wenzel, "Real-time Atmospheric Effects in Games"

vec4 fog(vec4 color, vec3 viewDir, float distance, FogParams params) {
    if (distance < params.start || distance > params.cutOffDistance) {
        return color;
    }
    
    // Height-based density calculation
    // density.z = density * exp(-falloff * height)
    float height = viewDir.y;
    float heightDensity = params.density * exp(-params.heightFalloff * height);
    
    // Optical path for height-varying fog
    float fogOpticalPathAtOneMeter = heightDensity;
    float fh = params.heightFalloff * viewDir.y;
    if (abs(fh) > 0.00125) {
        float baseDensity = params.density * exp(-params.heightFalloff * height);
        fogOpticalPathAtOneMeter = (heightDensity - baseDensity * exp(-fh)) / fh;
    }
    
    // Beer-Lambert Law
    float fogOpticalPath = fogOpticalPathAtOneMeter * max(distance - params.start, 0.0);
    float fogTransmittance = exp(-fogOpticalPath);
    float fogOpacity = min(1.0 - fogTransmittance, params.maxOpacity);
    
    // Base fog color
    vec3 fogColor = params.color * fogOpacity;
    
    // Sun inscattering
    if (params.inscatteringSize > 0.0) {
        float sunOpticalPath = fogOpticalPathAtOneMeter * max(distance - params.inscatteringStart, 0.0);
        float sunTransmittance = exp(-sunOpticalPath);
        
        float sunAmount = max(dot(normalize(viewDir), params.sunDirection), 0.0);
        float sunInscattering = pow(sunAmount, params.inscatteringSize);
        
        fogColor += params.sunColor * (sunInscattering * (1.0 - sunTransmittance));
    }
    
    color.rgb = color.rgb * (1.0 - fogOpacity) + fogColor;
    return color;
}

// =============================================================================
// Linear Fog (Simplified)
// =============================================================================

vec4 fogLinear(vec4 color, float distance, float start, float end, vec3 fogColor, float maxOpacity) {
    if (distance < start || distance > end) {
        return color;
    }
    
    float fogFactor = (distance - start) / (end - start);
    float fogOpacity = min(fogFactor, maxOpacity);
    
    color.rgb = mix(color.rgb, fogColor, fogOpacity);
    return color;
}

// =============================================================================
// Exponential Fog (Simple)
// =============================================================================

vec4 fogExponential(vec4 color, float distance, float density, vec3 fogColor, float maxOpacity) {
    float fogFactor = 1.0 - exp(-distance * density);
    float fogOpacity = min(fogFactor, maxOpacity);
    
    color.rgb = mix(color.rgb, fogColor, fogOpacity);
    return color;
}

// =============================================================================
// Exponential Squared Fog
// =============================================================================

vec4 fogExponential2(vec4 color, float distance, float density, vec3 fogColor, float maxOpacity) {
    float d = distance * density;
    float fogFactor = 1.0 - exp(-d * d);
    float fogOpacity = min(fogFactor, maxOpacity);
    
    color.rgb = mix(color.rgb, fogColor, fogOpacity);
    return color;
}

#endif // PBR_FOG_GLSL
