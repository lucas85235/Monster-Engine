#include "engine/renderer/RadianceCascadesPass.h"
#include "engine/renderer/ComputeShader.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <gtc/matrix_inverse.hpp>

namespace se {

namespace {
// Embedded compute shader sources for portability

constexpr const char* kRaymarchShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// Output cascade texture
layout(rgba16f, binding = 0) uniform image2D uCascadeOut;

// Scene textures (screen-space)
layout(binding = 1) uniform sampler2D uSceneColor;    // Emissive
layout(binding = 2) uniform sampler2D uSceneDepth;    // Depth
layout(binding = 3) uniform sampler2D uScenePosition; // World position

// Voxel textures (world-space)
layout(binding = 4) uniform sampler3D uVoxelAlbedo;   // Albedo + opacity
layout(binding = 5) uniform sampler3D uVoxelEmissive; // Emissive

uniform int uCascadeIndex;
uniform int uProbeCountX;
uniform int uProbeCountY;
uniform int uRayCount;
uniform float uIntervalLength;
uniform float uRayBias;
uniform vec2 uScreenSize;
uniform vec3 uCameraPos;

// Voxel grid uniforms
uniform vec3 uVoxelGridCenter;
uniform float uVoxelGridSize;
uniform int uVoxelResolution;
uniform bool uUseVoxels;

const float PI = 3.14159265359;

// Convert probe index to UV starting position
vec2 GetProbePosition(int probeX, int probeY) {
    vec2 spacing = uScreenSize / vec2(uProbeCountX, uProbeCountY);
    vec2 offset = spacing * 0.5;
    return (vec2(probeX, probeY) * spacing + offset) / uScreenSize;
}

// Calculate ray direction from ray index
vec2 GetRayDirection(int rayIndex) {
    float angle = (float(rayIndex) / float(uRayCount)) * 2.0 * PI;
    return vec2(cos(angle), sin(angle));
}

// Get ray interval start and length for this cascade
void GetRayInterval(out float start, out float length) {
    float factor = pow(4.0, float(uCascadeIndex));
    start = uIntervalLength * (1.0 - factor) / -3.0;
    length = uIntervalLength * factor;
}

vec3 SampleColor(vec2 uv) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec3(0.0);
    }
    return texture(uSceneColor, uv).rgb;
}

vec3 SampleWorldPos(vec2 uv) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec3(0.0);
    }
    return texture(uScenePosition, uv).xyz;
}

bool IsValidPosition(vec3 pos) {
    // Check if position is valid (not background/sky)
    return length(pos) > 0.01;
}

// Convert world position to voxel UV coordinates
vec3 WorldToVoxelUV(vec3 worldPos) {
    vec3 localPos = worldPos - uVoxelGridCenter;
    return (localPos / uVoxelGridSize) + 0.5;
}

// Sample emissive from voxel grid
vec4 SampleVoxelEmissive(vec3 worldPos) {
    if (!uUseVoxels) return vec4(0.0);
    
    vec3 voxelUV = WorldToVoxelUV(worldPos);
    if (any(lessThan(voxelUV, vec3(0.0))) || any(greaterThan(voxelUV, vec3(1.0)))) {
        return vec4(0.0);
    }
    return texture(uVoxelEmissive, voxelUV);
}

