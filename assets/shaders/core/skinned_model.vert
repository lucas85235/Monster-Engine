#version 330 core

// Vertex attributes
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;
layout(location = 4) in vec3 a_Bitangent;
layout(location = 5) in ivec4 a_BoneIds;
layout(location = 6) in vec4 a_BoneWeights;

// Uniforms
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;

// Skeletal animation
const int MAX_BONES = 128;
uniform mat4 uBoneMatrices[MAX_BONES];
uniform bool uHasBones;

// Outputs
out vec3 v_FragPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_ViewPos;
out vec4 v_LightSpacePos;
out mat3 v_TBN;

void main() {
    mat4 boneTransform = mat4(1.0);
    
    if (uHasBones) {
        boneTransform = mat4(0.0);
        
        for (int i = 0; i < 4; i++) {
            int boneId = a_BoneIds[i];
            float weight = a_BoneWeights[i];
            
            if (boneId >= 0 && boneId < MAX_BONES && weight > 0.0) {
                boneTransform += uBoneMatrices[boneId] * weight;
            }
        }
        
        // Handle case where no weights were applied
        if (boneTransform[0][0] == 0.0 && boneTransform[1][1] == 0.0 && 
            boneTransform[2][2] == 0.0 && boneTransform[3][3] == 0.0) {
            boneTransform = mat4(1.0);
        }
    }
    
    // Apply bone transform then model transform
    vec4 skinnedPosition = boneTransform * vec4(a_Position, 1.0);
    vec4 worldPosition = uModel * skinnedPosition;
    v_FragPos = worldPosition.xyz;
    
    // Transform normals with bone and model matrices
    mat3 boneNormalMatrix = mat3(transpose(inverse(boneTransform)));
    mat3 modelNormalMatrix = mat3(transpose(inverse(uModel)));
    mat3 combinedNormalMatrix = modelNormalMatrix * boneNormalMatrix;
    
    v_Normal = normalize(combinedNormalMatrix * a_Normal);
    
    v_TexCoord = a_TexCoord;
    v_ViewPos = inverse(uView)[3].xyz;
    v_LightSpacePos = uLightSpaceMatrix * worldPosition;
    
    // TBN matrix for normal mapping
    vec3 T = normalize(combinedNormalMatrix * a_Tangent);
    vec3 B = normalize(combinedNormalMatrix * a_Bitangent);
    vec3 N = v_Normal;
    v_TBN = mat3(T, B, N);
    
    gl_Position = uProj * uView * worldPosition;
}
