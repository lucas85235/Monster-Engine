#include "engine/renderer/SparseRadianceCascades.h"
#include "engine/renderer/ComputeShader.h"
#include "engine/renderer/SceneVoxelizer.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <gtc/matrix_inverse.hpp>

namespace se {

namespace {

// 3D Raymarch shader - marches through voxel grid to collect radiance
constexpr const char* kRaymarch3DShaderSource = R"(#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

// Output cascade texture (3D: probeX, probeY, probeZ * numDirs + dirIndex)
layout(rgba16f, binding = 0) uniform image3D uCascadeOut;

// Voxel grid textures
layout(binding = 1) uniform sampler3D uVoxelAlbedo;
layout(binding = 2) uniform sampler3D uVoxelEmissive;

uniform int uCascadeIndex;
uniform int uProbeCount;
uniform int uNumDirections;
uniform float uIntervalStart;
uniform float uIntervalEnd;
uniform vec3 uGridCenter;
uniform float uGridWorldSize;
uniform int uGridResolution;
uniform vec3 uCameraPos;

const float PI = 3.14159265359;

// 6 directions for cube faces
const vec3 DIRECTIONS[6] = vec3[6](
    vec3( 1.0,  0.0,  0.0),  // +X
    vec3(-1.0,  0.0,  0.0),  // -X
    vec3( 0.0,  1.0,  0.0),  // +Y
    vec3( 0.0, -1.0,  0.0),  // -Y
    vec3( 0.0,  0.0,  1.0),  // +Z
    vec3( 0.0,  0.0, -1.0)   // -Z
);

vec3 GetProbeWorldPos(ivec3 probeCoord) {
    vec3 normalizedPos = (vec3(probeCoord) + 0.5) / float(uProbeCount);
    vec3 localPos = (normalizedPos - 0.5) * uGridWorldSize;
    return localPos + uGridCenter;
}

vec3 WorldToVoxelUV(vec3 worldPos) {
    vec3 localPos = worldPos - uGridCenter;
    return (localPos / uGridWorldSize) + 0.5;
}

bool IsInsideGrid(vec3 voxelUV) {
    return all(greaterThanEqual(voxelUV, vec3(0.0))) && 
           all(lessThanEqual(voxelUV, vec3(1.0)));
}

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    
    int probeX = coord.x;
    int probeY = coord.y;
    int linearZ = coord.z;
    int probeZ = linearZ / uNumDirections;
    int dirIndex = linearZ % uNumDirections;
    
    if (probeX >= uProbeCount || probeY >= uProbeCount || probeZ >= uProbeCount) {
        return;
    }
    if (dirIndex >= uNumDirections) {
        return;
    }
    
    vec3 probeWorldPos = GetProbeWorldPos(ivec3(probeX, probeY, probeZ));
    vec3 rayDir = DIRECTIONS[dirIndex];
    
    vec4 result = vec4(0.0, 0.0, 0.0, 0.0);  // RGB = radiance, A = hit flag
    
    const int MAX_STEPS = 64;
    float intervalLength = uIntervalEnd - uIntervalStart;
    float stepSize = intervalLength / float(MAX_STEPS);
    
    // March within this cascade's interval
    for (int i = 0; i < MAX_STEPS; i++) {
        float t = uIntervalStart + stepSize * (float(i) + 0.5);
        
        vec3 sampleWorldPos = probeWorldPos + rayDir * t;
        vec3 voxelUV = WorldToVoxelUV(sampleWorldPos);
        
        if (!IsInsideGrid(voxelUV)) {
            break;
        }
        
        // Sample voxel grid
        vec4 albedo = texture(uVoxelAlbedo, voxelUV);
        vec4 emissive = texture(uVoxelEmissive, voxelUV);
        
        // Check for hit (occupied voxel)
        if (albedo.a > 0.1) {
            // Check if emissive
            float emissiveLuminance = dot(emissive.rgb, vec3(0.299, 0.587, 0.114));
            if (emissiveLuminance > 0.001) {
                // Collect emissive light with distance falloff
                float falloff = 1.0 / (1.0 + t * 0.1);
                result = vec4(emissive.rgb * falloff, 1.0);
            } else {
                // Hit non-emissive surface - blocked
                result = vec4(0.0, 0.0, 0.0, 1.0);
            }
            break;
        }
    }
    
    // Store result
    imageStore(uCascadeOut, coord, result);
}
)";

// Merge shader - merges upper cascade into lower cascade
constexpr const char* kMerge3DShaderSource = R"(#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform image3D uCascadeLower;
layout(binding = 1) uniform sampler3D uCascadeUpper;

