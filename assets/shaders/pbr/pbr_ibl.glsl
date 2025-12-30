// =============================================================================
// PBR Image-Based Lighting (IBL) Functions - Based on Google's Filament
// =============================================================================
// This file contains functions for evaluating indirect/environment lighting.
// Supports both Spherical Harmonics fallback and HDR cubemaps.
// Include pbr_common.glsl before this file.
// =============================================================================

#ifndef PBR_IBL_GLSL
#define PBR_IBL_GLSL

// -----------------------------------------------------------------------------
// Spherical Harmonics for Diffuse Irradiance (Fallback)
// -----------------------------------------------------------------------------

vec3 irradianceSH(vec3 n, vec3 sh[9]) {
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

vec3 defaultAmbient(vec3 n, vec3 skyColor, vec3 groundColor) {
    float skyBlend = n.y * 0.5 + 0.5;
    return mix(groundColor, skyColor, skyBlend);
}

// -----------------------------------------------------------------------------
// Analytical DFG Approximation (Fallback when no LUT)
// -----------------------------------------------------------------------------

vec2 prefilteredDFG_Karis(float NoV, float roughness) {
    vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
    return vec2(-1.04, 1.04) * a004 + r.zw;
}

// -----------------------------------------------------------------------------
// Cubemap IBL Evaluation (Proper HDR-based IBL)
// -----------------------------------------------------------------------------

// Sample DFG LUT texture
vec2 prefilteredDFG_LUT(sampler2D dfgLut, float NoV, float perceptualRoughness) {
    return texture(dfgLut, vec2(NoV, perceptualRoughness)).rg;
}

// Sample prefiltered environment cubemap
vec3 samplePrefilteredEnvMap(samplerCube prefilteredMap, vec3 r, float perceptualRoughness, float maxLod) {
    float lod = perceptualRoughness * maxLod;
    return textureLod(prefilteredMap, r, lod).rgb;
}

// Full IBL evaluation with cubemaps (highest quality)
vec3 evaluateIBL_Cubemap(
    samplerCube irradianceMap,
    samplerCube prefilteredMap,
    sampler2D dfgLut,
    float maxPrefilteredLod,
    vec3 diffuseColor,
    vec3 f0,
    float f90,
    float perceptualRoughness,
    vec3 normal,
    vec3 reflected,
    float NoV,
    float intensity
) {
    // Diffuse IBL from irradiance cubemap
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 Fd = diffuseColor * irradiance * Fd_Lambert();
    
    // Specular IBL with split-sum approximation
    vec3 prefilteredColor = samplePrefilteredEnvMap(prefilteredMap, reflected, perceptualRoughness, maxPrefilteredLod);
    vec2 dfg = prefilteredDFG_LUT(dfgLut, NoV, perceptualRoughness);
    vec3 Fr = prefilteredColor * (f0 * dfg.x + f90 * dfg.y);
    
    return (Fd + Fr) * intensity;
}

// -----------------------------------------------------------------------------
// Approximate Specular IBL (Fallback without cubemaps)
// -----------------------------------------------------------------------------

vec3 approximateSpecularIBL(vec3 irradiance, vec3 f0, float f90, float NoV, float roughness) {
    vec2 dfg = prefilteredDFG_Karis(NoV, roughness);
    vec3 specularColor = f0 * dfg.x + vec3(f90 * dfg.y);
    float specularAttenuation = 1.0 - roughness * roughness;
    return irradiance * specularColor * specularAttenuation;
}

// -----------------------------------------------------------------------------
// Complete IBL Evaluation (SH Fallback)
// -----------------------------------------------------------------------------

struct IBLDataShader {
    vec3 sh[9];
    float intensity;
};

vec3 evaluateIBL(
    IBLDataShader ibl,
    SurfaceData surface,
    vec3 normal,
    vec3 view,
    vec3 reflected
) {
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    vec3 irradiance = irradianceSH(normal, ibl.sh);
    vec3 Fd = surface.diffuseColor * irradiance * Fd_Lambert();
    vec3 Fr = approximateSpecularIBL(irradiance, surface.f0, surface.f90, NoV, surface.roughness);
    return (Fd + Fr) * ibl.intensity;
}

vec3 evaluateIBLSimple(
    SurfaceData surface,
    vec3 normal,
    vec3 view,
    vec3 skyColor,
    vec3 groundColor,
    float intensity
) {
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    vec3 irradiance = defaultAmbient(normal, skyColor, groundColor);
    vec3 Fd = surface.diffuseColor * irradiance * Fd_Lambert();
    vec2 dfg = prefilteredDFG_Karis(NoV, surface.roughness);
    vec3 specularColor = surface.f0 * dfg.x + vec3(surface.f90 * dfg.y);
    float specularAttenuation = 1.0 - surface.roughness * surface.roughness * 0.5;
    vec3 Fr = irradiance * specularColor * specularAttenuation;
    return (Fd + Fr) * intensity;
}

#endif // PBR_IBL_GLSL

