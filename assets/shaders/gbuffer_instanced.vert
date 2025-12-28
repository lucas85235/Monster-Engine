#version 430 core

// G-Buffer vertex shader for instanced geometry
// Layout matches MeshManager: Position(0), Color(1), Normal(2)
// Instance buffer starts at location 3 (sequentially after base attributes)

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;

// Instance attributes - Mat4 uses 4 slots (3, 4, 5, 6), then Color at 7
layout(location = 3) in mat4 a_InstanceTransform;
layout(location = 7) in vec4 a_InstanceColor;  // Float4 in InstanceData

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec3 v_Color;

uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 worldPos = a_InstanceTransform * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(a_InstanceTransform)));
    v_Normal = normalize(normalMatrix * a_Normal);
    
    // Use instance color (xyz component of vec4)
    v_Color = a_InstanceColor.rgb;
    
    gl_Position = uProj * uView * worldPos;
}
