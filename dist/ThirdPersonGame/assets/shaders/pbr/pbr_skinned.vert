#version 330 core

// Vertex layout for skinned meshes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in ivec4 a_BoneIDs;
layout(location = 6) in vec4 a_Weights;

// Transform uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;

// Bone matrices
const int MAX_BONES = 100;
uniform mat4 uBones[MAX_BONES];

// Outputs to fragment shader
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_ViewPos;
out vec4 v_LightSpacePos;
out mat3 v_TBN;

void main() {
    // Compute skinned position and normal
    mat4 boneTransform = mat4(0.0);
    
    for (int i = 0; i < 4; i++) {
        if (a_BoneIDs[i] >= 0 && a_BoneIDs[i] < MAX_BONES) {
            boneTransform += uBones[a_BoneIDs[i]] * a_Weights[i];
        }
    }
    
    // If no bones affect this vertex, use identity
    if (a_Weights[0] + a_Weights[1] + a_Weights[2] + a_Weights[3] < 0.01) {
        boneTransform = mat4(1.0);
    }
    
    vec4 skinnedPosition = boneTransform * vec4(a_Position, 1.0);
    vec3 skinnedNormal = mat3(boneTransform) * a_Normal;
    vec3 skinnedTangent = mat3(boneTransform) * a_Tangent;
    vec3 skinnedBitangent = mat3(boneTransform) * a_Bitangent;
    
    vec4 worldPosition = uModel * skinnedPosition;
    v_WorldPos = worldPosition.xyz;
    
    // Normal matrix for proper normal transformation
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    v_Normal = normalize(normalMatrix * skinnedNormal);
    
    v_TexCoord = a_TexCoord;
    v_ViewPos = inverse(uView)[3].xyz;
    v_LightSpacePos = uLightSpaceMatrix * worldPosition;
    
    // TBN matrix for normal mapping
    vec3 T = normalize(normalMatrix * skinnedTangent);
    vec3 B = normalize(normalMatrix * skinnedBitangent);
    vec3 N = v_Normal;
    v_TBN = mat3(T, B, N);
    
    gl_Position = uProj * uView * worldPosition;
}
