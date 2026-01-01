#version 430 core

out float FragColor;
in vec2 vTexCoord;

uniform sampler2D uSSAOTexture;
uniform int uBlurSize;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(uSSAOTexture, 0));
    float result = 0.0;
    int samples = 0;
    
    for (int x = -uBlurSize; x <= uBlurSize; ++x) {
        for (int y = -uBlurSize; y <= uBlurSize; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(uSSAOTexture, vTexCoord + offset).r;
            samples++;
        }
    }
    
    FragColor = result / float(samples);
}
