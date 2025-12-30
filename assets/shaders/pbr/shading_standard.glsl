// =============================================================================
// PBR Shading Standard - Main Shading Pipeline
// Based on Google Filament - See pbr_document.md PARTE 3
// =============================================================================

#ifndef PBR_SHADING_STANDARD_GLSL
#define PBR_SHADING_STANDARD_GLSL

#include "pbr_types.glsl"
#include "pbr_common.glsl"
#include "pbr_ibl.glsl"

// =============================================================================
// PixelParams Computation
// =============================================================================
// Converts MaterialInputs to PixelParams for shading.

PixelParams getPixelParams(MaterialInputs material, ShadingData shading) {
    PixelParams pixel;
    
    // Core parameters
    pixel.perceptualRoughness = clamp(material.roughness, 0.0, 1.0);
    pixel.roughness = clampRoughness(perceptualRoughnessToRoughness(pixel.perceptualRoughness));
    pixel.diffuseColor = computeDiffuseColor(material.baseColor.rgb, material.metallic);
    pixel.f0 = computeF0(material.baseColor.rgb, material.metallic, material.reflectance);
    pixel.f90 = 1.0;
    
    // Energy compensation - Kulla and Conty 2017
    // Uses analytical DFG approximation
    vec2 dfg = prefilteredDFG_Karis(shading.NoV, pixel.perceptualRoughness);
    pixel.dfg = vec3(dfg.x, dfg.y, 0.0);
    // energyCompensation = 1 + f0 * (1/dfg.y - 1)
    pixel.energyCompensation = 1.0 + pixel.f0 * (1.0 / max(dfg.y, 0.001) - 1.0);
    
    // Clear coat
    pixel.clearCoat = material.clearCoat;
    pixel.clearCoatRoughness = clampRoughness(perceptualRoughnessToRoughness(material.clearCoatRoughness));
    
    // Sheen
    pixel.sheenColor = material.sheenColor;
    pixel.sheenRoughness = clampRoughness(perceptualRoughnessToRoughness(material.sheenRoughness));
    pixel.sheenScaling = 1.0;
    pixel.sheenDFG = 0.0;
    
    // Anisotropy
    pixel.anisotropy = material.anisotropy;
    pixel.anisotropicT = shading.tangent;
    pixel.anisotropicB = shading.bitangent;
    
    // Compute anisotropic roughness
    float aniso = abs(material.anisotropy);
    pixel.at = max(pixel.roughness * (1.0 + aniso), MIN_ROUGHNESS);
    pixel.ab = max(pixel.roughness * (1.0 - aniso), MIN_ROUGHNESS);
    
    // Subsurface
    pixel.subsurfacePower = material.subsurfacePower;
    pixel.subsurfaceColor = material.subsurfaceColor;
    pixel.thickness = material.thickness;
    
    // Transmission
    pixel.transmission = material.transmission;
    pixel.absorption = material.absorption;
    pixel.etaIR = 1.0 / material.ior;
    pixel.etaRI = material.ior;
    
    return pixel;
}

// =============================================================================
// Specular Lobes
// =============================================================================

// Isotropic specular lobe - standard PBR
vec3 isotropicLobe(PixelParams pixel, float NoV, float NoL, float NoH, float LoH) {
    float D = D_GGX(NoH, pixel.roughness);
    float V = V_SmithGGXCorrelated(NoV, NoL, pixel.roughness);
    vec3  F = F_Schlick(LoH, pixel.f0, pixel.f90);
    return (D * V) * F;
}

// Anisotropic specular lobe - for brushed metal, hair
vec3 anisotropicLobe(PixelParams pixel, ShadingData shading, vec3 h,
        float NoV, float NoL, float NoH, float LoH) {
    vec3 l = reflect(-shading.view, shading.normal);
    float ToV = dot(shading.tangent, shading.view);
    float BoV = dot(shading.bitangent, shading.view);
    float ToL = dot(shading.tangent, l);
    float BoL = dot(shading.bitangent, l);
    float ToH = dot(shading.tangent, h);
    float BoH = dot(shading.bitangent, h);
    
    float D = D_GGX_Anisotropic(pixel.at, pixel.ab, ToH, BoH, NoH);
    float V = V_SmithGGXCorrelated_Anisotropic(pixel.at, pixel.ab, ToV, BoV, ToL, BoL, NoV, NoL);
    vec3  F = F_Schlick(LoH, pixel.f0, pixel.f90);
    return (D * V) * F;
}

