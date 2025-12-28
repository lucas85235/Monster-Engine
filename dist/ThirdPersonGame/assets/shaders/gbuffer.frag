#version 430 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gEmissive;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec3 v_Color;

uniform vec3 uEmissiveColor;
uniform float uEmissiveFactor;

void main() {
    gPosition = vec4(v_WorldPos, 1.0);
    gNormal = vec4(normalize(v_Normal), 0.0);
    gAlbedo = vec4(v_Color, 1.0);
    
    // Use emissive color from uniform (default 0 = no emission)
    gEmissive = vec4(uEmissiveColor * uEmissiveFactor, 1.0);
}
