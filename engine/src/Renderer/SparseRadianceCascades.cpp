#include "engine/renderer/SparseRadianceCascades.h"
#include "engine/renderer/ComputeShader.h"
#include "engine/renderer/SceneVoxelizer.h"
#include "engine/renderer/GBufferPass.h"
#include "engine/renderer/gi/VoxelRayQueryBackend.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <gtc/matrix_inverse.hpp>
#include <cmath>

namespace se {

namespace {

constexpr const char* kPopulateShaderSource = R"(#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform image3D uCascadeOut;

layout(binding = 1) uniform sampler3D uVoxelAlbedo;
layout(binding = 2) uniform sampler3D uVoxelEmissive;

uniform int uCascadeIndex;
uniform int uProbeCount;
uniform int uBrickSize;
uniform float uIntervalStart;
uniform float uIntervalEnd;
uniform vec3 uGridCenter;
uniform float uGridWorldSize;
uniform int uGridResolution;
uniform vec3 uCameraPos;
uniform float uCascadeVoxelSize;
uniform int uNumSamples;

const float PI = 3.14159265359;

// SH L2 basis functions
float SHBasis0() { return 0.282095; }
float SHBasis1(float x, float y, float z, int i) {
    const float k = 0.488603;
    if (i == 0) return k * y;
    if (i == 1) return k * z;
    return k * x;
}

// Fibonacci sphere sampling for uniform distribution
vec3 FibonacciSphere(int i, int n) {
    float phi = PI * (3.0 - sqrt(5.0));
    float y = 1.0 - (float(i) / float(n - 1)) * 2.0;
    float radius = sqrt(1.0 - y * y);
    float theta = phi * float(i);
    return vec3(cos(theta) * radius, y, sin(theta) * radius);
}

vec3 GetProbeWorldPos(ivec3 probeCoord, int probeCount) {
    // Probes should cover the same area as the voxel grid
    vec3 normalizedPos = (vec3(probeCoord) + 0.5) / float(probeCount);
    return (normalizedPos - 0.5) * uGridWorldSize + uGridCenter;
}

vec3 WorldToVoxelUV(vec3 worldPos) {
    vec3 localPos = worldPos - uGridCenter;
    return (localPos / uGridWorldSize) + 0.5;

}

bool IsInsideGrid(vec3 voxelUV) {
    return all(greaterThanEqual(voxelUV, vec3(0.0))) && 
           all(lessThanEqual(voxelUV, vec3(1.0)));
}

struct RayHit {
    bool hit;
    vec3 position;
    vec3 albedo;
    vec3 emissive;
    float distance;
};

RayHit TraceRay(vec3 origin, vec3 dir, float tMin, float tMax) {
    RayHit result;
    result.hit = false;
    result.distance = tMax;
    
    const int MAX_STEPS = 64;
    float stepSize = (tMax - tMin) / float(MAX_STEPS);
    
    for (int i = 0; i < MAX_STEPS; i++) {
        float t = tMin + stepSize * (float(i) + 0.5);
        vec3 samplePos = origin + dir * t;
        vec3 voxelUV = WorldToVoxelUV(samplePos);
        
        if (!IsInsideGrid(voxelUV)) break;
        
        vec4 albedo = texture(uVoxelAlbedo, voxelUV);
        
        if (albedo.a > 0.1) {
            result.hit = true;
            result.position = samplePos;
            result.distance = t;
            result.albedo = albedo.rgb;
            result.emissive = texture(uVoxelEmissive, voxelUV).rgb;
            return result;
        }
    }
    
    return result;
}

void main() {
    ivec3 probeCoord = ivec3(gl_GlobalInvocationID.xyz);
    
    if (probeCoord.x >= uProbeCount || probeCoord.y >= uProbeCount || probeCoord.z >= uProbeCount) {
        return;
    }
    
    vec3 probeWorldPos = GetProbeWorldPos(probeCoord, uProbeCount);
    
    // Accumulate SH coefficients
    vec3 sh[9];
    for (int i = 0; i < 9; i++) sh[i] = vec3(0.0);
    
    float intervalLength = uIntervalEnd - uIntervalStart;
    
    // Sample directions uniformly on sphere
    for (int s = 0; s < uNumSamples; s++) {
        vec3 dir = FibonacciSphere(s, uNumSamples);
        
        RayHit hit = TraceRay(probeWorldPos, dir, uIntervalStart, uIntervalEnd);
        
        vec3 radiance = vec3(0.0);
        
        if (hit.hit) {
            // Emissive contribution - boost emissive to make it more visible
            float emissiveLum = dot(hit.emissive, vec3(0.299, 0.587, 0.114));
            if (emissiveLum > 0.001) {
                float falloff = 1.0 / (1.0 + hit.distance * 0.05);
                radiance = hit.emissive * falloff * 3.0;  // Boost emissive
            }
            // Albedo bounce contribution (single bounce approximation)
            radiance += hit.albedo * 0.02;
            
            // DEBUG: Show hit.emissive directly for any hit (even if lum < threshold)
            // radiance = hit.emissive + vec3(0.01); // Uncomment to debug
        } else {
            // Small sky contribution for ambient occlusion effect
            radiance = vec3(0.005, 0.008, 0.012);  // Very subtle ambient
        }

        
        // Project to SH L2
        float x = dir.x, y = dir.y, z = dir.z;
        
        // L0
        sh[0] += radiance * 0.282095;
        
        // L1
        sh[1] += radiance * 0.488603 * y;
        sh[2] += radiance * 0.488603 * z;
        sh[3] += radiance * 0.488603 * x;
        
        // L2
        sh[4] += radiance * 1.092548 * x * y;
        sh[5] += radiance * 0.546274 * y * z;
        sh[6] += radiance * 0.315392 * (3.0 * z * z - 1.0);
        sh[7] += radiance * 0.546274 * x * z;
        sh[8] += radiance * 1.092548 * (x * x - y * y);
    }
    
    // Normalize by sample count
    float invSamples = 4.0 * PI / float(uNumSamples);
    for (int i = 0; i < 9; i++) {
        sh[i] *= invSamples;
    }
    
    // Store SH in 3x3 grid per probe (9 coefficients as 3 vec3s stored in 3x3 texels)
    ivec3 baseCoord = probeCoord * 3;
    
    imageStore(uCascadeOut, baseCoord + ivec3(0, 0, 0), vec4(sh[0], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(1, 0, 0), vec4(sh[1], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(2, 0, 0), vec4(sh[2], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(0, 1, 0), vec4(sh[3], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(1, 1, 0), vec4(sh[4], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(2, 1, 0), vec4(sh[5], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(0, 2, 0), vec4(sh[6], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(1, 2, 0), vec4(sh[7], 0.0));
    imageStore(uCascadeOut, baseCoord + ivec3(2, 2, 0), vec4(sh[8], 0.0));
}
)";

constexpr const char* kMergeShaderSource = R"(#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(rgba16f, binding = 0) uniform image3D uCascadeLower;
layout(binding = 1) uniform sampler3D uCascadeUpper;

uniform int uLowerProbeCount;
uniform int uUpperProbeCount;
uniform float uBlendFactor;

void main() {
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);
    
    int probeCount = uLowerProbeCount;
    if (coord.x >= probeCount * 3 || coord.y >= probeCount * 3 || coord.z >= probeCount) {
        return;
    }
    
    // Get SH coefficient index
    ivec3 probeCoord = coord / ivec3(3, 3, 1);
    ivec3 shOffset = coord - probeCoord * ivec3(3, 3, 1);
    
    // Read lower cascade SH
    vec4 lowerSH = imageLoad(uCascadeLower, coord);
    
    // Sample upper cascade with trilinear
    vec3 upperUV = (vec3(coord) + 0.5) / vec3(uUpperProbeCount * 3, uUpperProbeCount * 3, uUpperProbeCount);
    vec4 upperSH = texture(uCascadeUpper, upperUV);
    
    // Blend: far cascade contributes where near cascade has no hits
    float lowerLum = dot(lowerSH.rgb, vec3(0.299, 0.587, 0.114));
    float upperLum = dot(upperSH.rgb, vec3(0.299, 0.587, 0.114));
    
    vec4 merged = lowerSH;
    if (lowerLum < 0.001 && upperLum > 0.001) {
        merged = upperSH * uBlendFactor;
    } else if (lowerLum > 0.001 && upperLum > 0.001) {
        merged = lowerSH + upperSH * uBlendFactor * 0.5;
    }
    
    imageStore(uCascadeLower, coord, merged);
}
)";

constexpr const char* kResolveShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform image2D uOutputRadiance;
layout(binding = 1) uniform sampler3D uCascade0;
layout(binding = 2) uniform sampler2D uGBufferPosition;
layout(binding = 3) uniform sampler2D uGBufferNormal;

uniform int uProbeCount;
uniform vec3 uGridCenter;
uniform float uGridWorldSize;
uniform vec2 uScreenSize;
uniform float uIntensity;
uniform float uCascadeVoxelSize;

// SH evaluation with cosine lobe for diffuse irradiance
vec3 EvalSH(vec3 sh[9], vec3 normal) {
    float x = normal.x, y = normal.y, z = normal.z;
    
    const float kA0 = 3.141593;
    const float kA1 = 2.094395;
    const float kA2 = 0.785398;
    
    vec3 result = vec3(0.0);
    
    result += sh[0] * 0.282095 * kA0;
    result += sh[1] * 0.488603 * y * kA1;
    result += sh[2] * 0.488603 * z * kA1;
    result += sh[3] * 0.488603 * x * kA1;
    result += sh[4] * 1.092548 * x * y * kA2;
    result += sh[5] * 0.546274 * y * z * kA2;
    result += sh[6] * 0.315392 * (3.0 * z * z - 1.0) * kA2;
    result += sh[7] * 0.546274 * x * z * kA2;
    result += sh[8] * 1.092548 * (x * x - y * y) * kA2;
    
    return max(result, vec3(0.0));
}

vec3 WorldToProbeUV(vec3 worldPos) {
    // Match populate shader: probes cover uGridWorldSize
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
    
    // Skip background
    if (length(worldPos) < 0.01) {
        imageStore(uOutputRadiance, screenCoord, vec4(0.0));
        return;
    }
    
    vec3 probeUV = WorldToProbeUV(worldPos);
    
    // Compute edge fade for smooth falloff at grid boundaries
    vec3 edgeFade = smoothstep(vec3(0.0), vec3(0.05), probeUV) * 
                    smoothstep(vec3(1.0), vec3(0.95), probeUV);
    float fade = edgeFade.x * edgeFade.y * edgeFade.z;
    
    // Clamp to valid range (don't discard, just sample edge probes)
    probeUV = clamp(probeUV, vec3(0.001), vec3(0.999));

    
    // Sample 9 SH coefficients from cascade 0
    // Each probe stores 9 SH coefs in a 3x3 grid of texels (X,Y dimensions)
    // Cascade texture size is probeCount*3 x probeCount*3 x probeCount
    
    vec3 sh[9];
    
    // Convert world position to probe coordinate (0 to probeCount-1)
    vec3 probeCoordF = probeUV * float(uProbeCount) - 0.5;
    ivec3 probeCoord0 = ivec3(floor(probeCoordF));
    vec3 frac = probeCoordF - vec3(probeCoord0);
    
    // Clamp to valid range
    probeCoord0 = clamp(probeCoord0, ivec3(0), ivec3(uProbeCount - 1));
    ivec3 probeCoord1 = min(probeCoord0 + 1, ivec3(uProbeCount - 1));
    
    // For simplicity, use nearest neighbor for now (proper trilinear would sample 8 probes)
    ivec3 nearestProbe = ivec3(round(probeCoordF));
    nearestProbe = clamp(nearestProbe, ivec3(0), ivec3(uProbeCount - 1));
    
    // Each probe's SH is stored starting at probeCoord * 3
    ivec3 baseTexel = nearestProbe * 3;
    vec3 texSize = vec3(uProbeCount * 3, uProbeCount * 3, uProbeCount);
    
    // Sample the 9 SH coefficients
    for (int i = 0; i < 9; i++) {
        int ox = i % 3;
        int oy = i / 3;
        vec3 texCoord = (vec3(baseTexel) + vec3(ox, oy, 0) + 0.5) / texSize;
        sh[i] = texture(uCascade0, texCoord).rgb;
    }
    
    // Evaluate irradiance with edge fade
    vec3 irradiance = EvalSH(sh, normal) * fade;
    
    imageStore(uOutputRadiance, screenCoord, vec4(irradiance * uIntensity, 1.0));

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
    
    SE_LOG_INFO("Initializing SparseRadianceCascades: {} cascades, {}³ bricks/cascade, {}³ probes/brick",
                config_.NumCascades, config_.BaseBrickCount, config_.BrickSize);
    
    // Initialize subsystems
    brickCache_ = std::make_unique<gi::SparseBrickCache>();
    gi::SparseBrickCacheConfig brickConfig;
    brickConfig.numCascades = config_.NumCascades;
    brickConfig.brickSize = config_.BrickSize;
    brickConfig.maxBricksPerCascade = 512;
    brickConfig.maxTotalBricks = 2048;
    brickConfig.updateBudget = config_.UpdateBudget;
    brickConfig.cascadeWorldScale = config_.CascadeScale;
    brickConfig.baseVoxelSize = config_.BaseVoxelSize;
    brickCache_->Init(brickConfig);
    
    rayQuery_ = std::make_unique<gi::VoxelRayQueryBackend>();
    rayQuery_->SetVoxelizer(voxelizer_);
    
    profiler_ = std::make_unique<gi::RCProfiler>();
    profiler_->Init();
    
    debugRenderer_ = std::make_unique<gi::RCDebugRenderer>();
    debugRenderer_->Init();
    
    LoadShaders();
    CreateResources();
    
    initialized_ = true;
    SE_LOG_INFO("SparseRadianceCascades initialized successfully");
}

void SparseRadianceCascades::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down SparseRadianceCascades");
    
    DestroyResources();
    
    populateShader_.reset();
    mergeShader_.reset();
    resolveShader_.reset();
    
    debugRenderer_.reset();
    profiler_.reset();
    rayQuery_.reset();
    brickCache_.reset();
    voxelizer_.reset();
    
    initialized_ = false;
}

void SparseRadianceCascades::Resize(int screenWidth, int screenHeight) {
    if (screenWidth_ == screenWidth && screenHeight_ == screenHeight) return;
    
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;
    
    SE_LOG_INFO("Resizing SparseRadianceCascades to {}x{}", screenWidth, screenHeight);
    
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
                        config.BaseBrickCount != config_.BaseBrickCount ||
                        config.BrickSize != config_.BrickSize;
    config_ = config;
    
    if (initialized_ && needRecreate) {
        DestroyResources();
        CreateResources();
    }
    
    if (debugRenderer_) {
        gi::RCDebugConfig debugConfig;
        debugConfig.mode = config_.DebugMode;
        debugRenderer_->SetConfig(debugConfig);
    }
}

void SparseRadianceCascades::LoadShaders() {
    SE_LOG_INFO("Loading SparseRC shaders");
    
    populateShader_ = std::make_shared<ComputeShader>();
    if (!populateShader_->LoadFromSource(kPopulateShaderSource)) {
        SE_LOG_ERROR("Failed to compile populate shader");
        populateShader_.reset();
    }
    
    mergeShader_ = std::make_shared<ComputeShader>();
    if (!mergeShader_->LoadFromSource(kMergeShaderSource)) {
        SE_LOG_ERROR("Failed to compile merge shader");
        mergeShader_.reset();
    }
    
    resolveShader_ = std::make_shared<ComputeShader>();
    if (!resolveShader_->LoadFromSource(kResolveShaderSource)) {
        SE_LOG_ERROR("Failed to compile resolve shader");
        resolveShader_.reset();
    }
}

void SparseRadianceCascades::CreateResources() {
    cascadeTextures_.resize(config_.NumCascades);
    
    for (int i = 0; i < config_.NumCascades; ++i) {
        int probeCount = config_.BaseBrickCount >> i;
        int texSize = probeCount * 3;
        
        glGenTextures(1, &cascadeTextures_[i]);
        glBindTexture(GL_TEXTURE_3D, cascadeTextures_[i]);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, texSize, texSize, probeCount,
                     0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        SE_LOG_INFO("Created cascade {} SH texture: {}³ probes -> {}x{}x{} texels",
                    i, probeCount, texSize, texSize, probeCount);
    }
    
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    SE_LOG_INFO("Created final radiance texture: {}x{}", screenWidth_, screenHeight_);
}

void SparseRadianceCascades::DestroyResources() {
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

float SparseRadianceCascades::GetCascadeVoxelSize(int cascadeIndex) const {
    return config_.BaseVoxelSize * std::pow(config_.CascadeScale, static_cast<float>(cascadeIndex));
}

void SparseRadianceCascades::GetCascadeInterval(int cascadeIndex, float& start, float& end) const {
    // Each cascade covers a sphere of increasing radius around each probe
    // Cascade 0: 0 to BaseVoxelSize * BrickSize * 4 (cover 4x more area)
    // Each subsequent cascade extends further with overlap
    float baseRange = config_.BaseVoxelSize * config_.BrickSize * 4.0f;  // 4x larger range
    
    if (cascadeIndex == 0) {
        start = 0.0f;
        end = baseRange;
    } else {
        float scale = std::pow(config_.CascadeScale, static_cast<float>(cascadeIndex));
        start = baseRange * std::pow(config_.CascadeScale, static_cast<float>(cascadeIndex - 1)) * 0.5f;  // 50% overlap
        end = baseRange * scale;
    }
}


void SparseRadianceCascades::Execute(const glm::mat4& projection, const glm::mat4& view,
                                      const glm::vec3& cameraPos, uint32_t frameNumber) {
    if (!initialized_ || !config_.Enabled) return;
    if (!voxelizer_ || !voxelizer_->IsInitialized()) return;
    
    currentFrame_ = frameNumber;
    cameraPos_ = cameraPos;
    invProjection_ = glm::inverse(projection);
    invView_ = glm::inverse(view);
    
    profiler_->BeginFrame();
    
    // Update ray query backend with latest voxel data
    rayQuery_->Update();
    
    // Update brick cache - track active bricks based on camera position
    if (brickCache_) {
        brickCache_->BeginFrame(frameNumber, cameraPos);
        
        // Only allocate bricks every few frames to reduce overhead
        if ((frameNumber % 10) == 0) {
            // Allocate bricks around camera for each cascade level
            for (int cascade = 0; cascade < config_.NumCascades; ++cascade) {
                float voxelSize = brickCache_->GetCascadeVoxelSize(cascade);
                float brickWorldSize = voxelSize * config_.BrickSize;
                
                // Smaller allocation volume - 4x4x4 = 64 bricks per cascade
                int halfBricks = 2;
                glm::ivec3 centerBrick = glm::ivec3(glm::floor(cameraPos / brickWorldSize));
                
                int allocated = 0;
                int maxPerCascade = 64;  // Limit allocations per cascade
                
                for (int z = -halfBricks; z < halfBricks && allocated < maxPerCascade; ++z) {
                    for (int y = -halfBricks; y < halfBricks && allocated < maxPerCascade; ++y) {
                        for (int x = -halfBricks; x < halfBricks && allocated < maxPerCascade; ++x) {
                            gi::BrickKey key;
                            key.cascadeLevel = cascade;
                            key.coord = centerBrick + glm::ivec3(x, y, z);
                            
                            if (!brickCache_->HasBrick(key)) {
                                if (brickCache_->AllocateBrick(key) != UINT32_MAX) {
                                    allocated++;
                                }
                            } else {
                                brickCache_->TouchBrick(key);
                            }
                        }
                    }
                }
            }
        }
        
        brickCache_->UpdateActiveBricks(cameraPos, config_.ActivationRadius);
    }


    
    // Step 1: Populate all cascades with radiance
    profiler_->BeginVoxelization();
    PopulateCascades(cameraPos);
    profiler_->EndVoxelization();
    
    // Step 2: Merge cascades from far to near
    profiler_->BeginCascadeMerge();
    MergeCascades();
    profiler_->EndCascadeMerge();
    
    // Step 3: Resolve to screen-space
    profiler_->BeginResolve();
    ResolveToScreen();
    profiler_->EndResolve();
    
    // End brick cache frame
    if (brickCache_) {
        brickCache_->EndFrame();
    }
    
    profiler_->EndFrame();
}


void SparseRadianceCascades::PopulateCascades(const glm::vec3& cameraPos) {
    if (!populateShader_ || !populateShader_->IsValid()) return;
    
    const auto& voxelConfig = voxelizer_->GetConfig();
    
    // Cache grid params for use by resolve (ensure populate and resolve use same values)
    cachedGridCenter_ = voxelConfig.Center;
    cachedGridSize_ = voxelConfig.WorldSize;
    
    populateShader_->Bind();

    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelTexture());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelEmissiveTexture());
    
    for (int i = 0; i < config_.NumCascades; ++i) {
        profiler_->BeginCascadeUpdate(i);
        
        int probeCount = config_.BaseBrickCount >> i;
        float voxelSize = GetCascadeVoxelSize(i);
        float intervalStart, intervalEnd;
        GetCascadeInterval(i, intervalStart, intervalEnd);
        
        glBindImageTexture(0, cascadeTextures_[i], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        
        populateShader_->SetInt("uCascadeIndex", i);
        populateShader_->SetInt("uProbeCount", probeCount);
        populateShader_->SetInt("uBrickSize", config_.BrickSize);
        populateShader_->SetFloat("uIntervalStart", intervalStart);
        populateShader_->SetFloat("uIntervalEnd", intervalEnd);
        populateShader_->SetVec3("uGridCenter", voxelConfig.Center);
        populateShader_->SetFloat("uGridWorldSize", voxelConfig.WorldSize);
        populateShader_->SetInt("uGridResolution", voxelConfig.Resolution);
        populateShader_->SetVec3("uCameraPos", cameraPos);
        populateShader_->SetFloat("uCascadeVoxelSize", voxelSize);
        populateShader_->SetInt("uNumSamples", config_.SHSamplesPerProbe);
        
        uint32_t groupsX = (probeCount + 3) / 4;
        uint32_t groupsY = (probeCount + 3) / 4;
        uint32_t groupsZ = (probeCount + 3) / 4;
        
        populateShader_->DispatchAndWait(groupsX, groupsY, groupsZ);
        
        profiler_->EndCascadeUpdate(i);
        profiler_->RecordActiveBricks(i, probeCount * probeCount * probeCount);
    }
}

void SparseRadianceCascades::MergeCascades() {
    if (!mergeShader_ || !mergeShader_->IsValid()) return;
    
    mergeShader_->Bind();
    
    for (int i = config_.NumCascades - 2; i >= 0; --i) {
        int lowerProbeCount = config_.BaseBrickCount >> i;
        int upperProbeCount = config_.BaseBrickCount >> (i + 1);
        
        glBindImageTexture(0, cascadeTextures_[i], 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_3D, cascadeTextures_[i + 1]);
        
        mergeShader_->SetInt("uLowerProbeCount", lowerProbeCount);
        mergeShader_->SetInt("uUpperProbeCount", upperProbeCount);
        mergeShader_->SetFloat("uBlendFactor", 0.5f);
        
        int texSize = lowerProbeCount * 3;
        uint32_t groupsX = (texSize + 3) / 4;
        uint32_t groupsY = (texSize + 3) / 4;
        uint32_t groupsZ = (lowerProbeCount + 3) / 4;
        
        mergeShader_->DispatchAndWait(groupsX, groupsY, groupsZ);
    }
}

void SparseRadianceCascades::ResolveToScreen() {
    if (!resolveShader_ || !resolveShader_->IsValid()) return;
    if (!gbuffer_) {
        SE_LOG_WARN("GBuffer not set, cannot resolve radiance");
        return;
    }
    
    int probeCount = config_.BaseBrickCount;
    float voxelSize = GetCascadeVoxelSize(0);

    
    resolveShader_->Bind();
    
    glBindImageTexture(0, finalRadianceTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, cascadeTextures_[0]);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->GetPositionTexture());
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gbuffer_->GetNormalTexture());
    
    // Use cached grid params from populate (ensures populate and resolve use same values)
    resolveShader_->SetInt("uProbeCount", probeCount);
    resolveShader_->SetVec3("uGridCenter", cachedGridCenter_);
    resolveShader_->SetFloat("uGridWorldSize", cachedGridSize_);
    resolveShader_->SetVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetFloat("uIntensity", config_.GIIntensity);
    resolveShader_->SetFloat("uCascadeVoxelSize", voxelSize);
    
    SE_LOG_INFO("ResolveToScreen: probeCount={}, cachedGridCenter=({},{},{}), cachedGridSize={}",
                probeCount, cachedGridCenter_.x, cachedGridCenter_.y, cachedGridCenter_.z,
                cachedGridSize_);

    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
}

void SparseRadianceCascades::RenderDebug(const glm::mat4& viewProj) {
    if (!debugRenderer_ || config_.DebugMode == gi::RCDebugMode::Off) return;
    
    // Sync debug config
    gi::RCDebugConfig debugConfig;
    debugConfig.mode = config_.DebugMode;
    debugConfig.opacity = 0.8f;
    debugRenderer_->SetConfig(debugConfig);
    
    SE_LOG_INFO("RenderDebug: mode={}, brickCache={}", 
                static_cast<int>(config_.DebugMode), brickCache_ != nullptr);
    
    debugRenderer_->Render(viewProj, brickCache_.get(), finalRadianceTex_);
}


std::string SparseRadianceCascades::GetProfileSummary() const {
    if (profiler_) {
        return profiler_->GetSummaryString();
    }
    return "Profiler not available";
}

}  // namespace se
