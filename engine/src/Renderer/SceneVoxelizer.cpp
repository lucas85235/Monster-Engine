#include "engine/renderer/SceneVoxelizer.h"
#include "engine/renderer/ComputeShader.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

#include <glad/glad.h>

namespace se {

namespace {

constexpr const char* kClearVoxelShaderSource = R"(#version 430 core

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;

layout(rgba8, binding = 0) uniform image3D uVoxelAlbedo;
layout(rgba16f, binding = 1) uniform image3D uVoxelEmissive;

void main() {
    ivec3 pos = ivec3(gl_GlobalInvocationID.xyz);
    imageStore(uVoxelAlbedo, pos, vec4(0.0));
    imageStore(uVoxelEmissive, pos, vec4(0.0));
}
)";

constexpr const char* kVoxelizeShaderSource = R"(#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba8, binding = 0) uniform image3D uVoxelAlbedo;
layout(rgba16f, binding = 1) uniform image3D uVoxelEmissive;

// G-Buffer textures from scene rendering
layout(binding = 2) uniform sampler2D uGBufferPosition;  // World position
layout(binding = 3) uniform sampler2D uGBufferAlbedo;    // Albedo
layout(binding = 4) uniform sampler2D uGBufferEmissive;  // Emissive

uniform vec3 uGridCenter;
uniform float uGridWorldSize;
uniform int uGridResolution;
uniform vec2 uScreenSize;

ivec3 WorldToVoxel(vec3 worldPos) {
    vec3 localPos = worldPos - uGridCenter;
    vec3 normalizedPos = (localPos / uGridWorldSize) + 0.5;
    return ivec3(normalizedPos * float(uGridResolution));
}

bool IsValidVoxel(ivec3 voxel) {
    return all(greaterThanEqual(voxel, ivec3(0))) && 
           all(lessThan(voxel, ivec3(uGridResolution)));
}

void main() {
    ivec2 screenCoord = ivec2(gl_GlobalInvocationID.xy);
    if (screenCoord.x >= int(uScreenSize.x) || screenCoord.y >= int(uScreenSize.y)) {
        return;
    }
    
    vec2 uv = (vec2(screenCoord) + 0.5) / uScreenSize;
    
    // Sample G-Buffer
    vec3 worldPos = texture(uGBufferPosition, uv).xyz;
    vec4 albedo = texture(uGBufferAlbedo, uv);
    vec4 emissive = texture(uGBufferEmissive, uv);
    
    // Skip if no geometry
    if (length(worldPos) < 0.01) {
        return;
    }
    
    // Convert to voxel coordinates
    ivec3 voxelCoord = WorldToVoxel(worldPos);
    
    if (!IsValidVoxel(voxelCoord)) {
        return;
    }
    
    // Write to voxel grid (atomic max for opacity)
    imageStore(uVoxelAlbedo, voxelCoord, vec4(albedo.rgb, 1.0));
    
    // Store emissive if present
    float emissiveLuminance = dot(emissive.rgb, vec3(0.299, 0.587, 0.114));
    if (emissiveLuminance > 0.001) {
        imageStore(uVoxelEmissive, voxelCoord, vec4(emissive.rgb, 1.0));
    }
}
)";

}  // namespace

SceneVoxelizer::SceneVoxelizer() = default;

SceneVoxelizer::~SceneVoxelizer() {
    Shutdown();
}

void SceneVoxelizer::Init(const VoxelGridConfig& config) {
    if (initialized_) {
        SE_LOG_WARN("SceneVoxelizer already initialized");
        return;
    }
    
    config_ = config;
    
    SE_LOG_INFO("Initializing SceneVoxelizer: {}³ resolution, {} world units", 
                config_.Resolution, config_.WorldSize);
    
    CreateTextures();
    LoadShaders();
    
    initialized_ = true;
}

void SceneVoxelizer::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down SceneVoxelizer");
    
    DestroyTextures();
    clearShader_.reset();
    voxelizeShader_.reset();
    
    initialized_ = false;
}

