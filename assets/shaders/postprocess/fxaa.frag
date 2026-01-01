#version 460 core
// =============================================================================
// FXAA 3.11 - Fast Approximate Anti-Aliasing
// =============================================================================
// NVIDIA's FXAA implementation adapted for OpenGL.
// Quality preset 39 (high quality).

out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;
uniform vec2 uTexelSize;  // 1.0 / resolution

// Quality settings (preset 39 - high quality)
#define FXAA_EDGE_THRESHOLD      (1.0/8.0)
#define FXAA_EDGE_THRESHOLD_MIN  (1.0/24.0)
#define FXAA_SEARCH_STEPS        16
#define FXAA_SEARCH_ACCELERATION 1
#define FXAA_SEARCH_THRESHOLD    (1.0/4.0)
#define FXAA_SUBPIX              1
#define FXAA_SUBPIX_CAP          (3.0/4.0)
#define FXAA_SUBPIX_TRIM         (1.0/4.0)
#define FXAA_SUBPIX_TRIM_SCALE   (1.0/(1.0 - FXAA_SUBPIX_TRIM))

// Luminance calculation (using green as approximation for speed)
float FxaaLuma(vec3 rgb) {
    return rgb.g * (0.587/0.299) + rgb.r;
}

// More accurate luminance
float FxaaLumaAccurate(vec3 rgb) {
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

vec3 FxaaFilter(sampler2D tex, vec2 uv, vec2 texelSize) {
    // Sample the neighborhood
    vec3 rgbNW = textureLodOffset(tex, uv, 0.0, ivec2(-1, -1)).rgb;
    vec3 rgbNE = textureLodOffset(tex, uv, 0.0, ivec2( 1, -1)).rgb;
    vec3 rgbSW = textureLodOffset(tex, uv, 0.0, ivec2(-1,  1)).rgb;
    vec3 rgbSE = textureLodOffset(tex, uv, 0.0, ivec2( 1,  1)).rgb;
    vec3 rgbM  = textureLod(tex, uv, 0.0).rgb;
    
    // Convert to luminance
    float lumaNW = FxaaLuma(rgbNW);
    float lumaNE = FxaaLuma(rgbNE);
    float lumaSW = FxaaLuma(rgbSW);
    float lumaSE = FxaaLuma(rgbSE);
    float lumaM  = FxaaLuma(rgbM);
    
    // Find min/max luma in local neighborhood
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    // Compute edge contrast
    float lumaRange = lumaMax - lumaMin;
    
    // Skip if contrast is low (not an edge)
    if (lumaRange < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) {
        return rgbM;
    }
    
    // Sample additional neighbors for edge direction
    vec3 rgbN = textureLodOffset(tex, uv, 0.0, ivec2(0, -1)).rgb;
    vec3 rgbW = textureLodOffset(tex, uv, 0.0, ivec2(-1, 0)).rgb;
    vec3 rgbE = textureLodOffset(tex, uv, 0.0, ivec2(1, 0)).rgb;
    vec3 rgbS = textureLodOffset(tex, uv, 0.0, ivec2(0, 1)).rgb;
    
    float lumaN = FxaaLuma(rgbN);
    float lumaW = FxaaLuma(rgbW);
    float lumaE = FxaaLuma(rgbE);
    float lumaS = FxaaLuma(rgbS);
    
    // Compute subpix blending factor
    float lumaL = (lumaN + lumaS + lumaE + lumaW) * 0.25;
    float rangeL = abs(lumaL - lumaM);
    float blendL = max(0.0, (rangeL / lumaRange) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
    blendL = min(FXAA_SUBPIX_CAP, blendL);
    
    // Compute edge direction
    float edgeHorz = abs((lumaNW + lumaNE) - 2.0 * lumaN) +
                     abs((lumaW  + lumaE)  - 2.0 * lumaM) * 2.0 +
                     abs((lumaSW + lumaSE) - 2.0 * lumaS);
    
    float edgeVert = abs((lumaNW + lumaSW) - 2.0 * lumaW) +
                     abs((lumaN  + lumaS)  - 2.0 * lumaM) * 2.0 +
                     abs((lumaNE + lumaSE) - 2.0 * lumaE);
    
    bool horzSpan = edgeHorz >= edgeVert;
    float lengthSign = horzSpan ? texelSize.y : texelSize.x;
    
    // Select edge end-points
    float lumaPosN, lumaNegN;
    if (horzSpan) {
        lumaPosN = lumaN;
        lumaNegN = lumaS;
    } else {
        lumaPosN = lumaE;
        lumaNegN = lumaW;
    }
    
    float gradientN = abs(lumaPosN - lumaM);
    float gradientS = abs(lumaNegN - lumaM);
    
    if (gradientS > gradientN) {
        lengthSign = -lengthSign;
    }
    
    // Step along edge
    vec2 posN;
    vec2 offNP;
    
    if (horzSpan) {
        posN = vec2(uv.x, uv.y + lengthSign * 0.5);
        offNP = vec2(texelSize.x, 0.0);
    } else {
        posN = vec2(uv.x + lengthSign * 0.5, uv.y);
        offNP = vec2(0.0, texelSize.y);
    }
    
    // Search for end of edge
    vec2 posP = posN + offNP;
    vec2 posM = posN - offNP;
    
    float lumaEndP = FxaaLuma(textureLod(tex, posP, 0.0).rgb);
    float lumaEndN = FxaaLuma(textureLod(tex, posM, 0.0).rgb);
    
    float gradientScaled = max(gradientN, gradientS) * 0.25;
    bool lumaMLTZero = (lumaM - (lumaPosN + lumaNegN) * 0.5) < 0.0;
    
    // Extended edge search
    bool doneP = abs(lumaEndP - lumaM) >= gradientScaled;
    bool doneN = abs(lumaEndN - lumaM) >= gradientScaled;
    
    for (int i = 0; i < FXAA_SEARCH_STEPS && !(doneP && doneN); i++) {
        if (!doneP) {
            posP += offNP;
            lumaEndP = FxaaLuma(textureLod(tex, posP, 0.0).rgb);
            doneP = abs(lumaEndP - lumaM) >= gradientScaled;
        }
        if (!doneN) {
            posM -= offNP;
            lumaEndN = FxaaLuma(textureLod(tex, posM, 0.0).rgb);
            doneN = abs(lumaEndN - lumaM) >= gradientScaled;
        }
    }
    
    // Compute blend factor based on edge span
    float dstP = horzSpan ? (posP.x - uv.x) : (posP.y - uv.y);
    float dstN = horzSpan ? (uv.x - posM.x) : (uv.y - posM.y);
    
    float pixelOffset;
    bool goodSpan = (dstN < dstP) ? ((lumaEndN < 0.0) != lumaMLTZero) : ((lumaEndP < 0.0) != lumaMLTZero);
    float spanLen = dstP + dstN;
    
    if (goodSpan) {
        pixelOffset = (dstN < dstP) ? (0.5 - dstN / spanLen) : (dstP / spanLen - 0.5);
        pixelOffset *= lengthSign;
    } else {
        pixelOffset = 0.0;
    }
    
    // Final sample with offset
    vec2 finalUV = uv;
    if (horzSpan) {
        finalUV.y += pixelOffset;
    } else {
        finalUV.x += pixelOffset;
    }
    
    vec3 rgbF = textureLod(tex, finalUV, 0.0).rgb;
    
    // Blend with low-pass filtered result for sub-pixel quality
    vec3 rgbL = (rgbNW + rgbNE + rgbSW + rgbSE +
                 rgbN + rgbW + rgbE + rgbS + rgbM) / 9.0;
    
    return mix(rgbF, rgbL, blendL);
}

void main() {
    vec3 color = FxaaFilter(uSceneTexture, vTexCoord, uTexelSize);
    FragColor = vec4(color, 1.0);
}
