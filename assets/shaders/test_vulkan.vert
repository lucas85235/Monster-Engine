#version 450

// Vertex inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_TexCoord;

// UBO binding 0 - scene-wide data (matches VulkanDevice::UniformBufferObject)
layout(std140, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
};

// Push constants (matches VulkanDevice::PushConstants)
layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 materialColor;
    vec4 materialProps;
};

// Vertex outputs
layout(location = 0) out vec3 v_Color;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;

void main() {
    vec4 worldPos = model * vec4(a_Position, 1.0);
    gl_Position = proj * view * worldPos;
    v_Color = a_Color;
    v_Normal = mat3(transpose(inverse(model))) * a_Normal;
    v_TexCoord = a_TexCoord;
}
