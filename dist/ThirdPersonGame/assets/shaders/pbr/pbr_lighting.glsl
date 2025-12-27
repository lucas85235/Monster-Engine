// =============================================================================
// PBR Direct Lighting Functions - Based on Google's Filament
// =============================================================================
// This file contains functions for evaluating direct lighting (punctual lights).
// Include pbr_common.glsl before this file.
// =============================================================================

#ifndef PBR_LIGHTING_GLSL
#define PBR_LIGHTING_GLSL

// -----------------------------------------------------------------------------
// Light Structures
// -----------------------------------------------------------------------------

struct DirectionalLight {
    vec3 direction;   // Direction TO the light (normalized)
    vec3 color;       // Light color (linear RGB)
    float intensity;  // Illuminance in lux (or relative intensity)
};

struct PointLight {
    vec3 position;    // World position
    vec3 color;       // Light color (linear RGB)
    float intensity;  // Luminous power in lumens (or relative intensity)
    float range;      // Maximum influence range
};

struct SpotLight {
    vec3 position;     // World position
    vec3 direction;    // Direction the spotlight faces
    vec3 color;        // Light color (linear RGB)
    float intensity;   // Luminous power in lumens
    float range;       // Maximum influence range
    float innerAngle;  // Cosine of inner cone angle
    float outerAngle;  // Cosine of outer cone angle
};

// -----------------------------------------------------------------------------
// Attenuation Functions
// -----------------------------------------------------------------------------

// Inverse square law attenuation with smooth falloff at range boundary
// Based on Frostbite/Filament smooth distance attenuation
float getDistanceAttenuation(float distanceSquare, float inverseRangeSquare) {
    float factor = distanceSquare * inverseRangeSquare;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return smoothFactor * smoothFactor / max(distanceSquare, 1e-4);
}

// Simple inverse square attenuation (physically correct but no range falloff)
float getSquareFalloffAttenuation(float distanceSquare) {
    return 1.0 / max(distanceSquare, 1e-4);
}

// Angular attenuation for spotlights
float getAngleAttenuation(vec3 lightDir, vec3 spotDir, float innerAngle, float outerAngle) {
    float cosAngle = dot(lightDir, spotDir);
    float attenuation = saturate((cosAngle - outerAngle) / (innerAngle - outerAngle));
    return attenuation * attenuation;
}

// -----------------------------------------------------------------------------
// Direct Lighting Evaluation
// -----------------------------------------------------------------------------

// Evaluate directional light contribution
// lightDir is the direction FROM the surface TO the light
vec3 evaluateDirectionalLight(
    DirectionalLight light,
    SurfaceData surface,
    vec3 worldPos,
    vec3 normal,
    vec3 view
) {
    vec3 l = normalize(light.direction);
    LightVectors lv = computeLightVectors(normal, view, l);
    
    if (lv.NoL <= 0.0) return vec3(0.0);
    
    vec3 brdf = surfaceShading(surface, lv);
    
    // Luminance = BRDF * illuminance * cos(theta)
    // For directional lights: illuminance = intensity * NoL
    float illuminance = light.intensity * lv.NoL;
    return brdf * light.color * illuminance;
}

// TODO: Evaluate point light contribution
// vec3 evaluatePointLight(
//     PointLight light,
//     SurfaceData surface,
//     vec3 worldPos,
//     vec3 normal,
//     vec3 view
// ) {
//     vec3 toLight = light.position - worldPos;
//     float distanceSquare = dot(toLight, toLight);
//     vec3 l = toLight * inversesqrt(distanceSquare);
//     
//     LightVectors lv = computeLightVectors(normal, view, l);
//     if (lv.NoL <= 0.0) return vec3(0.0);
//     
//     vec3 brdf = surfaceShading(surface, lv);
//     
//     // Inverse range squared for attenuation
//     float inverseRangeSquare = 1.0 / (light.range * light.range);
//     float attenuation = getDistanceAttenuation(distanceSquare, inverseRangeSquare);
//     
//     // Point light illuminance: I / (4*PI*d^2) * NoL
//     // We fold the 4*PI into the intensity parameter for simplicity
//     float illuminance = light.intensity * attenuation * lv.NoL;
//     return brdf * light.color * illuminance;
// }

// TODO: Evaluate spot light contribution
// vec3 evaluateSpotLight(
//     SpotLight light,
//     SurfaceData surface,
//     vec3 worldPos,
//     vec3 normal,
//     vec3 view
// ) {
//     vec3 toLight = light.position - worldPos;
//     float distanceSquare = dot(toLight, toLight);
//     vec3 l = toLight * inversesqrt(distanceSquare);
//     
//     LightVectors lv = computeLightVectors(normal, view, l);
//     if (lv.NoL <= 0.0) return vec3(0.0);
//     
//     vec3 brdf = surfaceShading(surface, lv);
//     
//     float inverseRangeSquare = 1.0 / (light.range * light.range);
//     float distanceAtt = getDistanceAttenuation(distanceSquare, inverseRangeSquare);
//     float angleAtt = getAngleAttenuation(-l, light.direction, light.innerAngle, light.outerAngle);
//     float attenuation = distanceAtt * angleAtt;
//     
//     float illuminance = light.intensity * attenuation * lv.NoL;
//     return brdf * light.color * illuminance;
// }

// -----------------------------------------------------------------------------
// Convenience Function for Single Directional Light (most common case)
// -----------------------------------------------------------------------------

// Simple directional light evaluation with raw parameters (no struct)
vec3 evaluateDirectionalLightSimple(
    vec3 lightDir,       // Direction TO the light (normalized)
    vec3 lightColor,     // Light color (linear RGB)
    float lightIntensity,// Light intensity
    SurfaceData surface,
    vec3 normal,
    vec3 view
) {
    LightVectors lv = computeLightVectors(normal, view, lightDir);
    
    if (lv.NoL <= 0.0) return vec3(0.0);
    
    vec3 brdf = surfaceShading(surface, lv);
    float illuminance = lightIntensity * lv.NoL;
    return brdf * lightColor * illuminance;
}

#endif // PBR_LIGHTING_GLSL
