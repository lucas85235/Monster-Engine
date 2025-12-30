#version 430 core

// =============================================================================
// PBR Instanced Fragment Shader - Based on Google Filament Standard Model
// =============================================================================

#include "pbr/pbr_types.glsl"
#include "pbr/pbr_common.glsl"
#include "pbr/shading_standard.glsl"
#include "pbr/pbr_ibl.glsl"
#include "pbr/pbr_fog.glsl"

layout(location = 0) out vec4 color;

in vec3 v_Color;
in vec3 v_ViewPos;
in vec3 v_Normal;
in vec3 v_FragPos;
in vec4 v_LightSpacePos;
in float f_SpecularStrenght;

// Light Uniforms
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;

// Shadow Uniforms
uniform sampler2D uShadowMap;
uniform mat4 uView;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// AO Uniforms
uniform float uAOStrength;
uniform float uAORadius;

// Core PBR Parameters
uniform vec4 uBaseColor;
uniform int uUseBaseColorOverride;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uReflectance;

// Advanced PBR Parameters
uniform float uClearCoat;
uniform float uClearCoatRoughness;
uniform float uAnisotropy;
uniform vec3 uSheenColor;
uniform float uSheenRoughness;

// IBL Uniforms
uniform vec3 uSH[9];
uniform float uIBLIntensity;
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// HDR IBL Cubemap Uniforms
uniform int uHasIBLCubemaps;
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilteredMap;
uniform sampler2D uDfgLut;
uniform float uMaxPrefilteredLod;

// GI Uniforms
uniform sampler2D uGIMap;
uniform int uHasGI;
uniform float uGIIntensity;

// CSM (Cascaded Shadow Maps) Uniforms
uniform int uUseCSM;
uniform sampler2DArray uShadowCascades;
uniform mat4 uCascadeMatrices[4];
uniform float uCascadeSplits[4];
uniform int uCascadeCount;
uniform int uVisualizeCascades;

// Camera/Post-processing
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

// -----------------------------------------------------------------------------
// CSM Shadow Calculation
// -----------------------------------------------------------------------------
int getCascadeIndex(float viewDepth) {
    for (int i = 0; i < 4; i++) {
        if (viewDepth < uCascadeSplits[i]) return i;
    }
    return 3;
}

float CalculateCascadedShadow(vec3 worldPos, float viewDepth, vec3 normal, vec3 lightDir) {
    int cascade = getCascadeIndex(viewDepth);
    vec4 shadowCoord = uCascadeMatrices[cascade] * vec4(worldPos, 1.0);
    vec3 projCoords = shadowCoord.xyz / shadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    
    float ndotl = max(dot(normal, lightDir), 0.0);
    float baseBias = 0.0005 * (1.0 + float(cascade) * 0.5);
    float bias = max(baseBias * (1.0 - ndotl), baseBias * 0.1);
    
    // PCF 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowCascades, 0).xy);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            vec3 sampleCoord = vec3(projCoords.xy + vec2(x, y) * texelSize, float(cascade));
            float pcfDepth = texture(uShadowCascades, sampleCoord).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    // Edge fade
    float fadeStart = 0.85;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    float edgeFade = maxCoord > fadeStart ? 1.0 - smoothstep(fadeStart, 1.0, maxCoord) : 1.0;
    
    return shadow * edgeFade;
}

