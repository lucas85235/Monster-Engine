#version 430 core

layout(location = 0) in vec3 aPos;

out vec3 v_TexCoords;

uniform mat4 uProjection;
uniform mat4 uView;

void main() {
    v_TexCoords = aPos;
    // Remove translation from view matrix to keep skybox at "infinity"
    mat4 viewNoTranslation = mat4(mat3(uView));
    vec4 pos = uProjection * viewNoTranslation * vec4(aPos, 1.0);
    // Set z = w so depth is always max (1.0) after perspective divide
    gl_Position = pos.xyww;
}
