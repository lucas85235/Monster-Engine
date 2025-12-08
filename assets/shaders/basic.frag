#version 330 core
layout(location = 0) out vec4 color;

in vec3 v_Color;
in vec3 v_ViewPos;
in vec3 v_Normal;
in vec3 v_FragPos;
in vec4 v_LightSpacePos;
in float f_SpecularStrenght;

uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;
uniform float uAmbientStrength;
uniform sampler2D uShadowMap;
uniform float uReceiveShadows;
uniform float uShadowsEnabled;

// Ambient Occlusion parameters
uniform float uAOStrength;      // Overall AO intensity (0.0 - 1.0, default 0.5)
uniform float uAORadius;        // Sampling radius for curvature estimation (default 1.0)

vec3 Saturate(vec3 value){
 return vec3(clamp(value.x,0.0,1.0),clamp(value.y,0.0,1.0),clamp(value.z,0.0,1.0));
}

// Calculate approximate ambient occlusion using hemisphere sampling approximation
float CalculateAmbientOcclusion(vec3 normal, vec3 viewDir, vec3 fragPos) {
    float ao = 1.0;
    
    // 1. Edge darkening (inverted fresnel effect)
    // Surfaces facing away from the camera tend to be in concave areas
    float NdotV = max(dot(normal, viewDir), 0.0);
    float edgeAO = pow(NdotV, 0.5);  // Softer falloff for edge darkening
    
    // 2. Curvature-based AO estimation using screen-space derivatives
    // Approximate local curvature by checking normal variation
    vec3 dNdx = dFdx(normal);
    vec3 dNdy = dFdy(normal);
    float curvature = length(dNdx) + length(dNdy);
    
    // Higher curvature = more occlusion (concave surfaces)
    float curvatureAO = 1.0 - clamp(curvature * uAORadius * 2.0, 0.0, 0.8);
    
    // 3. Hemisphere occlusion approximation
    // Use light direction and normal to estimate self-occlusion
    vec3 lightDir = normalize(uLightDirection);
    float hemisphereOcclusion = 0.5 + 0.5 * dot(normal, vec3(0.0, 1.0, 0.0));  // Sky contribution
    
    // 4. Cavity detection using position derivatives
    vec3 dPosdx = dFdx(fragPos);
    vec3 dPosdy = dFdy(fragPos);
    vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
    float cavityFactor = dot(normal, faceNormal);
    float cavityAO = mix(0.7, 1.0, clamp(cavityFactor, 0.0, 1.0));
    
    // Combine all AO factors
    ao = edgeAO * curvatureAO * hemisphereOcclusion * cavityAO;
    
    // Apply strength and ensure minimum visibility
    ao = mix(1.0, ao, uAOStrength);
    ao = max(ao, 0.15);  // Prevent fully black areas
    
    return ao;
}

float CalculateShadow(vec4 lightSpacePos, vec3 normal, vec3 lightDir) {
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    // Early out for fragments outside shadow frustum
    if (projCoords.z > 1.0 || projCoords.z < 0.0)
        return 0.0;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    // Slope-scaled depth bias - reduced to minimize peter-panning
    float ndotl = max(dot(normal, lightDir), 0.0);
    float bias = max(0.0003 * (1.0 - ndotl), 0.00005);
    
    // 5x5 PCF for smoother shadow edges
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;
    
    // Smooth falloff at shadow map edges
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
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(uLightDirection);
    vec3 objectColor = v_Color;
    float diff = max(dot(normal, lightDir), 0.0);

    // Calculate view direction (needed for AO and specular)
    vec3 viewDir = normalize(v_ViewPos - v_FragPos);

    // Calculate Ambient Occlusion
    float ao = CalculateAmbientOcclusion(normal, viewDir, v_FragPos);

    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }

    // Specular reflection (Blinn-Phong)
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 128.0);
    vec3 specular = f_SpecularStrenght * spec * uLightColor;

    // Lighting components - apply AO to ambient
    vec3 ambient = uLightColor * uAmbientStrength * ao;
    vec3 diffuse = uLightColor * diff * uLightIntensity;

    // Shadow attenuation (preserves ambient, darkens diffuse and specular)
    float shadowAttenuation = 1.0 - shadow * 0.65;
    
    // Final lighting - AO also affects indirect light contribution
    vec3 lighting = ambient + (diffuse + specular) * shadowAttenuation * mix(1.0, ao, 0.3);
    // vec3 result = lighting * vec3(0.4, 0.4, 0.4);
    vec3 result = lighting * objectColor;

    color = vec4(result, 1.0);
}