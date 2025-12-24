#version 450

// Vertex inputs
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;
layout(location = 3) in vec2 a_TexCoord;

// Per-instance data (Mat4 uses locations 4-7, Color uses 8)
layout(location = 4) in mat4 a_InstanceTransform;
layout(location = 8) in vec4 a_InstanceColor;

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
    // Use instance transform if available (check if not identity/zero)
    mat4 instanceModel = a_InstanceTransform;
    
    // Apply instance transform to vertex position
    vec4 worldPos = instanceModel * vec4(a_Position, 1.0);
    gl_Position = proj * view * worldPos;
    
    // Use instance color if set (not white), otherwise use vertex color
    if (a_InstanceColor.rgb == vec3(1.0, 1.0, 1.0) || a_InstanceColor == vec4(0.0)) {
        v_Color = a_Color;
    } else {
        v_Color = a_InstanceColor.rgb;
    }
    
    v_Normal = mat3(transpose(inverse(instanceModel))) * a_Normal;
    v_TexCoord = a_TexCoord;
}
