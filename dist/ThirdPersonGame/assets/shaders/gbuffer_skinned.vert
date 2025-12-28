#version 430 core

// G-Buffer vertex shader for skinned models
// Uses bone matrices to transform vertices

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in ivec4 a_BoneIDs;
layout(location = 4) in vec4 a_BoneWeights;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uModel;

const int MAX_BONES = 100;
uniform mat4 uBoneMatrices[MAX_BONES];
uniform bool uHasBones;

void main() {
    vec4 localPos;
    vec3 localNormal;
    
    if (uHasBones) {
        mat4 boneTransform = mat4(0.0);
        float totalWeight = 0.0;
        
        for (int i = 0; i < 4; i++) {
            int boneId = a_BoneIDs[i];
            float weight = a_BoneWeights[i];
            
            if (boneId >= 0 && boneId < MAX_BONES && weight > 0.0) {
                boneTransform += uBoneMatrices[boneId] * weight;
                totalWeight += weight;
            }
        }
        
        if (totalWeight > 0.0) {
            boneTransform /= totalWeight;
        } else {
            boneTransform = mat4(1.0);
        }
        
        localPos = boneTransform * vec4(a_Position, 1.0);
        localNormal = mat3(boneTransform) * a_Normal;
    } else {
        localPos = vec4(a_Position, 1.0);
        localNormal = a_Normal;
    }
    
    vec4 worldPos = uModel * localPos;
    v_WorldPos = worldPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(uModel)));
    v_Normal = normalize(normalMatrix * localNormal);
    
    v_TexCoord = a_TexCoord;
    
    gl_Position = uProj * uView * worldPos;
}
