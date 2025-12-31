#version 430 core

out vec4 FragColor;

in vec3 v_TexCoords;

uniform samplerCube uSkybox;
uniform float uExposure;

// ACES tone mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Interleaved Gradient Noise for dithering - Jimenez 2014
float interleavedGradientNoise(vec2 screenPos) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

// Triangular dither for better color distribution
float triangularDither(float noise) {
    float n = noise * 2.0 - 1.0;
    return sign(n) * (1.0 - sqrt(1.0 - abs(n))) * 0.5;
}

void main() {
    vec3 color = texture(uSkybox, v_TexCoords).rgb;
    
    // Apply exposure
    color *= uExposure;
    
    // Tone mapping
    color = ACESFilm(color);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    // Dithering to reduce sky banding
    float noise = interleavedGradientNoise(gl_FragCoord.xy);
    float dither = triangularDither(noise);
    color += vec3(dither / 255.0);
    
    FragColor = vec4(color, 1.0);
}
