#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;

// Per-instance data (Mat4 uses locations 3-6, Color uses 7)
layout(location = 3) in mat4 a_InstanceTransform;
layout(location = 7) in vec4 a_InstanceColor;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;
uniform float uSpecularStrength;

out vec3 v_Color;
out vec3 v_ViewPos;
out vec3 v_Normal;
out vec3 v_FragPos;
out vec4 v_LightSpacePos;
out float f_SpecularStrenght;

void main() {
    mat4 model = a_InstanceTransform;
    vec4 world_position = model * vec4(a_Position, 1.0);
    v_FragPos = world_position.xyz;

    f_SpecularStrenght = uSpecularStrength;

    v_Normal = mat3(transpose(inverse(model))) * a_Normal;
    v_ViewPos = inverse(uView)[3].xyz;
    
    // Blend vertex color with instance color
    v_Color = a_Color * a_InstanceColor.rgb;
    
    v_LightSpacePos = uLightSpaceMatrix * world_position;

    gl_Position = uProj * uView * world_position;
}
