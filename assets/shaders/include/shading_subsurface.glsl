// =============================================================================
// Subsurface Shading Model - For skin, wax, leaves, and translucent materials
// =============================================================================
// Adds light transmission through thin materials and subsurface scattering.
// Based on Filament's subsurface model.

#ifndef SHADING_SUBSURFACE_GLSL
#define SHADING_SUBSURFACE_GLSL

#include "pbr_types.glsl"
#include "pbr_common.glsl"

// Subsurface scattering approximation
// Combines wrap lighting with forward/back scatter for realistic SSS
vec3 evaluateSubsurfaceScattering(
    PixelParams pixel,
    ShadingData shading,
    Light light
) {
    // View-light scatter (forward scatter through material)
    float scatterVoH = saturate(dot(shading.view, -light.l));
    float forwardScatter = exp2(scatterVoH * pixel.subsurfacePower - pixel.subsurfacePower);
    
    // Wrap lighting (back scatter - light from behind surface)
    float NoL = light.NoL;
    float backScatter = saturate(NoL * pixel.thickness + (1.0 - pixel.thickness)) * 0.5;
    
    // Combine scatter components
    float subsurface = mix(backScatter, 1.0, forwardScatter) * (1.0 - pixel.thickness);
    
    return pixel.subsurfaceColor * subsurface * Fd_Lambert();
}

// Full subsurface surface shading
vec3 surfaceShadingSubsurface(PixelParams pixel, ShadingData shading, Light light, float occlusion) {
    vec3 h = normalize(shading.view + light.l);
    
    float NoV = max(shading.NoV, MIN_N_DOT_V);
    float NoL = saturate(light.NoL);
    float NoH = saturate(dot(shading.normal, h));
    float LoH = saturate(dot(light.l, h));
    
    // Standard specular lobe
    float D = D_GGX(NoH, pixel.roughness);
    float V = V_SmithGGXCorrelated(NoV, NoL, pixel.roughness);
    vec3 F = F_Schlick(LoH, pixel.f0, pixel.f90);
    vec3 Fr = (D * V) * F;
    
    // Standard diffuse
    vec3 Fd = pixel.diffuseColor * Fd_Burley(NoV, NoL, LoH, pixel.perceptualRoughness);
    
    // Add subsurface scattering
    vec3 Fss = evaluateSubsurfaceScattering(pixel, shading, light);
    
    // Combine - subsurface adds to diffuse, replacing some of it
    vec3 color = Fd + Fr + Fss;
    
    // For subsurface materials, we use NoL differently - allow some light through
    float transmissionNoL = mix(NoL, saturate(NoL + 0.5), 1.0 - pixel.thickness);
    
    return (color * light.colorIntensity.rgb) *
           (light.colorIntensity.w * light.attenuation * transmissionNoL * occlusion);
}

// Transmission (for glass, water, thin materials)
// Returns the transmitted light color
vec3 evaluateTransmission(
    PixelParams pixel,
    ShadingData shading,
    sampler2D backgroundTexture,
    vec2 screenUV,
    float sceneDepth
) {
    if (pixel.transmission <= 0.0) {
        return vec3(0.0);
    }
    
    // Compute refracted direction
    vec3 refractedDir = refract(-shading.view, shading.normal, pixel.etaIR);
    
    // Perturb sample position based on refraction
    vec2 refractedUV = screenUV + refractedDir.xy * pixel.thickness * 0.1;
    refractedUV = clamp(refractedUV, vec2(0.001), vec2(0.999));
    
    // Sample background
    vec3 transmittedColor = texture(backgroundTexture, refractedUV).rgb;
    
    // Apply absorption (Beer-Lambert law)
    float distance = pixel.thickness * 0.5;  // Approximate distance through material
    vec3 absorption = exp(-pixel.absorption * distance);
    transmittedColor *= absorption;
    
    // Fresnel-weighted transmission (more transmission at normal incidence)
    float transmissionFresnel = 1.0 - F_Schlick(shading.NoV, 0.04, 1.0);
    
    return transmittedColor * pixel.transmission * transmissionFresnel;
}

// IBL for subsurface materials
vec3 evaluateSubsurfaceIBL(
    PixelParams pixel,
    ShadingData shading,
    vec3 irradiance,
    float diffuseAO,
    float iblIntensity
) {
    // Standard diffuse IBL
    vec3 Fd = pixel.diffuseColor * irradiance * Fd_Lambert();
    
    // Add wrapped ambient for subsurface
    vec3 wrappedNormal = normalize(shading.normal + shading.view * 0.5);
    vec3 subsurfaceIrradiance = irradiance * 0.5;  // Approximate, should sample with wrapped normal
    vec3 Fss = pixel.subsurfaceColor * subsurfaceIrradiance * Fd_Lambert() * (1.0 - pixel.thickness);
    
    Fd *= diffuseAO;
    
    return (Fd + Fss) * iblIntensity;
}

#endif // SHADING_SUBSURFACE_GLSL