uniform int uLowerProbeCount;
uniform int uUpperProbeCount;
uniform int uNumDirections;
uniform float uUpperProbeScale;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    
    int probeX = coord.x;
    int probeY = coord.y;
    int linearZ = coord.z;
    int probeZ = linearZ / uNumDirections;
    int dirIndex = linearZ % uNumDirections;
    
    if (probeX >= uLowerProbeCount || probeY >= uLowerProbeCount || probeZ >= uLowerProbeCount) {
        return;
    }
    
    vec4 lowerValue = imageLoad(uCascadeLower, coord);
    
    // If lower cascade already hit something, keep it (alpha > 0.5 means hit)
    if (lowerValue.a > 0.5) {
        return;
    }
    
    // Sample upper cascade with trilinear interpolation
    // Upper cascade has fewer probes, so we need to map coordinates
    vec3 upperCoord = vec3(probeX, probeY, probeZ) * uUpperProbeScale;
    
    // Ensure dirIndex is preserved  
    vec3 uvw = vec3(
        (upperCoord.x + 0.5) / float(uUpperProbeCount),
        (upperCoord.y + 0.5) / float(uUpperProbeCount),
        (float(int(upperCoord.z) * uNumDirections + dirIndex) + 0.5) / float(uUpperProbeCount * uNumDirections)
    );
    
    vec4 upperValue = texture(uCascadeUpper, uvw);
    
    // Blend: use upper cascade's result
    imageStore(uCascadeLower, coord, upperValue);
}
)";

// Resolve shader - project cascade 0 to screen space
constexpr const char* kResolve3DShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform image2D uOutputRadiance;
layout(binding = 1) uniform sampler3D uCascade0;
layout(binding = 2) uniform sampler2D uGBufferPosition;
layout(binding = 3) uniform sampler2D uGBufferNormal;

uniform int uProbeCount;
uniform int uNumDirections;
uniform vec3 uGridCenter;
uniform float uGridWorldSize;
uniform vec2 uScreenSize;
uniform float uIntensity;

const vec3 DIRECTIONS[6] = vec3[6](
    vec3( 1.0,  0.0,  0.0),
    vec3(-1.0,  0.0,  0.0),
    vec3( 0.0,  1.0,  0.0),
    vec3( 0.0, -1.0,  0.0),
    vec3( 0.0,  0.0,  1.0),
    vec3( 0.0,  0.0, -1.0)
);

vec3 WorldToProbeUV(vec3 worldPos) {
    vec3 localPos = worldPos - uGridCenter;
    return (localPos / uGridWorldSize) + 0.5;
}

void main() {
    ivec2 screenCoord = ivec2(gl_GlobalInvocationID.xy);
    if (screenCoord.x >= int(uScreenSize.x) || screenCoord.y >= int(uScreenSize.y)) {
        return;
    }
    
    vec2 uv = (vec2(screenCoord) + 0.5) / uScreenSize;
    
    vec3 worldPos = texture(uGBufferPosition, uv).xyz;
    vec3 normal = normalize(texture(uGBufferNormal, uv).xyz);
    
    // Skip background pixels
    if (length(worldPos) < 0.01) {
        imageStore(uOutputRadiance, screenCoord, vec4(0.0));
        return;
    }
    
    vec3 probeUV = WorldToProbeUV(worldPos);
    
    // Sample all directions and weight by normal
    vec3 totalRadiance = vec3(0.0);
    float totalWeight = 0.0;
    
    for (int dir = 0; dir < uNumDirections; dir++) {
        // Weight by how much this direction aligns with inverted normal (incoming light)
        float weight = max(0.0, dot(-DIRECTIONS[dir], normal));
        if (weight < 0.01) continue;
        
        // Sample cascade 0 at this position and direction
        vec3 sampleUV = vec3(
            probeUV.x,
            probeUV.y,
            (probeUV.z * float(uProbeCount) * float(uNumDirections) + float(dir)) / float(uProbeCount * uNumDirections)
        );
        
        vec4 radiance = texture(uCascade0, sampleUV);
        totalRadiance += radiance.rgb * weight;
        totalWeight += weight;
    }
    
    if (totalWeight > 0.0) {
        totalRadiance /= totalWeight;
    }
    
    imageStore(uOutputRadiance, screenCoord, vec4(totalRadiance * uIntensity, 1.0));
}
)";

}  // namespace

SparseRadianceCascades::SparseRadianceCascades() = default;

SparseRadianceCascades::~SparseRadianceCascades() {
    Shutdown();
}

