#version 430 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec3 gEmissive;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Color;

void main() {
    gPosition = v_WorldPos;
    gNormal = normalize(v_Normal);
    gAlbedo = vec4(v_Color, 1.0);
    
    // FORCE BRIGHT BLUE EMISSIVE - different from regular gbuffer for identification
    gEmissive = vec3(0.0, 0.0, 2.0);
}
