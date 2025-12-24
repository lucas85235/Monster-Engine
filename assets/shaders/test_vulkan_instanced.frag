#version 450

// Fragment inputs
layout(location = 0) in vec3 v_Color;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;

// Push constants (matches VulkanDevice::PushConstants)
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 materialColor;
    vec4 materialProps;
};

// Texture samplers
layout(binding = 1) uniform sampler2D uTexture;

// Output
layout(location = 0) out vec4 fragColor;

void main() {
    // Simple solid color output for debugging
    vec3 ambient = vec3(0.1);
    
    // Light from above
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.5));
    float diff = max(dot(normalize(v_Normal), lightDir), 0.0);
    
    // Use vertex color (which comes from instance color in instanced rendering)
    vec3 baseColor = length(v_Color) > 0.01 ? v_Color : vec3(1.0, 0.3, 0.1);
    
    vec3 result = baseColor * (ambient + diff * 0.9);
    fragColor = vec4(result, 1.0);
}
