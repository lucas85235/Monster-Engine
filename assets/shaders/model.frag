#version 430 core

// =============================================================================
// PBR Model Fragment Shader - Based on Google Filament Standard Model
// Uses the full Filament-style shading pipeline via shading_standard.glsl
// =============================================================================

#include "pbr/pbr_types.glsl"
#include "pbr/pbr_common.glsl"
#include "pbr/shading_standard.glsl"
#include "pbr/pbr_ibl.glsl"
#include "pbr/pbr_fog.glsl"
#include "pbr/pbr_contact_shadows.glsl"

// -----------------------------------------------------------------------------
// Inputs from Vertex Shader
// -----------------------------------------------------------------------------
in vec3 v_FragPos;
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
uniform mat4 uView;
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

// Advanced PBR Parameters
uniform float uClearCoat;
uniform float uClearCoatRoughness;
uniform float uAnisotropy;
uniform vec3 uAnisotropyDirection;
uniform vec3 uSheenColor;
uniform float uSheenRoughness;
uniform vec3 uSubsurfaceColor;
uniform float uSubsurfacePower;
uniform float uThickness;
uniform float uTransmission;
uniform float uIOR;

// Legacy fallback
uniform float uShininess;

// IBL Uniforms (SH fallback)
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

// Contact Shadows
uniform sampler2D uDepthBuffer;
uniform int uContactShadowsEnabled;
uniform int uContactShadowSteps;
uniform float uContactShadowMaxDistance;
uniform vec2 uScreenSize;

// -----------------------------------------------------------------------------
// Shadow Calculation (PCF 5x5)
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

// -----------------------------------------------------------------------------
// Build MaterialInputs from uniforms and textures
// -----------------------------------------------------------------------------
MaterialInputs getMaterialInputs(vec3 normal) {
    MaterialInputs m = initMaterialInputs();
    
    // Base Color
    if (uHasAlbedo == 1) {
        m.baseColor = texture(uAlbedoMap, v_TexCoord);
        m.baseColor.rgb = pow(m.baseColor.rgb, vec3(2.2)); // sRGB to linear
    } else {
        m.baseColor = uBaseColor;
        if (length(m.baseColor.rgb) < 0.01) {
            m.baseColor.rgb = vec3(0.5);
        }
    }
    
    // Metallic and Roughness
    if (uHasMetallicRoughness == 1) {
        vec4 mr = texture(uMetallicRoughnessMap, v_TexCoord);
        m.metallic = mr.b * uMetallicFactor;
        m.roughness = mr.g * uRoughnessFactor;
    } else {
        m.metallic = uHasMetallic == 1 ? texture(uMetallicMap, v_TexCoord).r * uMetallicFactor : uMetallicFactor;
        m.roughness = uHasRoughness == 1 ? texture(uRoughnessMap, v_TexCoord).r * uRoughnessFactor : uRoughnessFactor;
        
        // Legacy specular fallback
        if (uHasSpecular == 1 && m.roughness == 0.0) {
            float specValue = texture(uSpecularMap, v_TexCoord).r;
            m.roughness = 1.0 - specValue * 0.8;
        }
    }
    m.metallic = saturate(m.metallic);
    m.roughness = saturate(m.roughness);
    
    // Reflectance
    m.reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    
    // Ambient Occlusion
    m.ambientOcclusion = uHasAO == 1 ? texture(uAOMap, v_TexCoord).r : 1.0;
    m.ambientOcclusion = mix(1.0, m.ambientOcclusion, uAOFactor * uAOStrength);
    
    // Emissive
    if (uHasEmissive == 1) {
        m.emissive = vec4(pow(texture(uEmissiveMap, v_TexCoord).rgb, vec3(2.2)), 1.0);
    }
    m.emissive.rgb *= uEmissiveColor * uEmissiveFactor;
    
    // Normal (already processed in main)
    m.normal = normal;
    
    // Advanced parameters
    m.clearCoat = uClearCoat;
    m.clearCoatRoughness = uClearCoatRoughness;
    m.anisotropy = uAnisotropy;
    m.anisotropyDirection = uAnisotropyDirection;
    m.sheenColor = uSheenColor;
    m.sheenRoughness = uSheenRoughness;
    m.subsurfaceColor = uSubsurfaceColor;
    m.subsurfacePower = uSubsurfacePower;
    m.thickness = uThickness;
    m.transmission = uTransmission;
    m.ior = uIOR > 0.0 ? uIOR : 1.5;
    
    return m;
}

