// =============================================================================
// Cloth Shading Model - For fabrics, velvet, and textile materials
// =============================================================================
// Uses Charlie NDF and Neubelt visibility from pbr_common.glsl
// Based on "Production Friendly Microfacet Sheen BRDF" - Estevez and Kulla 2017

#ifndef SHADING_CLOTH_GLSL
#define SHADING_CLOTH_GLSL

#include "pbr_types.glsl"
#include "pbr_common.glsl"

// Cloth-specific surface shading
// Uses sheen color instead of specular F0
// Adds subsurface color for multi-layer fabric look
vec3 surfaceShadingCloth(PixelParams pixel, ShadingData shading, Light light, float occlusion) {
    vec3 h = normalize(shading.view + light.l);
    
    float NoV = max(shading.NoV, MIN_N_DOT_V);
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading.normal, h));
    float LoH = saturate(dot(light.l, h));
    
    if (NoL <= 0.0) {
        return vec3(0.0);
    }
    
    // Cloth specular using Charlie NDF and Neubelt visibility
    float D = D_Charlie(pixel.sheenRoughness, NoH);
    float V = V_Neubelt(NoV, NoL);
    vec3 F = pixel.sheenColor;  // For cloth, sheen color acts as specular tint
    vec3 Fr = (D * V) * F;
    
    // Diffuse BRDF (can use Burley for more accurate fabric retroreflection)
    vec3 Fd = pixel.diffuseColor * Fd_Burley(NoV, NoL, LoH, pixel.perceptualRoughness);
    
    // Subsurface approximation for multi-layer fabrics
    // Wraps lighting around edges for softer falloff
    if (length(pixel.subsurfaceColor) > 0.001) {
        float wrap = 0.5;
        float wrappedNoL = saturate((NoL + wrap) / (1.0 + wrap));
        vec3 subsurface = pixel.subsurfaceColor * Fd_Lambert() * wrappedNoL;
        Fd = mix(Fd, Fd + subsurface, pixel.thickness);
    }
    
    vec3 color = Fd + Fr;
    
    return (color * light.colorIntensity.rgb) *
           (light.colorIntensity.w * light.attenuation * NoL * occlusion);
}

// IBL evaluation for cloth materials
vec3 evaluateClothIBL(
    PixelParams pixel,
    ShadingData shading,
    vec3 irradiance,
    float diffuseAO,
    float iblIntensity
) {
    // Diffuse IBL
    vec3 Fd = pixel.diffuseColor * irradiance * Fd_Lambert();
    
    // Cloth specular IBL is simplified - uses sheen color with horizon fade
    float horizonFade = 1.0 - saturate(1.0 - shading.NoV);
    horizonFade = horizonFade * horizonFade;
    vec3 Fr = pixel.sheenColor * irradiance * (1.0 - pixel.sheenRoughness) * horizonFade * 0.25;
    
    // Apply AO
    Fd *= diffuseAO;
    Fr *= diffuseAO;
    
    return (Fd + Fr) * iblIntensity;
}

#endif // SHADING_CLOTH_GLSL