void main() {
    vec3 normal = normalize(v_Normal);
    
    // Build MaterialInputs - use override color if enabled, otherwise vertex color
    MaterialInputs material = initMaterialInputs();
    vec3 baseColor = (uUseBaseColorOverride == 1) ? uBaseColor.rgb : v_Color;
    material.baseColor = vec4(baseColor, 1.0);
    material.metallic = uMetallicFactor;
    material.roughness = uRoughnessFactor > 0.0 ? uRoughnessFactor : 0.5;
    material.reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    material.clearCoat = uClearCoat;
    material.clearCoatRoughness = uClearCoatRoughness;
    material.anisotropy = uAnisotropy;
    material.sheenColor = uSheenColor;
    material.sheenRoughness = uSheenRoughness;
    
    // Initialize ShadingData
    ShadingData shading = initShadingData(v_FragPos, normal, v_ViewPos);
    
    // Convert to PixelParams
    PixelParams pixel = getPixelParams(material, shading);
    
    // Create directional light
    Light light = createDirectionalLight(-uLightDirection, uLightColor, uLightIntensity);
    light.NoL = saturate(dot(normal, light.l));
    
    // 6. Calculate shadow
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        if (uUseCSM == 1) {
            float viewDepth = abs((uView * vec4(v_FragPos, 1.0)).z);
            shadow = CalculateCascadedShadow(v_FragPos, viewDepth, normal, light.l);
        } else {
            shadow = CalculateShadow(v_LightSpacePos, normal, light.l);
        }
    }
    float visibility = 1.0 - shadow * 0.65;
    
    // Apply micro-shadowing
    float ao = proceduralAO(normal, v_FragPos, uAORadius, uAOStrength);
    visibility *= computeMicroShadowing(light.NoL, ao);
    
    // Evaluate direct lighting 
    vec3 directLight = surfaceShading(pixel, shading, light, visibility);

    // Evaluate IBL
    vec3 indirectLight = vec3(0.0);
    
    if (uHasIBLCubemaps == 1) {
        // HDR cubemap-based IBL (highest quality)
        indirectLight = evaluateIBL_Cubemap(
            uIrradianceMap,
            uPrefilteredMap,
            uDfgLut,
            uMaxPrefilteredLod,
            pixel.diffuseColor,
            pixel.f0,
            pixel.f90,
            pixel.perceptualRoughness,
            normal,
            shading.reflected,
            shading.NoV,
            uIBLIntensity
        );
    } else if (length(uSH[0]) > 0.001) {
        // SH fallback
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
    vec3 aoColor = multiBounceAO(ao, pixel.diffuseColor);
    indirectLight *= aoColor;
    
    // Ambient and rim lighting
    vec3 omniAmbient = pixel.diffuseColor * uSH[0] * 0.15;
    float rimFactor = pow(1.0 - saturate(dot(normal, shading.view)), 3.0) * 0.3;
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - material.metallic);
    vec3 minAmbient = pixel.diffuseColor * 0.08;
    indirectLight = max(indirectLight + omniAmbient + rimLight, minAmbient);
    
    // GI contribution
    if (uHasGI == 1) {
        ivec2 texSize = textureSize(uGIMap, 0);
        vec2 screenUV = clamp(gl_FragCoord.xy / vec2(texSize), vec2(0.001), vec2(0.999));
        vec3 giContribution = texture(uGIMap, screenUV).rgb;
        
        if (uGIIntensity > 4.0) {
            color = vec4(giContribution, 1.0);
            return;
        }
        
        indirectLight += giContribution * uGIIntensity * mix(pixel.diffuseColor, vec3(0.04), material.metallic);
    }
    
    // Combine and tone map
    vec3 hdrColor = directLight + indirectLight + material.emissive.rgb;
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutput(hdrColor, exposure);
    
    vec4 finalResult = vec4(ldrColor, 1.0);
    
    // Debug visualization: tint by cascade
    if (uVisualizeCascades == 1) {
        float viewDepth = abs((uView * vec4(v_FragPos, 1.0)).z);
        int cascade = getCascadeIndex(viewDepth);
        vec3 tint = vec3(1.0);
        if (cascade == 0) tint = vec3(1.0, 0.4, 0.4); // Reddish
        if (cascade == 1) tint = vec3(0.4, 1.0, 0.4); // Greenish
        if (cascade == 2) tint = vec3(0.4, 0.4, 1.0); // Blueish
        if (cascade == 3) tint = vec3(1.0, 1.0, 0.4); // Yellowish
        finalResult.rgb = mix(finalResult.rgb, tint, 0.35);
    }
    
    color = finalResult;
}