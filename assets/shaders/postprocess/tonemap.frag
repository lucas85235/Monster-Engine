#version 430 core

out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uHDRTexture;
uniform float uExposure;
uniform float uGamma;
uniform int uTonemapOperator;

// ACES Filmic Tone Mapping
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Reinhard
vec3 Reinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

// Uncharted 2 filmic
vec3 Uncharted2Tonemap(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 Uncharted2(vec3 color) {
    float W = 11.2;
    float exposureBias = 2.0;
    vec3 curr = Uncharted2Tonemap(exposureBias * color);
    vec3 whiteScale = 1.0 / Uncharted2Tonemap(vec3(W));
    return curr * whiteScale;
}

// Neutral (simple S-curve)
vec3 Neutral(vec3 x) {
    const float a = 0.22;
    const float b = 0.30;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.01;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// AgX (simplified version)
vec3 AgXDefaultContrastApprox(vec3 x) {
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

vec3 AgX(vec3 color) {
    const mat3 agxInsetMat = mat3(
        0.842479, 0.0423516, 0.0423516,
        0.0784336, 0.878469, 0.0784336,
        0.0792237, 0.0791789, 0.879142
    );
    
    const mat3 agxOutsetMat = mat3(
        1.19687, -0.0528016, -0.0528016,
        -0.0980208, 1.15190, -0.0980208,
        -0.0990297, -0.0989611, 1.15107
    );
    
    vec3 col = agxInsetMat * color;
    col = clamp(col, 0.0, 1.0);
    col = AgXDefaultContrastApprox(col);
    col = agxOutsetMat * col;
    return clamp(col, 0.0, 1.0);
}

void main() {
    vec3 hdr = texture(uHDRTexture, vTexCoord).rgb;
    hdr *= uExposure;
    
    vec3 ldr;
    switch (uTonemapOperator) {
        case 0: ldr = ACESFilm(hdr); break;
        case 1: ldr = Reinhard(hdr); break;
        case 2: ldr = Uncharted2(hdr); break;
        case 3: ldr = Neutral(hdr); break;
        case 4: ldr = AgX(hdr); break;
        default: ldr = ACESFilm(hdr); break;
    }
    
    // Gamma correction
    ldr = pow(ldr, vec3(1.0 / uGamma));
    
    FragColor = vec4(ldr, 1.0);
}
