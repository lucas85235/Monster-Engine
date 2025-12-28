#version 430 core

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedo;
layout(location = 3) out vec4 gEmissive;

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

uniform sampler2D uAlbedoTex;
uniform sampler2D uEmissiveTex;
uniform bool uHasAlbedoTex;
uniform bool uHasEmissiveTex;
uniform vec3 uEmissiveColor;
uniform float uEmissiveFactor;

void main() {
    gPosition = vec4(v_WorldPos, 1.0);
    gNormal = vec4(normalize(v_Normal), 0.0);
    
    if (uHasAlbedoTex) {
        gAlbedo = texture(uAlbedoTex, v_TexCoord);
    } else {
        gAlbedo = vec4(0.8, 0.8, 0.8, 1.0);
    }
    
    // Use emissive texture if available, otherwise use uniform color
    if (uHasEmissiveTex) {
        gEmissive = vec4(texture(uEmissiveTex, v_TexCoord).rgb * uEmissiveFactor, 1.0);
    } else {
        gEmissive = vec4(uEmissiveColor * uEmissiveFactor, 1.0);
    }
}
