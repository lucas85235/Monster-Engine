#version 330 core

// Vertex layout for loaded models
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;

// Transform uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;

// Outputs to fragment shader
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_ViewPos;
out vec4 v_LightSpacePos;
out mat3 v_TBN;

void main() {
    vec4 worldPosition = uModel * vec4(a_Position, 1.0);
    v_WorldPos = worldPosition.xyz;
    
    // Normal matrix for proper normal transformation (handles non-uniform scale)
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    v_Normal = normalize(normalMatrix * a_Normal);
    
    v_TexCoord = a_TexCoord;
    v_ViewPos = inverse(uView)[3].xyz;
    v_LightSpacePos = uLightSpaceMatrix * worldPosition;
    
    // TBN matrix for normal mapping (tangent space to world space)
    vec3 T = normalize(normalMatrix * a_Tangent);
    vec3 B = normalize(normalMatrix * a_Bitangent);
    vec3 N = v_Normal;
    v_TBN = mat3(T, B, N);
    
    gl_Position = uProj * uView * worldPosition;
}
