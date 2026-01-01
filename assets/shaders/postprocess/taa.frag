#version 460 core
// =============================================================================
// TAA - Temporal Anti-Aliasing
// =============================================================================
// Uses per-pixel motion vectors to blend history with current frame.
// Includes neighborhood clamping to reduce ghosting.

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uCurrentFrame;   // Current frame (jittered)
uniform sampler2D uHistoryFrame;   // Previous frame's TAA output
uniform sampler2D uMotionVectors;  // Screen-space motion vectors
uniform sampler2D uDepthTexture;   // Current frame depth

uniform vec2 uResolution;
uniform vec2 uJitter;              // Current frame jitter offset
uniform float uBlendFactor;        // History blend (default 0.9)

// YCoCg color space for better neighborhood clamping
vec3 RGBToYCoCg(vec3 rgb) {
    return vec3(
         0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b,
         0.5  * rgb.r              - 0.5  * rgb.b,
        -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b
    );
}

vec3 YCoCgToRGB(vec3 ycocg) {
    return vec3(
        ycocg.x + ycocg.y - ycocg.z,
        ycocg.x + ycocg.z,
        ycocg.x - ycocg.y - ycocg.z
    );
}

// Catmull-Rom spline for sharper history sampling
vec4 textureCatmullRom(sampler2D tex, vec2 uv) {
    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 pos = uv * texSize;
    vec2 center = floor(pos - 0.5) + 0.5;
    vec2 f = pos - center;
    vec2 f2 = f * f;
    vec2 f3 = f2 * f;
    
    vec2 w0 = f2 - 0.5 * (f3 + f);
    vec2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
    vec2 w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f;
    vec2 w3 = 0.5 * (f3 - f2);
    
    vec2 w12 = w1 + w2;
    vec2 tc0 = (center - 1.0) / texSize;
    vec2 tc12 = (center + w2 / w12) / texSize;
    vec2 tc3 = (center + 2.0) / texSize;
    
    return
        (texture(tex, vec2(tc0.x,  tc0.y)) * w0.x +
         texture(tex, vec2(tc12.x, tc0.y)) * w12.x +
         texture(tex, vec2(tc3.x,  tc0.y)) * w3.x) * w0.y +
        (texture(tex, vec2(tc0.x,  tc12.y)) * w0.x +
         texture(tex, vec2(tc12.x, tc12.y)) * w12.x +
         texture(tex, vec2(tc3.x,  tc12.y)) * w3.x) * w12.y +
        (texture(tex, vec2(tc0.x,  tc3.y)) * w0.x +
         texture(tex, vec2(tc12.x, tc3.y)) * w12.x +
         texture(tex, vec2(tc3.x,  tc3.y)) * w3.x) * w3.y;
}

void main() {
    vec2 texelSize = 1.0 / uResolution;
    
    // Remove jitter for sampling
    vec2 unjitteredUV = vTexCoord - uJitter;
    
    // Sample current frame
    vec3 current = texture(uCurrentFrame, unjitteredUV).rgb;
    
    // Get motion vector and compute history UV
    vec2 motion = texture(uMotionVectors, vTexCoord).rg;
    vec2 historyUV = vTexCoord - motion;
    
    // Sample history with Catmull-Rom for sharpness
    vec3 history = textureCatmullRom(uHistoryFrame, historyUV).rgb;
    
    // Neighborhood clamping in YCoCg space
    // Sample 3x3 neighborhood
    vec3 n0 = texture(uCurrentFrame, unjitteredUV + vec2(-1, -1) * texelSize).rgb;
    vec3 n1 = texture(uCurrentFrame, unjitteredUV + vec2( 0, -1) * texelSize).rgb;
    vec3 n2 = texture(uCurrentFrame, unjitteredUV + vec2( 1, -1) * texelSize).rgb;
    vec3 n3 = texture(uCurrentFrame, unjitteredUV + vec2(-1,  0) * texelSize).rgb;
    vec3 n4 = current;
    vec3 n5 = texture(uCurrentFrame, unjitteredUV + vec2( 1,  0) * texelSize).rgb;
    vec3 n6 = texture(uCurrentFrame, unjitteredUV + vec2(-1,  1) * texelSize).rgb;
    vec3 n7 = texture(uCurrentFrame, unjitteredUV + vec2( 0,  1) * texelSize).rgb;
    vec3 n8 = texture(uCurrentFrame, unjitteredUV + vec2( 1,  1) * texelSize).rgb;
    
    // Convert to YCoCg
    vec3 ycocg0 = RGBToYCoCg(n0);
    vec3 ycocg1 = RGBToYCoCg(n1);
    vec3 ycocg2 = RGBToYCoCg(n2);
    vec3 ycocg3 = RGBToYCoCg(n3);
    vec3 ycocg4 = RGBToYCoCg(n4);
    vec3 ycocg5 = RGBToYCoCg(n5);
    vec3 ycocg6 = RGBToYCoCg(n6);
    vec3 ycocg7 = RGBToYCoCg(n7);
    vec3 ycocg8 = RGBToYCoCg(n8);
    
    // Compute AABB
    vec3 minColor = min(ycocg0, min(ycocg1, min(ycocg2, min(ycocg3, 
                    min(ycocg4, min(ycocg5, min(ycocg6, min(ycocg7, ycocg8))))))));
    vec3 maxColor = max(ycocg0, max(ycocg1, max(ycocg2, max(ycocg3,
                    max(ycocg4, max(ycocg5, max(ycocg6, max(ycocg7, ycocg8))))))));
    
    // Clamp history to neighborhood
    vec3 historyYCoCg = RGBToYCoCg(history);
    vec3 clampedYCoCg = clamp(historyYCoCg, minColor, maxColor);
    vec3 clampedHistory = YCoCgToRGB(clampedYCoCg);
    
    // Detect disocclusion (large motion or history outside bounds)
    float confidence = 1.0;
    
    // Reduce blend factor at screen edges
    vec2 edgeDist = min(historyUV, 1.0 - historyUV);
    float edgeFactor = min(edgeDist.x, edgeDist.y) * 10.0;
    edgeFactor = clamp(edgeFactor, 0.0, 1.0);
    confidence *= edgeFactor;
    
    // Reduce blend for large motion
    float motionLength = length(motion * uResolution);
    confidence *= clamp(1.0 - motionLength * 0.1, 0.2, 1.0);
    
    // Final blend
    float blend = uBlendFactor * confidence;
    vec3 result = mix(current, clampedHistory, blend);
    
    FragColor = vec4(result, 1.0);
}
