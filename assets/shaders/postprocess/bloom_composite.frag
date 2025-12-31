#version 430 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;
uniform float uIntensity;

void main() {
    vec3 scene = texture(uSceneTexture, vTexCoord).rgb;
    vec3 bloom = texture(uBloomTexture, vTexCoord).rgb;
    
    FragColor = vec4(scene + bloom * uIntensity, 1.0);
}
