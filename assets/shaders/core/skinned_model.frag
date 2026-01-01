#version 330 core

// =============================================================================
// PBR Skinned Model Fragment Shader - Based on Google Filament Standard Model
// =============================================================================

#include "../include/pbr_types.glsl"
#include "../include/pbr_common.glsl"
#include "../include/shading_standard.glsl"
#include "../include/pbr_ibl.glsl"
#include "../include/pbr_fog.glsl"

// Inputs from Vertex Shader
in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_ViewPos;
in vec4 v_LightSpacePos;
in mat3 v_TBN;

out vec4 FragColor;

// Light Uniforms
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

// PBR Material Textures
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uSpecularMap;
uniform sampler2D uAOMap;
uniform sampler2D uMetallicMap;
uniform sampler2D uRoughnessMap;
uniform sampler2D uEmissiveMap;
uniform sampler2D uMetallicRoughnessMap;

// Texture Presence Flags
uniform int uHasAlbedo;
uniform int uHasNormal;
uniform int uHasSpecular;
uniform int uHasAO;
uniform int uHasMetallic;
uniform int uHasRoughness;
uniform int uHasEmissive;
uniform int uHasMetallicRoughness;

// Core PBR Parameters
uniform vec4 uBaseColor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uReflectance;
uniform float uAOFactor;
uniform vec3 uEmissiveColor;
uniform float uEmissiveFactor;
uniform float uNormalScale;
uniform float uShininess;

// Advanced PBR Parameters
uniform float uClearCoat;
uniform float uClearCoatRoughness;
uniform float uAnisotropy;
uniform vec3 uSheenColor;
uniform float uSheenRoughness;
uniform vec3 uSubsurfaceColor;
uniform float uSubsurfacePower;
uniform float uThickness;

// IBL Uniforms
uniform vec3 uSH[9];
uniform float uIBLIntensity;
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// Camera/Post-processing Uniforms
uniform float uExposure;

// Shadow Calculation (PCF 5x5)
float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias = max(0.0003 * (1.0 - ndotl), 0.00005);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    
    float fadeStart = 0.9;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    float edgeFade = maxCoord > fadeStart ? 1.0 - smoothstep(fadeStart, 1.0, maxCoord) : 1.0;
    
    return shadow * edgeFade;
}

// Build MaterialInputs from uniforms/textures (same as model.frag)
MaterialInputs getMaterialInputs(vec3 normal) {
    MaterialInputs m = initMaterialInputs();
    
    // Base Color
    if (uHasAlbedo == 1) {
        m.baseColor = texture(uAlbedoMap, v_TexCoord);
        m.baseColor.rgb = pow(m.baseColor.rgb, vec3(2.2));
    } else {
        m.baseColor = uBaseColor;
        if (length(m.baseColor.rgb) < 0.01) m.baseColor.rgb = vec3(0.5);
    }
    
    // Metallic and Roughness
    if (uHasMetallicRoughness == 1) {
        vec4 mr = texture(uMetallicRoughnessMap, v_TexCoord);
        m.metallic = mr.b * uMetallicFactor;
        m.roughness = mr.g * uRoughnessFactor;
    } else {
        m.metallic = uHasMetallic == 1 ? texture(uMetallicMap, v_TexCoord).r * uMetallicFactor : uMetallicFactor;
        m.roughness = uHasRoughness == 1 ? texture(uRoughnessMap, v_TexCoord).r * uRoughnessFactor : uRoughnessFactor;
        if (uHasSpecular == 1 && m.roughness == 0.0) {
            m.roughness = 1.0 - texture(uSpecularMap, v_TexCoord).r * 0.8;
        }
    }
    m.metallic = saturate(m.metallic);
    m.roughness = saturate(m.roughness);
    
    m.reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    m.ambientOcclusion = uHasAO == 1 ? texture(uAOMap, v_TexCoord).r : 1.0;
    m.ambientOcclusion = mix(1.0, m.ambientOcclusion, uAOFactor * uAOStrength);
    
    if (uHasEmissive == 1) {
        m.emissive = vec4(pow(texture(uEmissiveMap, v_TexCoord).rgb, vec3(2.2)), 1.0);
    }
    m.emissive.rgb *= uEmissiveColor * uEmissiveFactor;
    
    m.normal = normal;
    m.clearCoat = uClearCoat;
    m.clearCoatRoughness = uClearCoatRoughness;
    m.anisotropy = uAnisotropy;
    m.sheenColor = uSheenColor;
    m.sheenRoughness = uSheenRoughness;
    m.subsurfaceColor = uSubsurfaceColor;
    m.subsurfacePower = uSubsurfacePower;
    m.thickness = uThickness;
    
    return m;
}

void main() {
    // Normal mapping
    vec3 normal;
    if (uHasNormal == 1) {
        vec3 normalMapValue = texture(uNormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        normalMapValue.xy *= uNormalScale;
        normal = normalize(v_TBN * normalMapValue);
    } else {
        normal = normalize(v_Normal);
    }
    
    // Build material and shading data
    MaterialInputs material = getMaterialInputs(normal);
    ShadingData shading = initShadingData(v_FragPos, normal, v_ViewPos);
    
    if (uHasNormal == 1) {
        shading.tangent = normalize(v_TBN[0]);
        shading.bitangent = normalize(v_TBN[1]);
    }
    
    PixelParams pixel = getPixelParams(material, shading);
    
    // Create directional light
    Light light = createDirectionalLight(uLightDirection, uLightColor, uLightIntensity);
    light.NoL = saturate(dot(normal, light.l));
    
    // Shadow
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, light.l);
    }
    float visibility = 1.0 - shadow * 0.65;
    visibility *= computeMicroShadowing(light.NoL, material.ambientOcclusion);
    
    // Direct lighting
    vec3 directLight = surfaceShading(pixel, shading, light, visibility);
    
    // IBL
    vec3 indirectLight = vec3(0.0);
    if (length(uSH[0]) > 0.001) {
        vec3 irradiance = irradianceSH(normal, uSH);
        vec3 Fd = pixel.diffuseColor * irradiance * Fd_Lambert();
        vec2 dfg = prefilteredDFG_Karis(shading.NoV, pixel.perceptualRoughness);
        vec3 specularColor = pixel.f0 * dfg.x + vec3(pixel.f90 * dfg.y);
        vec3 Fr = irradiance * specularColor * (1.0 - pixel.roughness * 0.5);
        indirectLight = (Fd + Fr) * uIBLIntensity;
    } else {
        vec3 ambientColor = mix(uGroundColor, uSkyColor, normal.y * 0.5 + 0.5);
        indirectLight = pixel.diffuseColor * ambientColor * uAmbientStrength;
    }
    
    // Apply AO
    vec3 aoColor = multiBounceAO(material.ambientOcclusion, pixel.diffuseColor);
    indirectLight *= aoColor;
    
    vec3 omniAmbient = pixel.diffuseColor * uSH[0] * 0.15;
    float rimFactor = pow(1.0 - saturate(dot(normal, shading.view)), 3.0) * 0.3;
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - material.metallic);
    vec3 minAmbient = pixel.diffuseColor * 0.08;
    indirectLight = max(indirectLight + omniAmbient + rimLight, minAmbient);
    
    // Combine and tone map with dithering
    vec3 hdrColor = directLight + indirectLight + material.emissive.rgb;
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutputDithered(hdrColor, exposure, gl_FragCoord.xy);
    
    FragColor = vec4(ldrColor, material.baseColor.a);
}