// Sample albedo (opacity) from voxel grid for occlusion
vec4 SampleVoxelAlbedo(vec3 worldPos) {
    if (!uUseVoxels) return vec4(0.0);
    
    vec3 voxelUV = WorldToVoxelUV(worldPos);
    if (any(lessThan(voxelUV, vec3(0.0))) || any(greaterThan(voxelUV, vec3(1.0)))) {
        return vec4(0.0);
    }
    return texture(uVoxelAlbedo, voxelUV);
}

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    
    int probeX = texCoord.x / uRayCount;
    int probeY = texCoord.y;
    int rayIndex = texCoord.x % uRayCount;
    
    if (probeX >= uProbeCountX || probeY >= uProbeCountY) {
        return;
    }
    
    vec2 probeUV = GetProbePosition(probeX, probeY);
    vec2 rayDir = GetRayDirection(rayIndex);
    
    float intervalStart, intervalLength;
    GetRayInterval(intervalStart, intervalLength);
    
    // Get the world position at the probe's screen location
    vec3 probeWorldPos = SampleWorldPos(probeUV);
    float probeDistFromCam = length(probeWorldPos - uCameraPos);
    bool probeHasGeometry = IsValidPosition(probeWorldPos);
    
    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);
    
    const int STEPS = 64;
    float stepSize = intervalLength / float(STEPS);
    
    // March outward from the probe in screen space
    for (int i = 0; i < STEPS; i++) {
        float t = intervalStart + stepSize * (float(i) + 0.5);
        if (t <= 0.0) continue;
        
        vec2 sampleUV = probeUV + (rayDir * t) / uScreenSize;
        
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            break;
        }
        
        // Get the world position at this sample point
        vec3 sampleWorldPos = SampleWorldPos(sampleUV);
        
        if (!IsValidPosition(sampleWorldPos)) {
            continue; // No geometry here, keep marching
        }
        
        // Calculate distance from camera of this sampled point
        float sampleDistFromCam = length(sampleWorldPos - uCameraPos);
        
        // Check if emissive first
        vec3 emissiveColor = SampleColor(sampleUV);
        float luminance = dot(emissiveColor, vec3(0.299, 0.587, 0.114));
        
        if (luminance > 0.001) {
            // Emissive surface found - collect light with distance falloff
            float worldDist = length(sampleWorldPos - probeWorldPos);
            float falloff = 1.0 / (1.0 + worldDist * 0.1);
            result = vec4(emissiveColor * falloff, 1.0);
            break;
        }
        
        // Non-emissive surface - check if it should occlude
        // Block if sample is CLOSER to camera than probe (something in front blocks light)
        // depthDiff > 0 means sample is BEHIND probe (further from camera) - don't block
        // depthDiff < 0 means sample is IN FRONT of probe (closer to camera) - block!
        if (probeHasGeometry) {
            float depthDiff = sampleDistFromCam - probeDistFromCam;
            if (depthDiff < -0.5) {
                // Sample is in front of probe (between camera and probe) - blocks light
                result = vec4(0.0, 0.0, 0.0, 1.0);
                break;
            }
            // Sample is at same depth or behind probe - keep marching
        }
        // If probe has no geometry (sky/background), keep marching
    }
    
    imageStore(uCascadeOut, texCoord, result);
}
)";

constexpr const char* kMergeShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform image2D uCascadeLower;
layout(binding = 1) uniform sampler2D uCascadeUpper;

