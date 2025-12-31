#version 430 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uTexture;
uniform float uThreshold;
uniform float uSoftKnee;

vec3 prefilter(vec3 c) {
    float brightness = max(c.r, max(c.g, c.b));
    float knee = uThreshold * uSoftKnee;
    float soft = brightness - uThreshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - uThreshold);
    contribution /= max(brightness, 0.00001);
    return c * contribution;
}

void main() {
    vec3 color = texture(uTexture, vTexCoord).rgb;
    FragColor = vec4(prefilter(color), 1.0);
}
