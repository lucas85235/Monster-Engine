#version 430 core

// Layout must match MeshManager: Position(0), Color(1), Normal(2)
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Color;
layout(location = 2) in vec3 a_Normal;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec3 v_Color;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 worldPos = uModel * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    v_Normal = normalize(normalMatrix * a_Normal);
    
    v_Color = a_Color;
    
    gl_Position = uProj * uView * worldPos;
}