uniform int uLowerProbeCountX;
uniform int uLowerProbeCountY;
uniform int uLowerRayCount;
uniform int uUpperProbeCountX;
uniform int uUpperProbeCountY;
uniform int uUpperRayCount;

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    
    int probeX = texCoord.x / uLowerRayCount;
    int probeY = texCoord.y;
    int rayIndex = texCoord.x % uLowerRayCount;
    
    if (probeX >= uLowerProbeCountX || probeY >= uLowerProbeCountY) {
        return;
    }
    
    vec4 lowerValue = imageLoad(uCascadeLower, texCoord);
    
    // If lower cascade already hit something, keep it
    if (lowerValue.a > 0.5) {
        return;
    }
    
    // Find corresponding probes in upper cascade (bilinear interpolation)
    float scaleX = float(uUpperProbeCountX) / float(uLowerProbeCountX);
    float scaleY = float(uUpperProbeCountY) / float(uLowerProbeCountY);
    
    vec2 upperProbePos = vec2(float(probeX) * scaleX, float(probeY) * scaleY);
    
    ivec2 p00 = ivec2(floor(upperProbePos));
    ivec2 p11 = min(p00 + ivec2(1), ivec2(uUpperProbeCountX - 1, uUpperProbeCountY - 1));
    ivec2 p01 = ivec2(p00.x, p11.y);
    ivec2 p10 = ivec2(p11.x, p00.y);
    
    vec2 frac = fract(upperProbePos);
    
    // Map ray index to upper cascade (4:1 ratio)
    int upperRayBase = rayIndex * 4;
    
    // Sample 4 matching rays from each of the 4 upper probes
    vec4 sum = vec4(0.0);
    float weight = 0.0;
    
    for (int r = 0; r < 4; r++) {
        int upperRayIndex = upperRayBase + r;
        if (upperRayIndex >= uUpperRayCount) break;
        
        // Sample from all 4 bilinear probes
        vec2 uv00 = vec2(p00.x * uUpperRayCount + upperRayIndex, p00.y) / vec2(uUpperProbeCountX * uUpperRayCount, uUpperProbeCountY);
        vec2 uv11 = vec2(p11.x * uUpperRayCount + upperRayIndex, p11.y) / vec2(uUpperProbeCountX * uUpperRayCount, uUpperProbeCountY);
        vec2 uv01 = vec2(p01.x * uUpperRayCount + upperRayIndex, p01.y) / vec2(uUpperProbeCountX * uUpperRayCount, uUpperProbeCountY);
        vec2 uv10 = vec2(p10.x * uUpperRayCount + upperRayIndex, p10.y) / vec2(uUpperProbeCountX * uUpperRayCount, uUpperProbeCountY);
        
        vec4 v00 = texture(uCascadeUpper, uv00);
        vec4 v11 = texture(uCascadeUpper, uv11);
        vec4 v01 = texture(uCascadeUpper, uv01);
        vec4 v10 = texture(uCascadeUpper, uv10);
        
        // Bilinear interpolation
        vec4 t0 = mix(v00, v10, frac.x);
        vec4 t1 = mix(v01, v11, frac.x);
        vec4 interpolated = mix(t0, t1, frac.y);
        
        sum += interpolated;
        weight += 1.0;
    }
    
    if (weight > 0.0) {
        vec4 merged = sum / weight;
        imageStore(uCascadeLower, texCoord, merged);
    }
}
)";

constexpr const char* kResolveShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform image2D uRadianceOut;
layout(binding = 1) uniform sampler2D uCascade0;
layout(binding = 2) uniform sampler2D uSceneEmissive;  // For debug mode 2
layout(binding = 3) uniform sampler2D uScenePosition;  // World position for temporal reprojection
layout(binding = 4) uniform sampler2D uHistoryRadiance; // Previous frame radiance