void SceneVoxelizer::CreateTextures() {
    int res = config_.Resolution;
    
    // Albedo + opacity texture (RGBA8)
    glGenTextures(1, &voxelTexture_);
    glBindTexture(GL_TEXTURE_3D, voxelTexture_);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, res, res, res, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    // Emissive texture (RGBA16F)
    glGenTextures(1, &voxelEmissiveTexture_);
    glBindTexture(GL_TEXTURE_3D, voxelEmissiveTexture_);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, res, res, res, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    SE_LOG_INFO("Created voxel textures: albedo={}, emissive={}", voxelTexture_, voxelEmissiveTexture_);
}

void SceneVoxelizer::DestroyTextures() {
    if (voxelTexture_ != 0) {
        glDeleteTextures(1, &voxelTexture_);
        voxelTexture_ = 0;
    }
    if (voxelEmissiveTexture_ != 0) {
        glDeleteTextures(1, &voxelEmissiveTexture_);
        voxelEmissiveTexture_ = 0;
    }
}

void SceneVoxelizer::LoadShaders() {
    clearShader_ = std::make_shared<ComputeShader>();
    if (!clearShader_->LoadFromSource(kClearVoxelShaderSource)) {
        SE_LOG_ERROR("Failed to compile voxel clear shader");
        clearShader_.reset();
    }
    
    voxelizeShader_ = std::make_shared<ComputeShader>();
    if (!voxelizeShader_->LoadFromSource(kVoxelizeShaderSource)) {
        SE_LOG_ERROR("Failed to compile voxelization shader");
        voxelizeShader_.reset();
    }
}

void SceneVoxelizer::Clear() {
    if (!initialized_ || !clearShader_ || !clearShader_->IsValid()) return;
    
    clearShader_->Bind();
    
    glBindImageTexture(0, voxelTexture_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glBindImageTexture(1, voxelEmissiveTexture_, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    int groups = (config_.Resolution + 3) / 4;
    clearShader_->DispatchAndWait(groups, groups, groups);
}

void SceneVoxelizer::Voxelize(Scene& scene, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
    if (!initialized_) return;
    
    // Clear previous voxels
    Clear();
    
    // Note: This implementation uses G-Buffer data which must be rendered first
    // The actual voxelization happens via compute shader sampling the G-Buffer
    // This is called from SceneRenderer after G-Buffer pass
}

void SceneVoxelizer::VoxelizeFromGBuffer(uint32_t positionTex, uint32_t albedoTex, uint32_t emissiveTex,
                                          int screenWidth, int screenHeight) {
    if (!initialized_ || !voxelizeShader_ || !voxelizeShader_->IsValid()) {
        SE_LOG_WARN("VoxelizeFromGBuffer: not ready (init={}, shader={})", 
                    initialized_, voxelizeShader_ != nullptr);
        return;
    }
    
    // Clear first
    Clear();
    
    voxelizeShader_->Bind();
    
    // Bind output voxel textures
    glBindImageTexture(0, voxelTexture_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
    glBindImageTexture(1, voxelEmissiveTexture_, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA16F);
    
    // Bind GBuffer input textures
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, positionTex);
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, albedoTex);
    
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, emissiveTex);
    
    // Set uniforms
    voxelizeShader_->SetVec3("uGridCenter", config_.Center);
    voxelizeShader_->SetFloat("uGridWorldSize", config_.WorldSize);
    voxelizeShader_->SetInt("uGridResolution", config_.Resolution);
    voxelizeShader_->SetVec2("uScreenSize", glm::vec2(screenWidth, screenHeight));
    
    // Dispatch compute shader
    int groupsX = (screenWidth + 7) / 8;
    int groupsY = (screenHeight + 7) / 8;
    voxelizeShader_->DispatchAndWait(groupsX, groupsY, 1);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

glm::ivec3 SceneVoxelizer::WorldToVoxel(const glm::vec3& worldPos) const {
    glm::vec3 localPos = worldPos - config_.Center;
    glm::vec3 normalizedPos = (localPos / config_.WorldSize) + 0.5f;
    return glm::ivec3(normalizedPos * float(config_.Resolution));
}

glm::vec3 SceneVoxelizer::VoxelToWorld(const glm::ivec3& voxelPos) const {
    glm::vec3 normalizedPos = glm::vec3(voxelPos) / float(config_.Resolution);
    glm::vec3 localPos = (normalizedPos - 0.5f) * config_.WorldSize;
    return localPos + config_.Center;
}

}  // namespace se

