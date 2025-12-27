#version 330 core

// =============================================================================
// PBR Standard Fragment Shader - Based on Google's Filament
// =============================================================================
// This shader implements a physically-based rendering model with:
// - Cook-Torrance specular BRDF (GGX NDF, Smith-GGX visibility, Schlick Fresnel)
// - Lambertian diffuse BRDF
// - IBL approximation using Spherical Harmonics and analytical DFG
// =============================================================================

// Include PBR library files
#include "pbr_common.glsl"
#include "pbr_lighting.glsl"
#include "pbr_ibl.glsl"

// -----------------------------------------------------------------------------
// Inputs from Vertex Shader
// -----------------------------------------------------------------------------
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_ViewPos;
in vec4 v_LightSpacePos;
in mat3 v_TBN;

out vec4 FragColor;

// -----------------------------------------------------------------------------
// Light Uniforms
// -----------------------------------------------------------------------------
uniform vec3 uLightDirection;    // Direction TO the light
uniform vec3 uLightColor;        // Light color (linear RGB)
uniform float uLightIntensity;   // Light intensity
uniform float uAmbientStrength;  // Ambient/IBL intensity multiplier

// -----------------------------------------------------------------------------
// Shadow Uniforms (for future shadow integration)
// -----------------------------------------------------------------------------
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// -----------------------------------------------------------------------------
// PBR Material Textures (slots 1-7)
// -----------------------------------------------------------------------------
uniform sampler2D uAlbedoMap;            // Base color texture (sRGB)
uniform sampler2D uNormalMap;            // Tangent-space normal map
uniform sampler2D uMetallicRoughnessMap; // Combined: G=Roughness, B=Metallic (GLTF)
uniform sampler2D uAOMap;                // Ambient occlusion
uniform sampler2D uEmissiveMap;          // Emissive color

// Legacy separate textures (fallback)
uniform sampler2D uRoughnessMap;         // Separate roughness texture
uniform sampler2D uMetallicMap;          // Separate metallic texture

// -----------------------------------------------------------------------------
// Texture Flags (1 = has texture, 0 = use uniform)
// -----------------------------------------------------------------------------
uniform int uHasAlbedo;
uniform int uHasNormal;
uniform int uHasMetallicRoughness;  // Combined GLTF texture
uniform int uHasRoughness;          // Separate roughness
uniform int uHasMetallic;           // Separate metallic
uniform int uHasAO;
uniform int uHasEmissive;

// -----------------------------------------------------------------------------
// PBR Material Parameters (fallbacks when textures not present)
// -----------------------------------------------------------------------------
uniform vec4 uBaseColor;           // Base color (linear RGBA)
uniform float uMetallicFactor;     // Metallic factor (0-1)
uniform float uRoughnessFactor;    // Perceptual roughness (0-1)
uniform float uReflectance;        // Dielectric reflectance (default 0.5 = 4% F0)
uniform float uAOFactor;           // AO multiplier
uniform vec3 uEmissiveColor;       // Emissive color
uniform float uEmissiveFactor;     // Emissive intensity
uniform float uNormalScale;        // Normal map intensity

// -----------------------------------------------------------------------------
// IBL / Environment Uniforms
// -----------------------------------------------------------------------------
uniform vec3 uSH[9];               // Spherical Harmonics for diffuse irradiance
uniform float uIBLIntensity;       // IBL overall intensity
uniform vec3 uSkyColor;            // Fallback sky color for simple IBL
uniform vec3 uGroundColor;         // Fallback ground color for simple IBL

// -----------------------------------------------------------------------------
// Shadow Calculation (compatible with existing shadow system)
// -----------------------------------------------------------------------------
float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 0.0;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias = max(0.0003 * (1.0 - ndotl), 0.00005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    
    // 5x5 PCF
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    
    // Edge fade
    float fadeStart = 0.9;
    float edgeFade = 1.0;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    if (maxCoord > fadeStart) {
        edgeFade = 1.0 - smoothstep(fadeStart, 1.0, maxCoord);
    }
    
    return shadow * edgeFade;
}