uniform int uProbeCountX;
uniform int uProbeCountY;
uniform int uRayCount;
uniform vec2 uScreenSize;
uniform int uDebugMode;  // 0 = normal, 1 = debug pattern, 2 = show emissive texture, 3 = pure red
uniform mat4 uPrevViewProj;
uniform bool uHasPreviousFrame;
uniform float uTemporalBlend;  // 0.0-1.0, how much to blend with history

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    
    if (texCoord.x >= int(uScreenSize.x) || texCoord.y >= int(uScreenSize.y)) {
        return;
    }
    
    vec2 uv = vec2(texCoord) / uScreenSize;
    
    // Debug mode 3: Pure red to verify compute shader is writing
    if (uDebugMode == 3) {
        imageStore(uRadianceOut, texCoord, vec4(1.0, 0.0, 0.0, 1.0));
        return;
    }
    
    // Debug mode 2: Show emissive texture directly with green base to verify sampling
    if (uDebugMode == 2) {
        vec3 emissive = texture(uSceneEmissive, uv).rgb;
        // Add green component to verify the shader is running (if pure green = texture is black)
        vec3 result = emissive + vec3(0.0, 0.1, 0.0);
        imageStore(uRadianceOut, texCoord, vec4(result, 1.0));
        return;
    }
    
    // Debug mode 1: Generate visible gradient pattern to verify compute shader works
    if (uDebugMode == 1) {
        // Create a radial gradient that pulses with position
        float cx = uv.x - 0.5;
        float cy = uv.y - 0.5;
        float dist = sqrt(cx*cx + cy*cy);
        
        // Color based on position and probes
        float probeInfluence = sin(uv.x * float(uProbeCountX) * 3.14159) * 
                               sin(uv.y * float(uProbeCountY) * 3.14159) * 0.5 + 0.5;
        
        vec3 debugColor = vec3(
            probeInfluence * (1.0 - dist),
            (1.0 - probeInfluence) * dist,
            dist * uv.x
        ) * 0.3;  // Keep it subtle
        
        imageStore(uRadianceOut, texCoord, vec4(debugColor, 1.0));
        return;
    }
    
    // Normal mode: Sample radiance at probe positions and interpolate
    // For each screen pixel, we find the 4 nearest probes and bilinearly interpolate
    // their accumulated radiance from all ray directions
    
    vec2 probeGrid = vec2(uProbeCountX, uProbeCountY);
    vec2 spacing = uScreenSize / probeGrid;
    
    // Find which probes affect this pixel
    vec2 probePos = (vec2(texCoord) + 0.5) / spacing - 0.5;
    
    ivec2 p00 = ivec2(floor(probePos));
    p00 = clamp(p00, ivec2(0), ivec2(uProbeCountX - 1, uProbeCountY - 1));
    ivec2 p11 = min(p00 + ivec2(1), ivec2(uProbeCountX - 1, uProbeCountY - 1));
    ivec2 p01 = ivec2(p00.x, p11.y);
    ivec2 p10 = ivec2(p11.x, p00.y);
    
    vec2 fractional = fract(probePos);
    
    // Accumulate radiance from all 4 nearest probes, all ray directions
    vec3 accum00 = vec3(0.0);
    vec3 accum01 = vec3(0.0);
    vec3 accum10 = vec3(0.0);
    vec3 accum11 = vec3(0.0);
    
    float invCascadeSize = 1.0 / vec2(uProbeCountX * uRayCount, uProbeCountY).x;
    
    for (int r = 0; r < uRayCount; r++) {
        // Sample from cascade0 for each probe at this ray direction
        vec2 uv00 = (vec2(p00.x * uRayCount + r, p00.y) + 0.5) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv11 = (vec2(p11.x * uRayCount + r, p11.y) + 0.5) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv01 = (vec2(p01.x * uRayCount + r, p01.y) + 0.5) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv10 = (vec2(p10.x * uRayCount + r, p10.y) + 0.5) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        
        accum00 += texture(uCascade0, uv00).rgb;
        accum11 += texture(uCascade0, uv11).rgb;
        accum01 += texture(uCascade0, uv01).rgb;
        accum10 += texture(uCascade0, uv10).rgb;
    }
    
    // Bilinear interpolate the accumulated radiance
    vec3 t0 = mix(accum00, accum10, fractional.x);
    vec3 t1 = mix(accum01, accum11, fractional.x);
    vec3 currentRadiance = mix(t0, t1, fractional.y);
    
    // Normalize by dividing by number of rays (hemisphere sampling)
    float normFactor = 6.28318 / float(uRayCount);
    currentRadiance *= normFactor;
    
    // Apply intensity boost for visibility
    currentRadiance *= 2.0;
    
    // Temporal blending for smooth results and off-screen persistence
    vec3 finalRadiance = currentRadiance;
    if (uHasPreviousFrame && uTemporalBlend > 0.0) {
        // Get world position for this pixel
        vec3 worldPos = texture(uScenePosition, uv).xyz;
        
        if (length(worldPos) > 0.01) {
            // Reproject to previous frame
            vec4 prevClip = uPrevViewProj * vec4(worldPos, 1.0);
            vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
            
            // Check if in bounds
            if (prevUV.x >= 0.0 && prevUV.x <= 1.0 && prevUV.y >= 0.0 && prevUV.y <= 1.0) {
                vec3 historyRadiance = texture(uHistoryRadiance, prevUV).rgb;
                // Blend: more weight on history for smoother results
                finalRadiance = mix(currentRadiance, historyRadiance, uTemporalBlend);
            }
        }
    }
    
    imageStore(uRadianceOut, texCoord, vec4(finalRadiance, 1.0));
}
)";

}  // namespace

