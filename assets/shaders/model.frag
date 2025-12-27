#version 330 core

in vec3 v_FragPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_ViewPos;
in vec4 v_LightSpacePos;
in mat3 v_TBN;

out vec4 FragColor;

// Light uniforms (matching SceneRenderer)
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;

// Shadow uniforms
uniform sampler2D uShadowMap;
uniform mat4 uLightSpaceMatrix;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// AO uniforms
uniform float uAOStrength;
uniform float uAORadius;

// Texture samplers (slots 1-7)
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uSpecularMap;
uniform sampler2D uAOMap;

// Texture flags (1 = has texture, 0 = no texture)
uniform int uHasAlbedo;
uniform int uHasNormal;
uniform int uHasSpecular;
uniform int uHasAO;

// Fallback values
uniform vec4 uBaseColor;
uniform float uShininess;

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
    
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    
    float fadeStart = 0.9;
    float edgeFade = 1.0;
    vec2 absCoord = abs(projCoords.xy * 2.0 - 1.0);
    float maxCoord = max(absCoord.x, absCoord.y);
    if (maxCoord > fadeStart) {
        edgeFade = 1.0 - smoothstep(fadeStart, 1.0, maxCoord);
    }
    
    return shadow * edgeFade;
}

void main() {
    // Get normal (with normal mapping if available)
    vec3 normal;
    if (uHasNormal == 1) {
        vec3 normalMapValue = texture(uNormalMap, v_TexCoord).rgb;
        normalMapValue = normalMapValue * 2.0 - 1.0;
        normal = normalize(v_TBN * normalMapValue);
    } else {
        normal = normalize(v_Normal);
    }
    
    vec3 lightDir = normalize(uLightDirection);
    vec3 viewDir = normalize(v_ViewPos - v_FragPos);
    
    // Get albedo color
    vec3 albedo;
    if (uHasAlbedo == 1) {
        albedo = texture(uAlbedoMap, v_TexCoord).rgb;
    } else {
        albedo = uBaseColor.rgb;
        if (length(albedo) < 0.01) {
            albedo = vec3(0.7, 0.7, 0.7);
        }
    }
    
    // Get specular intensity
    float specularIntensity = 0.5;
    if (uHasSpecular == 1) {
        specularIntensity = texture(uSpecularMap, v_TexCoord).r;
    }
    
    // Get AO
    float ao = 1.0;
    if (uHasAO == 1) {
        ao = texture(uAOMap, v_TexCoord).r;
    }
    
    // Apply scene AO strength
    ao = mix(1.0, ao, uAOStrength);
    
    // Calculate lighting
    // Ambient
    vec3 ambient = uAmbientStrength * uLightColor * albedo * ao;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * uLightIntensity * albedo;
    
    // Specular (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), uShininess);
    vec3 specular = specularIntensity * spec * uLightColor * uLightIntensity;
    
    // Shadow calculation
    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }
    
    float shadowAttenuation = 1.0 - shadow * 0.65;
    
    // Combine lighting
    vec3 result = ambient + (diffuse + specular) * shadowAttenuation;
    
    FragColor = vec4(result, 1.0);
}
