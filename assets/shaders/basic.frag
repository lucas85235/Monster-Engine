#version 330 core

// =============================================================================
// PBR Basic Fragment Shader - For simple geometry with vertex colors
// =============================================================================

// Include PBR library files
#include "pbr/pbr_common.glsl"
#include "pbr/pbr_lighting.glsl"
#include "pbr/pbr_ibl.glsl"

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
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// AO Uniforms
uniform float uAOStrength;
uniform float uAORadius;

// PBR Material Parameters
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uReflectance;
uniform vec4 uBaseColor;

// IBL Uniforms
uniform vec3 uSH[9];
uniform float uIBLIntensity;
uniform vec3 uSkyColor;
uniform vec3 uGroundColor;

// Camera/Post-processing Uniforms
uniform float uExposure;

// Shadow Calculation
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

void main() {
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(uLightDirection);
    vec3 view = normalize(v_ViewPos - v_FragPos);
    
    // Use material baseColor directly (if set), otherwise fall back to vertex color
    vec3 baseColor = uBaseColor.a > 0.01 ? uBaseColor.rgb : v_Color;
    
    // PBR parameters (defaults for basic geometry - non-metallic, medium roughness)
    float metallic = uMetallicFactor;
    float perceptualRoughness = uRoughnessFactor > 0.0 ? uRoughnessFactor : 0.5;
    float reflectance = uReflectance > 0.0 ? uReflectance : 0.5;
    
    // Compute PBR surface data
    float roughness = clampRoughness(perceptualRoughnessToRoughness(perceptualRoughness));
    vec3 diffuseColor = computeDiffuseColor(baseColor, metallic);
    vec3 f0 = computeF0(baseColor, metallic, reflectance);
    
    SurfaceData surface;
    surface.diffuseColor = diffuseColor;
    surface.f0 = f0;
    surface.roughness = roughness;
    surface.f90 = 1.0;
    
    // Direct Lighting
    vec3 directLight = evaluateDirectionalLightSimple(lightDir, uLightColor, uLightIntensity, surface, normal, view);
    
    // Shadow
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }
    directLight *= (1.0 - shadow * 0.65);
    
    // Indirect Lighting
    vec3 indirectLight;
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
        indirectLight = evaluateIBLSimple(surface, normal, view, uSkyColor, uGroundColor, uAmbientStrength);
    }
    
    // Procedural AO using curvature and cavity detection
    float ao = proceduralAO(normal, v_FragPos, uAORadius, uAOStrength);
    
    // Apply multi-bounce AO to preserve color in shadowed areas
    float NoV = abs(dot(normal, view)) + MIN_N_DOT_V;
    vec3 aoColor = multiBounceAO(ao, diffuseColor);
    float specAO = specularAO(NoV, ao, roughness);
    
    // Apply multi-bounce AO
    indirectLight = indirectLight * aoColor;
    
    // Omnidirectional ambient - constant light from all directions (ignores normal)
    vec3 omniAmbient = diffuseColor * uSH[0] * 0.15;  // 15% of L00 term as constant
    
    // Rim lighting - adds subtle edge definition when backlit
    float rimFactor = 1.0 - saturate(dot(normal, view));
    rimFactor = pow(rimFactor, 3.0) * 0.3;  // Soft rim, 30% intensity
    vec3 rimLight = uSkyColor * rimFactor * (1.0 - metallic);
    
    // Add minimum ambient floor (8% of diffuse color)
    vec3 minAmbient = diffuseColor * 0.08;
    indirectLight = max(indirectLight + omniAmbient + rimLight, minAmbient);
    
    // HDR Pipeline: exposure, tone mapping, gamma
    vec3 hdrColor = directLight + indirectLight;
    float exposure = uExposure > 0.0 ? uExposure : 1.0;
    vec3 ldrColor = finalColorOutput(hdrColor, exposure);
    
    color = vec4(ldrColor, 1.0);
}