void SparseRadianceCascades::Init(int screenWidth, int screenHeight, 
                                   std::shared_ptr<SceneVoxelizer> voxelizer) {
    if (initialized_) {
        SE_LOG_WARN("SparseRadianceCascades already initialized");
        return;
    }
    
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    voxelizer_ = voxelizer;
    
    SE_LOG_INFO("Initializing SparseRadianceCascades: {} cascades, {} base probes", 
                config_.NumCascades, config_.BaseProbeCount);
    
    LoadShaders();
    CreateCascadeTextures();
    
    initialized_ = true;
}

void SparseRadianceCascades::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down SparseRadianceCascades");
    
    DestroyCascadeTextures();
    raymarchShader_.reset();
    mergeShader_.reset();
    resolveShader_.reset();
    voxelizer_.reset();
    
    initialized_ = false;
}

void SparseRadianceCascades::Resize(int screenWidth, int screenHeight) {
    if (screenWidth_ == screenWidth && screenHeight_ == screenHeight) return;
    
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    // Recreate screen-space output texture
    if (finalRadianceTex_ != 0) {
        glDeleteTextures(1, &finalRadianceTex_);
    }
    
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SparseRadianceCascades::SetConfig(const SparseRCConfig& config) {
    bool needRecreate = config.NumCascades != config_.NumCascades ||
                        config.BaseProbeCount != config_.BaseProbeCount ||
                        config.DirectionsPerProbe != config_.DirectionsPerProbe;
    config_ = config;
    
    if (initialized_ && needRecreate) {
        DestroyCascadeTextures();
        CreateCascadeTextures();
    }
}

void SparseRadianceCascades::LoadShaders() {
    raymarchShader_ = std::make_shared<ComputeShader>();
    if (!raymarchShader_->LoadFromSource(kRaymarch3DShaderSource)) {
        SE_LOG_ERROR("Failed to compile 3D raymarch shader");
        raymarchShader_.reset();
    }
    
    mergeShader_ = std::make_shared<ComputeShader>();
    if (!mergeShader_->LoadFromSource(kMerge3DShaderSource)) {
        SE_LOG_ERROR("Failed to compile 3D merge shader");
        mergeShader_.reset();
    }
    
    resolveShader_ = std::make_shared<ComputeShader>();
    if (!resolveShader_->LoadFromSource(kResolve3DShaderSource)) {
        SE_LOG_ERROR("Failed to compile 3D resolve shader");
        resolveShader_.reset();
    }
}

void SparseRadianceCascades::CreateCascadeTextures() {
    cascadeTextures_.resize(config_.NumCascades);
    
    for (int i = 0; i < config_.NumCascades; i++) {
        int probeCount = GetProbeCount(i);
        int zSize = probeCount * config_.DirectionsPerProbe;
        
        glGenTextures(1, &cascadeTextures_[i]);
        glBindTexture(GL_TEXTURE_3D, cascadeTextures_[i]);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, probeCount, probeCount, zSize, 
                     0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        SE_LOG_INFO("Created cascade {} texture: {}³ probes × {} dirs = {}×{}×{}", 
                    i, probeCount, config_.DirectionsPerProbe, 
                    probeCount, probeCount, zSize);
    }
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    // Create final screen output texture
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SparseRadianceCascades::DestroyCascadeTextures() {
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

int SparseRadianceCascades::GetProbeCount(int cascadeIndex) const {
    // Cascade 0 has BaseProbeCount, each higher cascade has half
    return config_.BaseProbeCount >> cascadeIndex;
}

void SparseRadianceCascades::GetCascadeInterval(int cascadeIndex, float& start, float& end) const {
    // Geometric sequence: cascade N covers [scale^(N-1) * base, scale^N * base]
    // Cascade 0 starts from 0
    if (cascadeIndex == 0) {
        start = 0.0f;
        end = config_.BaseInterval;
    } else {
        float scale = config_.CascadeScale;
        start = config_.BaseInterval * std::pow(scale, static_cast<float>(cascadeIndex - 1));
        end = config_.BaseInterval * std::pow(scale, static_cast<float>(cascadeIndex));
    }
}

void SparseRadianceCascades::Execute(const glm::mat4& projection, const glm::mat4& view, 
                                      const glm::vec3& cameraPos) {
    if (!initialized_ || !config_.Enabled) return;
    if (!voxelizer_ || !voxelizer_->IsInitialized()) return;
    
    invProjection_ = glm::inverse(projection);
    invView_ = glm::inverse(view);
    
    // Step 1: Raymarch all cascades (can be done in parallel but we do sequentially)
    for (int i = 0; i < config_.NumCascades; i++) {
        RaymarchCascade(i, cameraPos);
    }
    
    // Step 2: Merge cascades from highest to lowest (outside-in)
    MergeCascades();
    
    // Step 3: Resolve cascade 0 to screen space
    ResolveToScreen(invView_);
}

void SparseRadianceCascades::RaymarchCascade(int cascadeIndex, const glm::vec3& cameraPos) {
    if (!raymarchShader_ || !raymarchShader_->IsValid()) return;
    
    int probeCount = GetProbeCount(cascadeIndex);
    float intervalStart, intervalEnd;
    GetCascadeInterval(cascadeIndex, intervalStart, intervalEnd);
    
    const auto& voxelConfig = voxelizer_->GetConfig();
    
    raymarchShader_->Bind();
    
    // Bind output cascade
    glBindImageTexture(0, cascadeTextures_[cascadeIndex], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Bind voxel textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelEmissiveTexture());
    
    // Set uniforms
    raymarchShader_->SetInt("uCascadeIndex", cascadeIndex);
    raymarchShader_->SetInt("uProbeCount", probeCount);
    raymarchShader_->SetInt("uNumDirections", config_.DirectionsPerProbe);
    raymarchShader_->SetFloat("uIntervalStart", intervalStart);
    raymarchShader_->SetFloat("uIntervalEnd", intervalEnd);
    raymarchShader_->SetVec3("uGridCenter", voxelConfig.Center);
    raymarchShader_->SetFloat("uGridWorldSize", voxelConfig.WorldSize);
    raymarchShader_->SetInt("uGridResolution", voxelConfig.Resolution);
    raymarchShader_->SetVec3("uCameraPos", cameraPos);
    
    // Dispatch: probeCount × probeCount × (probeCount * numDirections)
    int zSize = probeCount * config_.DirectionsPerProbe;
    uint32_t groupsX = (probeCount + 3) / 4;
    uint32_t groupsY = (probeCount + 3) / 4;
    uint32_t groupsZ = (zSize + 3) / 4;
    
    raymarchShader_->DispatchAndWait(groupsX, groupsY, groupsZ);
}

void SparseRadianceCascades::MergeCascades() {
    if (!mergeShader_ || !mergeShader_->IsValid()) return;
    
    mergeShader_->Bind();
    
    // Merge from highest cascade down to cascade 0
    for (int i = config_.NumCascades - 2; i >= 0; i--) {
        int lowerProbeCount = GetProbeCount(i);
        int upperProbeCount = GetProbeCount(i + 1);
        float upperProbeScale = float(upperProbeCount) / float(lowerProbeCount);
        
        // Lower cascade (read/write)
        glBindImageTexture(0, cascadeTextures_[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        
        // Upper cascade (read via sampler)
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, cascadeTextures_[i + 1]);
        
        mergeShader_->SetInt("uLowerProbeCount", lowerProbeCount);
        mergeShader_->SetInt("uUpperProbeCount", upperProbeCount);
        mergeShader_->SetInt("uNumDirections", config_.DirectionsPerProbe);
        mergeShader_->SetFloat("uUpperProbeScale", upperProbeScale);
        
        int zSize = lowerProbeCount * config_.DirectionsPerProbe;
        uint32_t groupsX = (lowerProbeCount + 3) / 4;
        uint32_t groupsY = (lowerProbeCount + 3) / 4;
        uint32_t groupsZ = (zSize + 3) / 4;
        
        mergeShader_->DispatchAndWait(groupsX, groupsY, groupsZ);
    }
}

void SparseRadianceCascades::ResolveToScreen(const glm::mat4& invView) {
    if (!resolveShader_ || !resolveShader_->IsValid()) return;
    
    const auto& voxelConfig = voxelizer_->GetConfig();
    int probeCount = GetProbeCount(0);
    
    resolveShader_->Bind();
    
    // Output texture
    glBindImageTexture(0, finalRadianceTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Cascade 0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, cascadeTextures_[0]);
    
    // G-Buffer textures (position and normal) - these should be bound externally
    // For now, we'll skip and rely on SceneRenderer to pass them
    
    resolveShader_->SetInt("uProbeCount", probeCount);
    resolveShader_->SetInt("uNumDirections", config_.DirectionsPerProbe);
    resolveShader_->SetVec3("uGridCenter", voxelConfig.Center);
    resolveShader_->SetFloat("uGridWorldSize", voxelConfig.WorldSize);
    resolveShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetFloat("uIntensity", 1.0f);
    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
}

}  // namespace se
