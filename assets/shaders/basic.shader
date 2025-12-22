#shader vertex
#version 450

// Vertex inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_TexCoord;

// Uniform block for per-frame data
layout(std140, binding = 0) uniform PerFrameData {
    mat4 uView;
    mat4 uProj;
    mat4 uLightSpaceMatrix;
};

// Uniform block for per-object data
layout(std140, binding = 1) uniform PerObjectData {
    mat4 uModel;
    float uSpecularStrength;
};

// Vertex outputs
layout(location = 0) out vec3 v_Color;
layout(location = 1) out vec3 v_ViewPos;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec3 v_FragPos;
layout(location = 4) out vec4 v_LightSpacePos;
layout(location = 5) out float f_SpecularStrenght;
layout(location = 6) out vec2 v_TexCoord;

void main() {
    vec4 world_position = uModel * vec4(a_Position, 1.0);
    v_FragPos = world_position.xyz;
    f_SpecularStrenght = uSpecularStrength;
    v_Normal = mat3(transpose(inverse(uModel))) * a_Normal;
    v_ViewPos = inverse(uView)[3].xyz;
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_LightSpacePos = uLightSpaceMatrix * world_position;
    gl_Position = uProj * uView * world_position;
}

#shader fragment
#version 450

layout(location = 0) out vec4 color;

// Fragment inputs
layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec3 v_ViewPos;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec3 v_FragPos;
layout(location = 4) in vec4 v_LightSpacePos;
layout(location = 5) in float f_SpecularStrenght;
layout(location = 6) in vec2 v_TexCoord;

// Lighting uniforms
layout(std140, binding = 2) uniform LightingData {
    vec3 uLightDirection;
    float uLightIntensity;
    vec3 uLightColor;
    float uAmbientStrength;
    float uReceiveShadows;
    float uShadowsEnabled;
    float uAOStrength;
    float uAORadius;
};

// Texture samplers
layout(binding = 0) uniform sampler2D uTexture;
layout(binding = 7) uniform sampler2D uShadowMap; // Moved to slot 7

float CalculateAmbientOcclusion(vec3 normal, vec3 viewDir, vec3 fragPos) {
    float ao = 1.0;
    float NdotV = max(dot(normal, viewDir), 0.0);
    float edgeAO = pow(NdotV, 0.5);
    
    vec3 dNdx = dFdx(normal);
    vec3 dNdy = dFdy(normal);
    float curvature = length(dNdx) + length(dNdy);
    float curvatureAO = 1.0 - clamp(curvature * uAORadius * 2.0, 0.0, 0.8);
    
    vec3 lightDir = normalize(uLightDirection);
    float hemisphereOcclusion = 0.5 + 0.5 * dot(normal, vec3(0.0, 1.0, 0.0));
    
    vec3 dPosdx = dFdx(fragPos);
    vec3 dPosdy = dFdy(fragPos);
    vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
    float cavityFactor = dot(normal, faceNormal);
    float cavityAO = mix(0.7, 1.0, clamp(cavityFactor, 0.0, 1.0));
    
    ao = edgeAO * curvatureAO * hemisphereOcclusion * cavityAO;
    ao = mix(1.0, ao, uAOStrength);
    ao = max(ao, 0.15);
    
    return ao;
}

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
    
    // Sample texture and combine with vertex color
    vec4 texColor = texture(uTexture, v_TexCoord);
    vec3 objectColor = v_Color * texColor.rgb;
    
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(v_ViewPos - v_FragPos);
    float ao = CalculateAmbientOcclusion(normal, viewDir, v_FragPos);

    float shadow = 0.0;
    if (uReceiveShadows > 0.5 && uShadowsEnabled > 0.5) {
        shadow = CalculateShadow(v_LightSpacePos, normal, lightDir);
    }

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 128.0);
    vec3 specular = f_SpecularStrenght * spec * uLightColor;

    vec3 ambient = uLightColor * uAmbientStrength * ao;
    vec3 diffuse = uLightColor * diff * uLightIntensity;

    float shadowAttenuation = 1.0 - shadow * 0.65;
    vec3 lighting = ambient + (diffuse + specular) * shadowAttenuation * mix(1.0, ao, 0.3);
    vec3 result = lighting * objectColor;

    color = vec4(result, 1.0);
}
