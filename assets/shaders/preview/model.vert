#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in ivec4 aBoneIds;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int hasBones;
uniform mat4 boneMatrices[128];

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main() {
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    
    if (hasBones == 1) {
        for (int i = 0; i < 4; ++i) {
            if (aBoneIds[i] >= 0 && aBoneIds[i] < 128) {
                float weight = aBoneWeights[i];
                if (weight > 0.0) {
                    mat4 boneMatrix = boneMatrices[aBoneIds[i]];
                    totalPosition += weight * (boneMatrix * vec4(aPos, 1.0));
                    totalNormal += weight * (mat3(boneMatrix) * aNormal);
                }
            }
        }
        
        if (length(totalNormal) > 0.001) {
            totalNormal = normalize(totalNormal);
        } else {
            totalNormal = aNormal;
        }
    } else {
        totalPosition = vec4(aPos, 1.0);
        totalNormal = aNormal;
    }
    
    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
