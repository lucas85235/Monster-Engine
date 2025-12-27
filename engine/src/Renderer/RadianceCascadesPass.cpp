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

// Scene textures
layout(binding = 1) uniform sampler2D uSceneColor;
layout(binding = 2) uniform sampler2D uSceneDepth;

uniform int uCascadeIndex;
uniform int uProbeCountX;
uniform int uProbeCountY;
uniform int uRayCount;
uniform float uIntervalLength;
uniform float uRayBias;
uniform vec2 uScreenSize;

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
    // We need to account for aspect ratio if we want consistent angular sweep in screen space
    // but standard RC 2D uses uniform angles and handles aspect in the step.
    return vec2(cos(angle), sin(angle));
}

// Get ray interval start and length for this cascade
void GetRayInterval(out float start, out float length) {
    float factor = pow(4.0, float(uCascadeIndex));
    start = uIntervalLength * (1.0 - factor) / -3.0;
    length = uIntervalLength * factor;
}

// Sample scene depth and check if ray hit something
float SampleDepth(vec2 uv) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return 1.0; // Out of bounds = far depth
    }
    return texture(uSceneDepth, uv).r;
}

vec3 SampleColor(vec2 uv) {
    return texture(uSceneColor, uv).rgb;
}

void main() {
    ivec2 texCoord = ivec2(gl_GlobalInvocationID.xy);
    
    int probeX = texCoord.x / uRayCount;
    int probeY = texCoord.y;
    int rayIndex = texCoord.x % uRayCount;
    
    if (probeX >= uProbeCountX || probeY >= uProbeCountY) {
        return;
    }
    
    vec2 probePos = GetProbePosition(probeX, probeY);
    vec2 rayDir = GetRayDirection(rayIndex);
    
    float intervalStart, intervalLength;
    GetRayInterval(intervalStart, intervalLength);
    
    // Raymarch along the ray
    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);
    
    const int STEPS = 64; // Increased for better precision
    float stepSize = intervalLength / float(STEPS);
    
    for (int i = 0; i < STEPS; i++) {
        float t = intervalStart + stepSize * (float(i) + 0.5);
        // Correct conversion from pixel distance to UV offset
        vec2 sampleUV = probePos + (rayDir * t) / uScreenSize;
        
        // Check bounds
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
            break;
        }
        
        // Sample emissive color directly
        vec3 hitColor = SampleColor(sampleUV);
        float luminance = dot(hitColor, vec3(0.299, 0.587, 0.114));
        
        if (luminance > 0.01) {
            result = vec4(hitColor, 1.0);
            break;
        }
        
        // Check depth occluders
        float depth = SampleDepth(sampleUV);
        if (depth > 0.01 && depth < 0.999) {
            result = vec4(0.0, 0.0, 0.0, 1.0);
            break;
        }
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

uniform int uProbeCountX;
uniform int uProbeCountY;
uniform int uRayCount;
uniform vec2 uScreenSize;
uniform int uDebugMode;  // 0 = normal, 1 = debug pattern, 2 = show emissive texture, 3 = pure red

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
    vec2 probeGrid = vec2(uProbeCountX, uProbeCountY);
    vec2 spacing = (uScreenSize / probeGrid);
    vec2 probePos = (vec2(texCoord) - spacing * 0.5) / spacing;
    
    ivec2 p00 = ivec2(floor(probePos));
    ivec2 p11 = min(p00 + ivec2(1), ivec2(uProbeCountX - 1, uProbeCountY - 1));
    
    vec2 frac = fract(probePos);
    
    // Sum all rays from nearest probes
    vec3 totalRadiance = vec3(0.0);
    
    for (int r = 0; r < uRayCount; r++) {
        // Bilinear sample from cascade0
        vec2 uv00 = vec2(p00.x * uRayCount + r, p00.y) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv11 = vec2(p11.x * uRayCount + r, p11.y) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv01 = vec2(p00.x * uRayCount + r, p11.y) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        vec2 uv10 = vec2(p11.x * uRayCount + r, p00.y) / vec2(uProbeCountX * uRayCount, uProbeCountY);
        
        vec3 v00 = texture(uCascade0, uv00).rgb;
        vec3 v11 = texture(uCascade0, uv11).rgb;
        vec3 v01 = texture(uCascade0, uv01).rgb;
        vec3 v10 = texture(uCascade0, uv10).rgb;
        
        vec3 t0 = mix(v00, v10, frac.x);
        vec3 t1 = mix(v01, v11, frac.x);
        totalRadiance += mix(t0, t1, frac.y);
    }
    
    // Average over all rays
    totalRadiance /= float(uRayCount);
    
    imageStore(uRadianceOut, texCoord, vec4(totalRadiance, 1.0));
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
    
    glBindTexture(GL_TEXTURE_2D, 0);
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
}

void RadianceCascadesPass::Execute(uint32_t sceneColorTex, uint32_t sceneDepthTex,
                                   const glm::mat4& projection, const glm::mat4& view) {
    if (!initialized_ || !config_.Enabled) return;
    
    invProjection_ = glm::inverse(projection);
    invView_ = glm::inverse(view);
    
    // Store emissive texture for debug mode
    lastEmissiveTex_ = sceneColorTex;
    
    // Step 1: Raymarch all cascades (from lowest to highest)
    for (int i = 0; i < config_.NumCascades; i++) {
        RaymarchCascade(i, sceneColorTex, sceneDepthTex);
    }
    
    // Step 2: Merge cascades (from highest to lowest)
    MergeCascades();
    
    // Step 3: Resolve final radiance from cascade 0
    ResolveRadiance();
}

void RadianceCascadesPass::RaymarchCascade(int cascadeIndex, uint32_t sceneColorTex, uint32_t sceneDepthTex) {
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
    
    raymarchShader_->SetInt("uCascadeIndex", cascadeIndex);
    raymarchShader_->SetInt("uProbeCountX", probeCount);
    raymarchShader_->SetInt("uProbeCountY", probeCount);
    raymarchShader_->SetInt("uRayCount", rayCount);
    raymarchShader_->SetFloat("uIntervalLength", config_.IntervalLength);
    raymarchShader_->SetFloat("uRayBias", config_.RayBias);
    raymarchShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    
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
    
    int probeCount = config_.BaseProbeCount;
    int rayCount = config_.BaseRayCount;
    
    resolveShader_->SetInt("uProbeCountX", probeCount);
    resolveShader_->SetInt("uProbeCountY", probeCount);
    resolveShader_->SetInt("uRayCount", rayCount);
    resolveShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetInt("uDebugMode", 2);  // 2 = show emissive texture directly
    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
}

}  // namespace se
