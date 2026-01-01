#version 460 core
// =============================================================================
// Color Grading Post-Process Shader
// =============================================================================
// Applies exposure, contrast, saturation, white balance, and shadows/highlights.

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;

// Color grading parameters
uniform float uExposure;        // Exposure multiplier (default 1.0)
uniform float uContrast;        // Contrast adjustment (default 1.0)
uniform float uSaturation;      // Saturation (0 = grayscale, 1 = normal, >1 = oversaturated)
uniform float uBrightness;      // Brightness offset (default 0.0)

// White balance
uniform float uTemperature;     // -1 to 1 (negative = cooler/blue, positive = warmer/orange)
uniform float uTint;            // -1 to 1 (negative = green, positive = magenta)

// Shadows/Highlights/Midtones
uniform float uShadows;         // Shadow adjustment (-1 to 1)
uniform float uHighlights;      // Highlight adjustment (-1 to 1)

// Vignette
uniform float uVignetteIntensity;   // 0 = off, 1 = full
uniform float uVignetteFalloff;     // Falloff curve (default 0.5)

// Color luminance weights (Rec. 709)
const vec3 LUMINANCE_WEIGHTS = vec3(0.2126, 0.7152, 0.0722);

float getLuminance(vec3 color) {
    return dot(color, LUMINANCE_WEIGHTS);
}

// Apply white balance using temperature/tint
vec3 applyWhiteBalance(vec3 color, float temperature, float tint) {
    // Simple chromatic adaptation
    // Temperature: shift between orange (warm) and blue (cool)
    // Tint: shift between green and magenta
    
    mat3 lmsFromRgb = mat3(
        0.4122214708, 0.5363325363, 0.0514459929,
        0.2119034982, 0.6806995451, 0.1073969566,
        0.0883024619, 0.2817188376, 0.6299787005
    );
    
    mat3 rgbFromLms = mat3(
        4.0767416621, -3.3077115913, 0.2309699292,
        -1.2684380046, 2.6097574011, -0.3413193965,
        -0.0041960863, -0.7034186147, 1.7076147010
    );
    
    vec3 lms = lmsFromRgb * color;
    
    // Temperature affects L and S channels
    lms.x *= 1.0 + temperature * 0.1;
    lms.z *= 1.0 - temperature * 0.1;
    
    // Tint affects M channel
    lms.y *= 1.0 + tint * 0.05;
    
    return rgbFromLms * lms;
}

// Apply contrast using S-curve
vec3 applyContrast(vec3 color, float contrast) {
    // Center around 0.5 (mid-gray in linear)
    float midpoint = 0.18;  // 18% gray (perceptual middle)
    return (color - midpoint) * contrast + midpoint;
}

// Apply saturation
vec3 applySaturation(vec3 color, float saturation) {
    float lum = getLuminance(color);
    return mix(vec3(lum), color, saturation);
}

// Shadow/Highlight adjustment
vec3 applyShadowsHighlights(vec3 color, float shadows, float highlights) {
    float lum = getLuminance(color);
    
    // Shadow adjustment (affects dark areas)
    float shadowFactor = 1.0 - lum;
    shadowFactor = shadowFactor * shadowFactor;  // Square for smoother falloff
    color += color * shadows * shadowFactor * 0.5;
    
    // Highlight adjustment (affects bright areas)
    float highlightFactor = lum;
    highlightFactor = highlightFactor * highlightFactor;
    color += color * highlights * highlightFactor * 0.5;
    
    return color;
}

// Vignette effect
float computeVignette(vec2 uv, float intensity, float falloff) {
    vec2 centered = uv - 0.5;
    float dist = length(centered);
    float vignette = 1.0 - smoothstep(falloff, falloff + 0.5, dist);
    return mix(1.0, vignette, intensity);
}

void main() {
    vec3 color = texture(uSceneTexture, vTexCoord).rgb;
    
    // Apply exposure
    color *= uExposure;
    
    // Apply white balance
    if (abs(uTemperature) > 0.001 || abs(uTint) > 0.001) {
        color = applyWhiteBalance(color, uTemperature, uTint);
    }
    
    // Apply contrast
    if (abs(uContrast - 1.0) > 0.001) {
        color = applyContrast(color, uContrast);
    }
    
    // Apply brightness
    color += uBrightness;
    
    // Apply saturation
    if (abs(uSaturation - 1.0) > 0.001) {
        color = applySaturation(color, uSaturation);
    }
    
    // Apply shadows/highlights
    if (abs(uShadows) > 0.001 || abs(uHighlights) > 0.001) {
        color = applyShadowsHighlights(color, uShadows, uHighlights);
    }
    
    // Apply vignette
    if (uVignetteIntensity > 0.001) {
        float vignette = computeVignette(vTexCoord, uVignetteIntensity, uVignetteFalloff);
        color *= vignette;
    }
    
    // Clamp to valid range
    color = max(color, vec3(0.0));
    
    FragColor = vec4(color, 1.0);
}
