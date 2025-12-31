#version 430 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uFilterRadius;

void main() {
    // 9-tap tent filter (3x3 kernel)
    float x = uFilterRadius;
    float y = uFilterRadius;

    vec3 a = texture(uTexture, vTexCoord + vec2(-x, -y)).rgb;
    vec3 b = texture(uTexture, vTexCoord + vec2( 0, -y)).rgb;
    vec3 c = texture(uTexture, vTexCoord + vec2( x, -y)).rgb;
    vec3 d = texture(uTexture, vTexCoord + vec2(-x,  0)).rgb;
    vec3 e = texture(uTexture, vTexCoord).rgb;
    vec3 f = texture(uTexture, vTexCoord + vec2( x,  0)).rgb;
    vec3 g = texture(uTexture, vTexCoord + vec2(-x,  y)).rgb;
    vec3 h = texture(uTexture, vTexCoord + vec2( 0,  y)).rgb;
    vec3 i = texture(uTexture, vTexCoord + vec2( x,  y)).rgb;

    vec3 color = e * 4.0;
    color += (b + d + f + h) * 2.0;
    color += (a + c + g + i);
    color /= 16.0;

    FragColor = vec4(color, 1.0);
}