RadianceCascadesPass::RadianceCascadesPass() = default;

RadianceCascadesPass::~RadianceCascadesPass() {
    Shutdown();
}

void RadianceCascadesPass::Init(int screenWidth, int screenHeight) {
    if (initialized_) {
        SE_LOG_WARN("RadianceCascadesPass already initialized");
        return;
    }
    
    SE_LOG_INFO("Initializing RadianceCascadesPass ({}x{})", screenWidth, screenHeight);
    
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    LoadShaders();
    CreateCascadeTextures();
    
    initialized_ = true;
    SE_LOG_INFO("RadianceCascadesPass initialized successfully");
}

void RadianceCascadesPass::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down RadianceCascadesPass");
    
    DestroyCascadeTextures();
    
    raymarchShader_.reset();
    mergeShader_.reset();
    resolveShader_.reset();
    
    initialized_ = false;
}

void RadianceCascadesPass::Resize(int screenWidth, int screenHeight) {
    if (screenWidth_ == screenWidth && screenHeight_ == screenHeight) {
        return;
    }
    
    SE_LOG_INFO("Resizing RadianceCascadesPass to {}x{}", screenWidth, screenHeight);
    
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    DestroyCascadeTextures();
    CreateCascadeTextures();
}

void RadianceCascadesPass::SetConfig(const RadianceCascadeConfig& config) {
    config_ = config;
    
    if (initialized_) {
        DestroyCascadeTextures();
        CreateCascadeTextures();
    }
}

void RadianceCascadesPass::LoadShaders() {
    SE_LOG_INFO("Loading Radiance Cascades shaders");
    
    raymarchShader_ = std::make_shared<ComputeShader>(kRaymarchShaderSource);
    if (!raymarchShader_->IsValid()) {
        SE_LOG_ERROR("Failed to create raymarch shader");
    }
    
    mergeShader_ = std::make_shared<ComputeShader>(kMergeShaderSource);
    if (!mergeShader_->IsValid()) {
        SE_LOG_ERROR("Failed to create merge shader");
    }
    
    resolveShader_ = std::make_shared<ComputeShader>(kResolveShaderSource);
    if (!resolveShader_->IsValid()) {
        SE_LOG_ERROR("Failed to create resolve shader");
    }
}