// -----------------------------------------------------------------------------
// Main Fragment Shader
// -----------------------------------------------------------------------------
void main() {
    // -------------------------------------------------------------------------
    // 1. Sample Normal (with normal mapping)
    // -------------------------------------------------------------------------
    vec3 normal;
    if (uHasNormal == 1) {
        vec3 normalMapValue = texture(uNormalMap, v_TexCoord).rgb;
        normalMapValue = normalMapValue * 2.0 - 1.0;
        normalMapValue.xy *= uNormalScale;
        normal = normalize(v_TBN * normalMapValue);
    } else {
        normal = normalize(v_Normal);
    }
    
    // -------------------------------------------------------------------------
    // 2. Sample Material Properties
    // -------------------------------------------------------------------------
    
    // Base Color (albedo)
    vec4 baseColor;
    if (uHasAlbedo == 1) {
        baseColor = texture(uAlbedoMap, v_TexCoord);
        // Convert from sRGB to linear (approximation)
        baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    } else {
        baseColor = uBaseColor;
        // Fallback to gray if zero
        if (length(baseColor.rgb) < 0.01) {
            baseColor.rgb = vec3(0.5);
        }
    }
    
    // Metallic and Roughness
    float metallic;
    float perceptualRoughness;
    
    if (uHasMetallicRoughness == 1) {
        // GLTF format: G=Roughness, B=Metallic
        vec4 mr = texture(uMetallicRoughnessMap, v_TexCoord);
        metallic = mr.b * uMetallicFactor;
        perceptualRoughness = mr.g * uRoughnessFactor;
    } else {
        // Try separate textures
        if (uHasMetallic == 1) {
            metallic = texture(uMetallicMap, v_TexCoord).r * uMetallicFactor;
        } else {
            metallic = uMetallicFactor;
        }
        
        if (uHasRoughness == 1) {
            perceptualRoughness = texture(uRoughnessMap, v_TexCoord).r * uRoughnessFactor;
        } else {
            perceptualRoughness = uRoughnessFactor;
        }
    }
    
    // Clamp metallic and roughness
    metallic = saturate(metallic);
    perceptualRoughness = saturate(perceptualRoughness);
    
    // Ambient Occlusion
    float ao = 1.0;
    if (uHasAO == 1) {
        ao = texture(uAOMap, v_TexCoord).r;
    }
    ao = mix(1.0, ao, uAOFactor);
    
    // Emissive
    vec3 emissive = vec3(0.0);
    if (uHasEmissive == 1) {
        emissive = texture(uEmissiveMap, v_TexCoord).rgb;
        emissive = pow(emissive, vec3(2.2)); // sRGB to linear
    }
    emissive = emissive * uEmissiveColor * uEmissiveFactor;
    
    // -------------------------------------------------------------------------
    // 3. Compute PBR Surface Data
    // -------------------------------------------------------------------------
    
    // Convert perceptual roughness to roughness (squared)
    float roughness = perceptualRoughnessToRoughness(perceptualRoughness);
    roughness = clampRoughness(roughness);
    
    // Compute diffuse color (non-metallic contribution)
    vec3 diffuseColor = computeDiffuseColor(baseColor.rgb, metallic);
    
    // Compute F0 (specular reflectance at normal incidence)
    float reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    vec3 f0 = computeF0(baseColor.rgb, metallic, reflectance);
    
    // Create surface data structure
    SurfaceData surface;
    surface.diffuseColor = diffuseColor;
    surface.f0 = f0;
    surface.roughness = roughness;
    surface.f90 = 1.0; // Smooth materials reflect 100% at grazing angles
    
    // -------------------------------------------------------------------------
    // 4. Compute Lighting
    // -------------------------------------------------------------------------
    vec3 view = normalize(v_ViewPos - v_WorldPos);
    vec3 lightDir = normalize(uLightDirection);
    
    // Direct Lighting (Directional Light)
    vec3 directLight = evaluateDirectionalLightSimple(
        lightDir,
        uLightColor,
        uLightIntensity,
        surface,
        normal,
        view
    );
    
    // Shadow (if enabled)
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }
    float shadowAttenuation = 1.0 - shadow * 0.65;
    directLight *= shadowAttenuation;
    
    // Indirect Lighting (IBL approximation)
    vec3 indirectLight;
    
    // Check if SH data is provided (first coefficient not zero means it's set)
    if (length(uSH[0]) > 0.001) {
        // Use Spherical Harmonics for diffuse irradiance
        vec3 irradiance = irradianceSH(normal, uSH);
        vec3 Fd = diffuseColor * irradiance * Fd_Lambert();
        
        // Specular IBL approximation
        float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
        vec2 dfg = prefilteredDFG_Karis(NoV, roughness);
        vec3 specularColor = f0 * dfg.x + vec3(surface.f90 * dfg.y);
        float specularAttenuation = 1.0 - roughness * roughness * 0.5;
        vec3 Fr = irradiance * specularColor * specularAttenuation;
        
        indirectLight = (Fd + Fr) * uIBLIntensity;
    } else {
        // Fallback to simple ambient with sky/ground gradient
        indirectLight = evaluateIBLSimple(
            surface,
            normal,
            view,
            uSkyColor.rgb,
            uGroundColor.rgb,
            uAmbientStrength
        );
    }
    
    // Apply ambient occlusion to indirect light
    indirectLight *= ao;
    
    // -------------------------------------------------------------------------
    // 5. Combine Final Color
    // -------------------------------------------------------------------------
    vec3 color = directLight + indirectLight + emissive;
    
    // Output (will be tonemapped in post-processing if available)
    FragColor = vec4(color, baseColor.a);
}
