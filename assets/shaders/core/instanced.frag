#version 430 core

// =============================================================================
// PBR Instanced Fragment Shader - Based on Google Filament Standard Model
// =============================================================================

#include "../include/pbr_types.glsl"
#include "../include/pbr_common.glsl"
#include "../include/shading_standard.glsl"
#include "../include/pbr_ibl.glsl"
#include "../include/pbr_fog.glsl"
#include "../include/pbr_contact_shadows.glsl"

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
uniform sampler2D uSSAOTexture;
uniform int uHasSSAO;

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
uniform mat4 uProjection;

// Debug modes: 0=off, 1=AO only, 2=Normals, 3=Roughness, 4=Metallic
uniform int uDebugMode;

// Contact Shadows
uniform sampler2D uDepthBuffer;
uniform int uContactShadowsEnabled;
uniform int uContactShadowSteps;
uniform float uContactShadowMaxDistance;
uniform vec2 uScreenSize;

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
    float baseBias = 0.0001 * (1.0 + float(cascade) * 0.3);
    float bias = max(baseBias * (1.0 - ndotl), baseBias * 0.05);
    
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
    
    // Build MaterialInputs
    // If v_Color is white (1,1,1) - which is the default instance color - use uBaseColor uniform
    // Otherwise use the per-instance color from v_Color
    MaterialInputs material = initMaterialInputs();
    vec3 baseColor = (v_Color == vec3(1.0)) ? uBaseColor.rgb : v_Color;
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
    Light light = createDirectionalLight(uLightDirection, uLightColor, uLightIntensity);
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
    
    // Apply contact shadows (screen-space detail shadows)
    if (uContactShadowsEnabled == 1 && visibility > 0.0 && light.NoL > 0.0) {
        int steps = uContactShadowSteps > 0 ? uContactShadowSteps : 16;
        float maxDist = uContactShadowMaxDistance > 0.0 ? uContactShadowMaxDistance : 0.5;
        float contactShadow = contactShadowDirectional(
            uDepthBuffer, v_FragPos, light.l, uView, uProjection,
            uScreenSize, steps, maxDist
        );
        visibility *= (1.0 - contactShadow * 0.5);
    }
    
    // Debug visualization modes (bypass all lighting calculations)
    if (uDebugMode == 2) {
        // World normals visualization
        color = vec4(normal * 0.5 + 0.5, 1.0);
        return;
    } else if (uDebugMode == 3) {
        // Roughness visualization
        color = vec4(vec3(pixel.perceptualRoughness), 1.0);
        return;
    } else if (uDebugMode == 4) {
        // Metallic visualization
        color = vec4(vec3(material.metallic), 1.0);
        return;
    } else if (uDebugMode == 5) {
        // Depth visualization (logarithmic for better close-range detail)
        float viewDepth = abs((uView * vec4(v_FragPos, 1.0)).z);
        float near = 0.1;
        float far = 200.0;
        // Use logarithmic depth for better distribution
        float logDepth = log(viewDepth / near) / log(far / near);
        logDepth = clamp(logDepth, 0.0, 1.0);
        color = vec4(vec3(1.0 - logDepth), 1.0);
        return;
    } else if (uDebugMode == 6) {
        // Geometry info visualization: face normals + checker pattern
        vec3 dPosdx = dFdx(v_FragPos);
        vec3 dPosdy = dFdy(v_FragPos);
        vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
        vec3 faceNormalColor = faceNormal * 0.5 + 0.5;
        
        // Add checker pattern to show triangles/quads
        float checker = mod(floor(v_FragPos.x * 2.0) + floor(v_FragPos.y * 2.0) + floor(v_FragPos.z * 2.0), 2.0);
        faceNormalColor *= mix(0.8, 1.0, checker);
        
        color = vec4(faceNormalColor, 1.0);
        return;
    }
    
    // Sample SSAO texture or fallback to procedural AO
    float ao;
    if (uHasSSAO == 1) {
        // Sample from SSAO texture using screen-space coordinates
        vec2 screenUV = gl_FragCoord.xy / uScreenSize;
        ao = texture(uSSAOTexture, screenUV).r;
        ao = mix(1.0, ao, uAOStrength);  // Apply strength factor
    } else {
        // Fallback to procedural AO (works on edges/curvature only)
        ao = proceduralAO(normal, v_FragPos, uAORadius, uAOStrength);
    }
    
    // Apply micro-shadowing
    visibility *= computeMicroShadowing(light.NoL, ao);
    
    // AO debug mode (after AO calculation)
    if (uDebugMode == 1) {
        // AO only - white = no occlusion, black = full occlusion
        color = vec4(vec3(ao), 1.0);
        return;
    }
    
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
    
    // Ambient and rim lighting (reduced for better shadow contrast)
    vec3 omniAmbient = pixel.diffuseColor * uSH[0] * 0.05;
    float rimFactor = pow(1.0 - saturate(dot(normal, shading.view)), 3.0) * 0.15;
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - material.metallic);
    vec3 minAmbient = pixel.diffuseColor * 0.02;
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
    
    // Combine and tone map with dithering
    vec3 hdrColor = directLight + indirectLight + material.emissive.rgb;
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutputDithered(hdrColor, exposure, gl_FragCoord.xy);
    
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