void RadianceCascadesPass::CreateCascadeTextures() {
    SE_LOG_INFO("Creating cascade textures ({} cascades)", config_.NumCascades);
    
    cascadeTextures_.resize(config_.NumCascades);
    
    for (int i = 0; i < config_.NumCascades; i++) {
        // Each cascade: probes decrease by 2x, rays increase by 4x
        int probeCount = config_.BaseProbeCount >> i;  // /2 per cascade
        int rayCount = config_.BaseRayCount << (i * 2); // *4 per cascade
        
        int texWidth = probeCount * rayCount;
        int texHeight = probeCount;
        
        SE_LOG_INFO("  Cascade {}: {}x{} probes, {} rays -> {}x{} texture", 
                    i, probeCount, probeCount, rayCount, texWidth, texHeight);
        
        glGenTextures(1, &cascadeTextures_[i]);
        glBindTexture(GL_TEXTURE_2D, cascadeTextures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, texWidth, texHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // Create final radiance texture (screen resolution)
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create history texture for temporal reprojection (same as final)
    glGenTextures(1, &historyRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, historyRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    hasPreviousFrame_ = false;
}

void RadianceCascadesPass::DestroyCascadeTextures() {
    for (uint32_t tex : cascadeTextures_) {
        if (tex != 0) {
            glDeleteTextures(1, &tex);
        }
    }
    cascadeTextures_.clear();
    
    if (finalRadianceTex_ != 0) {
        glDeleteTextures(1, &finalRadianceTex_);
        finalRadianceTex_ = 0;
    }
    
    if (historyRadianceTex_ != 0) {
        glDeleteTextures(1, &historyRadianceTex_);
        historyRadianceTex_ = 0;
    }
    
    hasPreviousFrame_ = false;
}

void RadianceCascadesPass::Execute(uint32_t sceneColorTex, uint32_t sceneDepthTex, uint32_t scenePositionTex,
                                   const glm::mat4& projection, const glm::mat4& view,
                                   const glm::vec3& cameraPos,
                                   uint32_t voxelAlbedoTex, uint32_t voxelEmissiveTex,
                                   const glm::vec3& voxelGridCenter, float voxelGridSize, int voxelResolution) {
    if (!initialized_ || !config_.Enabled) return;
    
    invProjection_ = glm::inverse(projection);
    invView_ = glm::inverse(view);
    
    // Store textures for resolve pass
    lastEmissiveTex_ = sceneColorTex;
    lastPositionTex_ = scenePositionTex;
    
    // Step 1: Raymarch all cascades (from lowest to highest)
    for (int i = 0; i < config_.NumCascades; i++) {
        RaymarchCascade(i, sceneColorTex, sceneDepthTex, scenePositionTex, cameraPos,
                       voxelAlbedoTex, voxelEmissiveTex, voxelGridCenter, voxelGridSize, voxelResolution);
    }
    
    // Step 2: Merge cascades (from highest to lowest)
    MergeCascades();
    
    // Step 3: Resolve final radiance from cascade 0
    ResolveRadiance();
    
    // Store current view-projection for next frame's temporal reprojection
    prevViewProj_ = projection * view;
}

void RadianceCascadesPass::RaymarchCascade(int cascadeIndex, uint32_t sceneColorTex, uint32_t sceneDepthTex,
                                           uint32_t scenePositionTex, const glm::vec3& cameraPos,
                                           uint32_t voxelAlbedoTex, uint32_t voxelEmissiveTex,
                                           const glm::vec3& voxelGridCenter, float voxelGridSize, int voxelResolution) {
    if (!raymarchShader_ || !raymarchShader_->IsValid()) return;
    
    int probeCount = config_.BaseProbeCount >> cascadeIndex;
    int rayCount = config_.BaseRayCount << (cascadeIndex * 2);
    
    int texWidth = probeCount * rayCount;
    int texHeight = probeCount;
    
    raymarchShader_->Bind();
    
    // Bind output cascade
    glBindImageTexture(0, cascadeTextures_[cascadeIndex], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Bind scene textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, sceneColorTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, scenePositionTex);
    
    // Bind voxel textures
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_3D, voxelAlbedoTex);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_3D, voxelEmissiveTex);
    
    raymarchShader_->SetInt("uCascadeIndex", cascadeIndex);
    raymarchShader_->SetInt("uProbeCountX", probeCount);
    raymarchShader_->SetInt("uProbeCountY", probeCount);
    raymarchShader_->SetInt("uRayCount", rayCount);
    raymarchShader_->SetFloat("uIntervalLength", config_.IntervalLength);
    raymarchShader_->SetFloat("uRayBias", config_.RayBias);
    raymarchShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    raymarchShader_->SetVec3("uCameraPos", cameraPos);
    
    // Voxel uniforms
    raymarchShader_->SetVec3("uVoxelGridCenter", voxelGridCenter);
    raymarchShader_->SetFloat("uVoxelGridSize", voxelGridSize);
    raymarchShader_->SetInt("uVoxelResolution", voxelResolution);
    raymarchShader_->SetInt("uUseVoxels", voxelAlbedoTex != 0 && voxelEmissiveTex != 0 ? 1 : 0);
    
    uint32_t groupsX = (texWidth + 7) / 8;
    uint32_t groupsY = (texHeight + 7) / 8;
    
    raymarchShader_->DispatchAndWait(groupsX, groupsY, 1);
}

void RadianceCascadesPass::MergeCascades() {
    if (!mergeShader_ || !mergeShader_->IsValid()) return;
    
    mergeShader_->Bind();
    
    // Merge from highest cascade down to cascade 0
    for (int i = config_.NumCascades - 2; i >= 0; i--) {
        int lowerProbeCount = config_.BaseProbeCount >> i;
        int lowerRayCount = config_.BaseRayCount << (i * 2);
        int upperProbeCount = config_.BaseProbeCount >> (i + 1);
        int upperRayCount = config_.BaseRayCount << ((i + 1) * 2);
        
        // Lower cascade (read/write)
        glBindImageTexture(0, cascadeTextures_[i], 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        
        // Upper cascade (read only via sampler)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, cascadeTextures_[i + 1]);
        
        mergeShader_->SetInt("uLowerProbeCountX", lowerProbeCount);
        mergeShader_->SetInt("uLowerProbeCountY", lowerProbeCount);
        mergeShader_->SetInt("uLowerRayCount", lowerRayCount);
        mergeShader_->SetInt("uUpperProbeCountX", upperProbeCount);
        mergeShader_->SetInt("uUpperProbeCountY", upperProbeCount);
        mergeShader_->SetInt("uUpperRayCount", upperRayCount);
        
        int texWidth = lowerProbeCount * lowerRayCount;
        int texHeight = lowerProbeCount;
        
        uint32_t groupsX = (texWidth + 7) / 8;
        uint32_t groupsY = (texHeight + 7) / 8;
        
        mergeShader_->DispatchAndWait(groupsX, groupsY, 1);
    }
}

void RadianceCascadesPass::ResolveRadiance() {
    if (!resolveShader_ || !resolveShader_->IsValid()) return;
    
    resolveShader_->Bind();
    
    // Output final radiance
    glBindImageTexture(0, finalRadianceTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Input cascade 0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, cascadeTextures_[0]);
    
    // Bind emissive texture for debug mode 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lastEmissiveTex_);
    
    // Bind position texture for temporal reprojection
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, lastPositionTex_);
    
    // Bind history radiance for temporal blending
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, historyRadianceTex_);
    
    int probeCount = config_.BaseProbeCount;
    int rayCount = config_.BaseRayCount;
    
    resolveShader_->SetInt("uProbeCountX", probeCount);
    resolveShader_->SetInt("uProbeCountY", probeCount);
    resolveShader_->SetInt("uRayCount", rayCount);
    resolveShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetInt("uDebugMode", 0);  // 0 = normal GI rendering
    
    // Temporal uniforms
    resolveShader_->SetMat4("uPrevViewProj", prevViewProj_);
    resolveShader_->SetInt("uHasPreviousFrame", hasPreviousFrame_ ? 1 : 0);
    resolveShader_->SetFloat("uTemporalBlend", 0.9f);  // 90% history, 10% current
    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
    
    // Copy current result to history for next frame
    glCopyImageSubData(
        finalRadianceTex_, GL_TEXTURE_2D, 0, 0, 0, 0,
        historyRadianceTex_, GL_TEXTURE_2D, 0, 0, 0, 0,
        screenWidth_, screenHeight_, 1
    );
    
    // Store current view-projection for next frame
    hasPreviousFrame_ = true;
}

}  // namespace se
