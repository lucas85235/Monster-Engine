// =============================================================================
// PBR Image-Based Lighting (IBL) Functions - Based on Google's Filament
// =============================================================================
// This file contains functions for evaluating indirect/environment lighting.
// Uses Spherical Harmonics for diffuse and analytical DFG for specular.
// Include pbr_common.glsl before this file.
// =============================================================================

#ifndef PBR_IBL_GLSL
#define PBR_IBL_GLSL

// -----------------------------------------------------------------------------
// Spherical Harmonics for Diffuse Irradiance
// -----------------------------------------------------------------------------
// 9 coefficients (3 bands) provide a good approximation of diffuse irradiance.
// The coefficients are pre-convolved with the cosine lobe.

vec3 irradianceSH(vec3 n, vec3 sh[9]) {
    // L00 + L1-1*y + L10*z + L11*x + L2-2*xy + L2-1*yz + ...
    return sh[0]
         + sh[1] * n.y
         + sh[2] * n.z
         + sh[3] * n.x
         + sh[4] * (n.y * n.x)
         + sh[5] * (n.y * n.z)
         + sh[6] * (3.0 * n.z * n.z - 1.0)
         + sh[7] * (n.z * n.x)
         + sh[8] * (n.x * n.x - n.y * n.y);
}

// Simplified SH with only 2 bands (4 coefficients) for mobile
vec3 irradianceSH_2Bands(vec3 n, vec3 sh[4]) {
    return sh[0]
         + sh[1] * n.y
         + sh[2] * n.z
         + sh[3] * n.x;
}

// Default ambient when no SH data is available (simple sky gradient)
vec3 defaultAmbient(vec3 n, vec3 skyColor, vec3 groundColor) {
    float skyBlend = n.y * 0.5 + 0.5;
    return mix(groundColor, skyColor, skyBlend);
}

// -----------------------------------------------------------------------------
// Analytical DFG Approximation (Karis/Lazarov)
// -----------------------------------------------------------------------------
// Approximates the pre-integrated DFG term without a LUT texture.
// Based on Karis (UE4) and Lazarov (Call of Duty) approximations.

// Returns vec2(scale, bias) for F0 * scale + F90 * bias
vec2 prefilteredDFG_Karis(float NoV, float roughness) {
    // Karis' approximation from "Real Shading in Unreal Engine 4"
    vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

// Alternative Lazarov approximation (slightly different fit)
vec2 prefilteredDFG_Lazarov(float NoV, float roughness) {
    // Lazarov's approximation
    float c0 = roughness + (1.0 - roughness) * pow(1.0 - NoV, 5.0);
    float c1 = (1.0 - roughness) * pow(1.0 - NoV, 5.0);
    return vec2(1.0 - c0, c0);
}

// TODO: Use precomputed DFG LUT texture for better accuracy
// vec2 prefilteredDFG_LUT(sampler2D dfgLUT, float NoV, float roughness) {
//     return textureLod(dfgLUT, vec2(NoV, roughness), 0.0).xy;
// }

// -----------------------------------------------------------------------------
// Specular IBL Approximation
// -----------------------------------------------------------------------------
// Without a prefiltered environment cubemap, we approximate specular reflection.

// TODO: Sample prefiltered environment cubemap
// vec3 samplePrefilteredEnvMap(samplerCube envMap, vec3 r, float roughness, float maxLod) {
//     float lod = roughness * maxLod;  // perceptualRoughness maps linearly to LOD
//     return textureLod(envMap, r, lod).rgb;
// }

// Approximate specular IBL using diffuse irradiance and fresnel
// This is a rough approximation when no prefiltered env map is available
vec3 approximateSpecularIBL(vec3 irradiance, vec3 f0, float f90, float NoV, float roughness) {
    vec2 dfg = prefilteredDFG_Karis(NoV, roughness);
    vec3 specularColor = f0 * dfg.x + vec3(f90 * dfg.y);
    
    // Rough approximation: use irradiance attenuated by roughness
    // Real IBL would sample the env map at the reflection direction with LOD based on roughness
    float specularAttenuation = 1.0 - roughness * roughness;
    return irradiance * specularColor * specularAttenuation;
}

// -----------------------------------------------------------------------------
// Complete IBL Evaluation
// -----------------------------------------------------------------------------

struct IBLData {
    vec3 sh[9];           // Spherical Harmonics for diffuse irradiance
    float intensity;       // Overall IBL intensity multiplier
    // TODO: samplerCube prefilteredEnvMap;
    // TODO: sampler2D dfgLUT;
    // TODO: float maxLod;
};

// Evaluate complete IBL contribution (diffuse + specular)
vec3 evaluateIBL(
    IBLData ibl,
    SurfaceData surface,
    vec3 normal,
    vec3 view,
    vec3 reflected
) {
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    
    // Diffuse IBL from Spherical Harmonics
    vec3 irradiance = irradianceSH(normal, ibl.sh);
    vec3 Fd = surface.diffuseColor * irradiance * Fd_Lambert();
    
    // Specular IBL (approximated without prefiltered env map)
    vec3 Fr = approximateSpecularIBL(irradiance, surface.f0, surface.f90, NoV, surface.roughness);
    
    return (Fd + Fr) * ibl.intensity;
}

// Simplified IBL without SH data (uses default sky gradient)
vec3 evaluateIBLSimple(
    SurfaceData surface,
    vec3 normal,
    vec3 view,
    vec3 skyColor,
    vec3 groundColor,
    float intensity
) {
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    
    // Simple sky/ground gradient for ambient
    vec3 irradiance = defaultAmbient(normal, skyColor, groundColor);
    
    // Diffuse
    vec3 Fd = surface.diffuseColor * irradiance * Fd_Lambert();
    
    // Specular approximation
    vec2 dfg = prefilteredDFG_Karis(NoV, surface.roughness);
    vec3 specularColor = surface.f0 * dfg.x + vec3(surface.f90 * dfg.y);
    float specularAttenuation = 1.0 - surface.roughness * surface.roughness * 0.5;
    vec3 Fr = irradiance * specularColor * specularAttenuation;
    
    return (Fd + Fr) * intensity;
}

#endif // PBR_IBL_GLSL