// -----------------------------------------------------------------------------
// Main Fragment Shader
// -----------------------------------------------------------------------------
void main() {
    // 1. Sample/compute normal
    vec3 normal;
    if (uHasNormal == 1) {
        vec3 normalMapValue = texture(uNormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        normalMapValue.xy *= uNormalScale;
        normal = normalize(v_TBN * normalMapValue);
    } else {
        normal = normalize(v_Normal);
    }
    
    // 2. Build MaterialInputs from uniforms/textures
    MaterialInputs material = getMaterialInputs(normal);
    
    // 3. Initialize ShadingData
    ShadingData shading = initShadingData(v_FragPos, normal, v_ViewPos);
    
    // Use TBN for anisotropy if available
    if (uHasNormal == 1) {
        shading.tangent = normalize(v_TBN[0]);
        shading.bitangent = normalize(v_TBN[1]);
    }
    
    // 4. Convert to PixelParams for shading
    PixelParams pixel = getPixelParams(material, shading);
    
    // 5. Create directional light
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
    
    // Apply micro-shadowing from AO
    visibility *= computeMicroShadowing(light.NoL, material.ambientOcclusion);
    
    // 7. Evaluate direct lighting using Filament pipeline
    vec3 directLight = surfaceShading(pixel, shading, light, visibility);
    
    // 8. Evaluate IBL (indirect lighting)
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
        // Diffuse IBL via Spherical Harmonics (fallback)
        vec3 irradiance = irradianceSH(normal, uSH);
        vec3 Fd = pixel.diffuseColor * irradiance * Fd_Lambert();
        
        // Specular IBL approximation
        vec2 dfg = prefilteredDFG_Karis(shading.NoV, pixel.perceptualRoughness);
        vec3 specularColor = pixel.f0 * dfg.x + vec3(pixel.f90 * dfg.y);
        float specularAttenuation = 1.0 - pixel.roughness * 0.5;
        vec3 Fr = irradiance * specularColor * specularAttenuation;
        
        indirectLight = (Fd + Fr) * uIBLIntensity;
    } else {
        // Fallback ambient
        vec3 ambientColor = mix(uGroundColor, uSkyColor, normal.y * 0.5 + 0.5);
        indirectLight = pixel.diffuseColor * ambientColor * uAmbientStrength;
    }
    
    // 9. Apply AO with multi-bounce correction
    vec3 aoColor = multiBounceAO(material.ambientOcclusion, pixel.diffuseColor);
    float specAO = specularAO(shading.NoV, material.ambientOcclusion, pixel.roughness);
    indirectLight *= aoColor;
    
    // Omnidirectional ambient (fills deep shadows)
    vec3 omniAmbient = pixel.diffuseColor * uSH[0] * 0.15;
    
    // Rim lighting for edge definition
    float rimFactor = 1.0 - saturate(dot(normal, shading.view));
    rimFactor = pow(rimFactor, 3.0) * 0.3;
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - material.metallic);
    
    // Minimum ambient floor
    vec3 minAmbient = pixel.diffuseColor * 0.08;
    indirectLight = max(indirectLight + omniAmbient + rimLight, minAmbient);
    
    // 10. GI contribution (Radiance Cascades / SSGI)
    if (uHasGI == 1) {
        ivec2 texSize = textureSize(uGIMap, 0);
        vec2 screenUV = clamp(gl_FragCoord.xy / vec2(texSize), vec2(0.001), vec2(0.999));
        vec3 giContribution = texture(uGIMap, screenUV).rgb;
        
        if (uGIIntensity > 4.0) {
            FragColor = vec4(giContribution, 1.0);
            return;
        }
        
        indirectLight += giContribution * uGIIntensity * mix(pixel.diffuseColor, vec3(0.04), material.metallic);
    }
    
    // 11. Combine all lighting
    vec3 hdrColor = directLight + indirectLight + material.emissive.rgb;
    
    // 12. Tone mapping, gamma correction, and dithering
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutputDithered(hdrColor, exposure, gl_FragCoord.xy);
    
    vec4 color = vec4(ldrColor, material.baseColor.a);

    // Debug visualization: tint by cascade
    if (uVisualizeCascades == 1) {
        float viewDepth = abs((uView * vec4(v_FragPos, 1.0)).z);
        int cascade = getCascadeIndex(viewDepth);
        vec3 tint = vec3(1.0);
        if (cascade == 0) tint = vec3(1.0, 0.4, 0.4); // Reddish
        if (cascade == 1) tint = vec3(0.4, 1.0, 0.4); // Greenish
        if (cascade == 2) tint = vec3(0.4, 0.4, 1.0); // Blueish
        if (cascade == 3) tint = vec3(1.0, 1.0, 0.4); // Yellowish
        color.rgb = mix(color.rgb, tint, 0.35);
    }
    
    FragColor = color;
}