// Select appropriate specular lobe
vec3 specularLobe(PixelParams pixel, ShadingData shading, vec3 h,
        float NoV, float NoL, float NoH, float LoH) {
    if (abs(pixel.anisotropy) > 0.01) {
        return anisotropicLobe(pixel, shading, h, NoV, NoL, NoH, LoH);
    }
    return isotropicLobe(pixel, NoV, NoL, NoH, LoH);
}

// =============================================================================
// Diffuse Lobes
// =============================================================================

// Lambertian diffuse (fast)
vec3 diffuseLobe_Lambert(PixelParams pixel) {
    return pixel.diffuseColor * Fd_Lambert();
}

// Burley diffuse (more accurate for rough surfaces)
vec3 diffuseLobe_Burley(PixelParams pixel, float NoV, float NoL, float LoH) {
    return pixel.diffuseColor * Fd_Burley(NoV, NoL, LoH, pixel.roughness);
}

// Default diffuse lobe - uses Burley for quality
vec3 diffuseLobe(PixelParams pixel, float NoV, float NoL, float LoH) {
    return diffuseLobe_Burley(pixel, NoV, NoL, LoH);
}

// =============================================================================
// Clear Coat Layer
// =============================================================================

float clearCoatLobe(PixelParams pixel, vec3 n, vec3 h, float NoH, float LoH, out float Fcc) {
    // Clear coat uses fixed IOR of 1.5 (4% reflectance)
    float D = D_GGX(NoH, pixel.clearCoatRoughness);
    float V = V_Kelemen(LoH);
    float F = F_Schlick(LoH, 0.04, 1.0) * pixel.clearCoat;
    
    Fcc = F;
    return D * V * F;
}

// =============================================================================
// Sheen Layer (Cloth/Fabric)
// =============================================================================

vec3 sheenLobe(PixelParams pixel, float NoV, float NoL, float NoH) {
    float D = D_Charlie(pixel.sheenRoughness, NoH);
    float V = V_Neubelt(NoV, NoL);
    return (D * V) * pixel.sheenColor;
}

// =============================================================================
// Main Surface Shading
// =============================================================================

vec3 surfaceShading(PixelParams pixel, ShadingData shading, Light light, float occlusion) {
    vec3 h = normalize(shading.view + light.l);
    
    float NoV = shading.NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading.normal, h));
    float LoH = saturate(dot(light.l, h));
    
    if (NoL <= 0.0) {
        return vec3(0.0);
    }
    
    // Specular lobe
    vec3 Fr = specularLobe(pixel, shading, h, NoV, NoL, NoH, LoH);
    
    // Diffuse lobe
    vec3 Fd = diffuseLobe(pixel, NoV, NoL, LoH);
    
    // Apply energy compensation for multi-scattering
    vec3 color = Fd + Fr * pixel.energyCompensation;
    
    // Sheen layer (cloth/velvet)
    if (length(pixel.sheenColor) > 0.001) {
        color *= pixel.sheenScaling;
        color += sheenLobe(pixel, NoV, NoL, NoH);
    }
    
    // Clear coat layer
    if (pixel.clearCoat > 0.0) {
        float Fcc;
        float clearCoat = clearCoatLobe(pixel, shading.normal, h, NoH, LoH, Fcc);
        float attenuation = 1.0 - Fcc;
        color *= attenuation;
        color += clearCoat;
    }
    
    // Final light contribution
    return (color * light.colorIntensity.rgb) *
           (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

// Simplified surface shading without optional layers
vec3 surfaceShadingSimple(PixelParams pixel, ShadingData shading, Light light, float occlusion) {
    vec3 h = normalize(shading.view + light.l);
    
    float NoV = shading.NoV;
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading.normal, h));
    float LoH = saturate(dot(light.l, h));
    
    if (NoL <= 0.0) {
        return vec3(0.0);
    }
    
    vec3 Fr = isotropicLobe(pixel, NoV, NoL, NoH, LoH);
    vec3 Fd = diffuseLobe_Lambert(pixel);
    vec3 color = Fd + Fr;
    
    return (color * light.colorIntensity.rgb) *
           (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

#endif // PBR_SHADING_STANDARD_GLSL
