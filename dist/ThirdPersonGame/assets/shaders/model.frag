#version 330 core

// =============================================================================
// PBR Model Fragment Shader - Based on Google's Filament Standard Model
// =============================================================================

// Include PBR library files
#include "pbr/pbr_common.glsl"
#include "pbr/pbr_lighting.glsl"
#include "pbr/pbr_ibl.glsl"

// -----------------------------------------------------------------------------
// Inputs from Vertex Shader
// -----------------------------------------------------------------------------
in vec3 v_FragPos;      // World position (same as v_WorldPos in PBR shaders)
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_ViewPos;
in vec4 v_LightSpacePos;
in mat3 v_TBN;

out vec4 FragColor;

// -----------------------------------------------------------------------------
// Light Uniforms
// -----------------------------------------------------------------------------
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;

// Shadow Uniforms
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// AO Uniforms
uniform float uAOStrength;
uniform float uAORadius;

// -----------------------------------------------------------------------------
// PBR Material Textures
// -----------------------------------------------------------------------------
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uSpecularMap;       // Legacy specular map
uniform sampler2D uAOMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uEmissiveMap;
uniform sampler2D uMetallicRoughnessMap;  // Combined GLTF texture

// Texture Presence Flags
uniform int uHasAlbedo;
uniform int uHasNormal;
uniform int uHasSpecular;
uniform int uHasAO;
uniform int uHasMetallic;
uniform int uHasRoughness;
uniform int uHasEmissive;
uniform int uHasMetallicRoughness;

// PBR Material Parameters
uniform vec4 uBaseColor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uReflectance;
uniform float uAOFactor;
uniform vec3 uEmissiveColor;
uniform float uEmissiveFactor;
uniform float uNormalScale;

// Legacy Blinn-Phong fallback
uniform float uShininess;

// IBL Uniforms
uniform vec3 uSH[9];
uniform float uIBLIntensity;
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// Camera/Post-processing Uniforms
uniform float uExposure;

// -----------------------------------------------------------------------------
// Shadow Calculation
// -----------------------------------------------------------------------------
float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

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
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    float edgeFade = maxCoord > fadeStart ? 1.0 - smoothstep(fadeStart, 1.0, maxCoord) : 1.0;
    
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
        vec3 normalMapValue = texture(uNormalMap, v_TexCoord).rgb * 2.0 - 1.0;
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
        // Convert from sRGB to linear
        baseColor.rgb = pow(baseColor.rgb, vec3(2.2));
    } else {
        baseColor = uBaseColor;
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
        // Try separate textures or use uniform values
        if (uHasMetallic == 1) {
            metallic = texture(uMetallicMap, v_TexCoord).r * uMetallicFactor;
        } else {
            metallic = uMetallicFactor;
        }
        
        if (uHasRoughness == 1) {
            perceptualRoughness = texture(uRoughnessMap, v_TexCoord).r * uRoughnessFactor;
        } else {
            perceptualRoughness = uRoughnessFactor;
            // If no roughness set and using legacy specular, derive roughness from shininess
            if (uHasSpecular == 1 && perceptualRoughness == 0.0) {
                float specValue = texture(uSpecularMap, v_TexCoord).r;
                perceptualRoughness = 1.0 - specValue * 0.8;
            }
        }
    }
    
    metallic = saturate(metallic);
    perceptualRoughness = saturate(perceptualRoughness);
    
    // Ambient Occlusion
    float ao = 1.0;
    if (uHasAO == 1) {
        ao = texture(uAOMap, v_TexCoord).r;
    }
    ao = mix(1.0, ao, uAOFactor * uAOStrength);
    
    // Emissive
    vec3 emissive = vec3(0.0);
    if (uHasEmissive == 1) {
        emissive = pow(texture(uEmissiveMap, v_TexCoord).rgb, vec3(2.2));
    }
    emissive = emissive * uEmissiveColor * uEmissiveFactor;
    
    // -------------------------------------------------------------------------
    // 3. Compute PBR Surface Data
    // -------------------------------------------------------------------------
    float roughness = clampRoughness(perceptualRoughnessToRoughness(perceptualRoughness));
    vec3 diffuseColor = computeDiffuseColor(baseColor.rgb, metallic);
    float reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    vec3 f0 = computeF0(baseColor.rgb, metallic, reflectance);
    
    SurfaceData surface;
    surface.diffuseColor = diffuseColor;
    surface.f0 = f0;
    surface.roughness = roughness;
    surface.f90 = 1.0;
    
    // -------------------------------------------------------------------------
    // 4. Compute Lighting
    // -------------------------------------------------------------------------
    vec3 view = normalize(v_ViewPos - v_FragPos);
    vec3 lightDir = normalize(uLightDirection);
    
    // Direct Lighting (Directional Light)
    vec3 directLight = evaluateDirectionalLightSimple(
        lightDir, uLightColor, uLightIntensity, surface, normal, view
    );
    
    // Shadow
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }
    directLight *= (1.0 - shadow * 0.65);
    
    // Indirect Lighting (IBL approximation)
    vec3 indirectLight;
    
    // Check if SH data is provided
    if (length(uSH[0]) > 0.001) {
        vec3 irradiance = irradianceSH(normal, uSH);
        vec3 Fd = diffuseColor * irradiance * Fd_Lambert();
        
        float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
        vec2 dfg = prefilteredDFG_Karis(NoV, roughness);
        vec3 specularColor = f0 * dfg.x + vec3(surface.f90 * dfg.y);
        float specularAttenuation = 1.0 - roughness * roughness * 0.5;
        vec3 Fr = irradiance * specularColor * specularAttenuation;
        
        indirectLight = (Fd + Fr) * uIBLIntensity;
    } else {
        // Fallback to simple ambient
        indirectLight = evaluateIBLSimple(surface, normal, view, uSkyColor, uGroundColor, uAmbientStrength);
    }
    
    // Apply multi-bounce AO to preserve color in shadowed areas
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    vec3 aoColor = multiBounceAO(ao, diffuseColor);
    float specAO = specularAO(NoV, ao, roughness);
    
    // Separate diffuse and specular AO application
    indirectLight = indirectLight * aoColor;
    
    // Omnidirectional ambient - constant light from all directions (ignores normal)
    // This ensures surfaces facing away from sky still receive some light
    vec3 omniAmbient = diffuseColor * uSH[0] * 0.15;  // 15% of L00 term as constant
    
    // Rim lighting - adds subtle edge definition when backlit
    float rimFactor = 1.0 - saturate(dot(normal, view));
    rimFactor = pow(rimFactor, 3.0) * 0.3;  // Soft rim, 30% intensity
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - metallic);
    
    // Add minimum ambient floor (8% of diffuse color)
    vec3 minAmbient = diffuseColor * 0.08;
    indirectLight = max(indirectLight + omniAmbient + rimLight, minAmbient);
    
    // -------------------------------------------------------------------------
    // 5. Combine Final Color with HDR Pipeline
    // -------------------------------------------------------------------------
    vec3 hdrColor = directLight + indirectLight + emissive;
    
    // Apply exposure, tone mapping, and gamma correction
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutput(hdrColor, exposure);
    
    FragColor = vec4(ldrColor, baseColor.a);
}
