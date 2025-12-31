#version 430 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform vec2 uTexelSize;

void main() {
    // 13-tap downsampling filter (Kawase)
    vec3 a = texture(uTexture, vTexCoord + uTexelSize * vec2(-1.0, -1.0)).rgb;
    vec3 b = texture(uTexture, vTexCoord + uTexelSize * vec2( 0.0, -1.0)).rgb;
    vec3 c = texture(uTexture, vTexCoord + uTexelSize * vec2( 1.0, -1.0)).rgb;
    vec3 d = texture(uTexture, vTexCoord + uTexelSize * vec2(-1.0,  0.0)).rgb;
    vec3 e = texture(uTexture, vTexCoord).rgb;
    vec3 f = texture(uTexture, vTexCoord + uTexelSize * vec2( 1.0,  0.0)).rgb;
    vec3 g = texture(uTexture, vTexCoord + uTexelSize * vec2(-1.0,  1.0)).rgb;
    vec3 h = texture(uTexture, vTexCoord + uTexelSize * vec2( 0.0,  1.0)).rgb;
    vec3 i = texture(uTexture, vTexCoord + uTexelSize * vec2( 1.0,  1.0)).rgb;

    vec3 color = e * 0.25;
    color += (b + d + f + h) * 0.125;
    color += (a + c + g + i) * 0.0625;

    FragColor = vec4(color, 1.0);
}
