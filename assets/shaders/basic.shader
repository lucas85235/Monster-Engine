#shader vertex
#version 450

// Vertex inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_TexCoord;

// UBO binding 0 - scene-wide data (matches VulkanDevice::UniformBufferObject)
layout(std140, binding = 0) uniform SceneData {
    mat4 uView;
    mat4 uProj;
    vec4 uLightDir;      // direction.xyz + intensity
    vec4 uLightColor;    // color.rgb + unused
    vec4 uViewPos;       // position.xyz + unused
};

// Push constants (matches VulkanDevice::PushConstants)
layout(push_constant) uniform PushConstants {
    mat4 uModel;
    vec4 materialColor;   // albedo.rgb + metallic
    vec4 materialProps;   // roughness, ao, emission, flags
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
    f_SpecularStrenght = materialProps.x;  // Use roughness as specular for now
    v_Normal = mat3(transpose(inverse(uModel))) * a_Normal;
    v_ViewPos = uViewPos.xyz;
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    // Light space matrix not in UBO yet, use identity for now
    v_LightSpacePos = vec4(0.0);
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

// UBO binding 0 - scene-wide data
layout(std140, binding = 0) uniform SceneData {
    mat4 uView;
    mat4 uProj;
    vec4 uLightDir;
    vec4 uLightColor;
    vec4 uViewPos;
};

// Push constants
layout(push_constant) uniform PushConstants {
    mat4 uModel;
    vec4 materialColor;
    vec4 materialProps;
};

// Texture samplers
layout(binding = 1) uniform sampler2D uTexture;
layout(binding = 7) uniform sampler2D uShadowMap;

float CalculateAmbientOcclusion(vec3 normal, vec3 viewDir, vec3 fragPos) {
    float ao = 1.0;
    float NdotV = max(dot(normal, viewDir), 0.0);
    float edgeAO = pow(NdotV, 0.5);
    
    vec3 dNdx = dFdx(normal);
    vec3 dNdy = dFdy(normal);
    float curvature = length(dNdx) + length(dNdy);
    float curvatureAO = 1.0 - clamp(curvature * 2.0, 0.0, 0.8);
    
    vec3 lightDir = normalize(uLightDir.xyz);
    float hemisphereOcclusion = 0.5 + 0.5 * dot(normal, vec3(0.0, 1.0, 0.0));
    
    vec3 dPosdx = dFdx(fragPos);
    vec3 dPosdy = dFdy(fragPos);
    vec3 faceNormal = normalize(cross(dPosdx, dPosdy));
    float cavityFactor = dot(normal, faceNormal);
    float cavityAO = mix(0.7, 1.0, clamp(cavityFactor, 0.0, 1.0));
    
    ao = edgeAO * curvatureAO * hemisphereOcclusion * cavityAO;
    float aoStrength = 0.5;  // Default AO strength
    ao = mix(1.0, ao, aoStrength);
    ao = max(ao, 0.15);
    
    return ao;
}

void main() {
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(uLightDir.xyz);
    float lightIntensity = uLightDir.w;
    float ambientStrength = 0.15;
    
    // Sample texture and combine with vertex color
    vec4 texColor = texture(uTexture, v_TexCoord);
    vec3 objectColor = v_Color * texColor.rgb;
    
    // Apply material color if set
    if (length(materialColor.rgb) > 0.01) {
        objectColor = materialColor.rgb;
    }
    
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(v_ViewPos - v_FragPos);
    float ao = CalculateAmbientOcclusion(normal, viewDir, v_FragPos);

    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 128.0);
    vec3 specular = f_SpecularStrenght * spec * uLightColor.rgb;

    vec3 ambient = uLightColor.rgb * ambientStrength * ao;
    vec3 diffuse = uLightColor.rgb * diff * lightIntensity;

    vec3 lighting = ambient + (diffuse + specular) * mix(1.0, ao, 0.3);
    vec3 result = lighting * objectColor;

    color = vec4(result, 1.0);
}
