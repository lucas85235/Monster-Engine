#include "engine/renderer/SSGIPass.h"
#include "engine/renderer/ComputeShader.h"
#include "engine/Log.h"

#include <cstdio>
#include <glad/glad.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

namespace se {

// Raymarch compute shader source
static const char* kRaymarchShaderSource = R"(
#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

// SH coefficient outputs (L1 = 4 coefficients)
layout(rgba16f, binding = 0) uniform writeonly image2D uSH0;
layout(rgba16f, binding = 1) uniform writeonly image2D uSH1;
layout(rgba16f, binding = 2) uniform writeonly image2D uSH2;
layout(rgba16f, binding = 3) uniform writeonly image2D uSH3;

// Input textures
layout(binding = 0) uniform sampler2D uPositionTex;
layout(binding = 1) uniform sampler2D uNormalTex;
layout(binding = 2) uniform sampler2D uAlbedoTex;
layout(binding = 3) uniform sampler2D uEmissiveTex;
layout(binding = 4) uniform sampler2D uDepthTex;

// Uniforms
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uInvProjection;
uniform mat4 uInvView;
uniform vec3 uCameraPos;
uniform vec2 uResolution;
uniform int uRayCount;
uniform int uStepsPerRay;
uniform float uMaxDistance;
uniform float uFalloffExponent;
uniform float uIntensity;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;

// SH basis functions (L1)
vec4 shBasis(vec3 dir) {
    return vec4(
        0.282095,              // L0
        0.488603 * dir.y,      // L1-1
        0.488603 * dir.z,      // L1+0
        0.488603 * dir.x       // L1+1
    );
}

// Project world position to screen UV
vec2 projectToScreen(vec3 worldPos) {
    vec4 clipPos = uProjection * uView * vec4(worldPos, 1.0);
    clipPos.xyz /= clipPos.w;
    return clipPos.xy * 0.5 + 0.5;
}

// Sample scene radiance at screen UV with LOD
vec3 sampleRadiance(vec2 uv, float lod) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec3(0.0);
    }
    
    // Sample albedo and emissive
    vec3 albedo = textureLod(uAlbedoTex, uv, lod).rgb;
    vec3 emissive = textureLod(uEmissiveTex, uv, lod).rgb;
    
    // Use emissive directly, add some ambient from albedo
    return emissive + albedo * 0.1;
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (float(pixelCoord.x) >= uResolution.x || float(pixelCoord.y) >= uResolution.y) {
        return;
    }
    
    vec2 uv = (vec2(pixelCoord) + 0.5) / uResolution;
    
    // Get surface data
    vec3 worldPos = texture(uPositionTex, uv).xyz;
    vec3 normal = normalize(texture(uNormalTex, uv).xyz);
    float depth = texture(uDepthTex, uv).r;
    
    // Skip sky pixels
    if (depth >= 1.0) {
        imageStore(uSH0, pixelCoord, vec4(0.0));
        imageStore(uSH1, pixelCoord, vec4(0.0));
        imageStore(uSH2, pixelCoord, vec4(0.0));
        imageStore(uSH3, pixelCoord, vec4(0.0));
        return;
    }
    
    // Accumulate SH coefficients
    vec3 sh0 = vec3(0.0);
    vec3 sh1 = vec3(0.0);
    vec3 sh2 = vec3(0.0);
    vec3 sh3 = vec3(0.0);
    
    float totalWeight = 0.0;
    
    // Generate rays in hemisphere around normal
    for (int i = 0; i < uRayCount; i++) {
        // Golden angle spiral for uniform distribution
        float t = float(i) / float(uRayCount);
        float phi = TWO_PI * t * 6.180339887;  // Golden ratio
        float cosTheta = 1.0 - t;
        float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
        
        // Direction in tangent space
        vec3 tangentDir = vec3(
            sinTheta * cos(phi),
            sinTheta * sin(phi),
            cosTheta
        );
        
        // Build TBN matrix
        vec3 up = abs(normal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        vec3 tangent = normalize(cross(up, normal));
        vec3 bitangent = cross(normal, tangent);
        mat3 TBN = mat3(tangent, bitangent, normal);
        
        // World-space ray direction
        vec3 rayDir = normalize(TBN * tangentDir);
        
        // Skip rays going away from hemisphere
        if (dot(rayDir, normal) <= 0.0) continue;
        
        // Raymarch
        vec3 accumulatedRadiance = vec3(0.0);
        float rayWeight = 0.0;
        
        for (int step = 1; step <= uStepsPerRay; step++) {
            float stepT = float(step) / float(uStepsPerRay);
            float dist = stepT * uMaxDistance;
            
            vec3 samplePos = worldPos + rayDir * dist + normal * 0.1;
            vec2 sampleUV = projectToScreen(samplePos);
            
            // Check valid screen bounds
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }
            
            // Check depth against scene
            vec3 scenePosAtSample = texture(uPositionTex, sampleUV).xyz;
            float distToScene = length(scenePosAtSample - samplePos);
            
            // If close to a surface, sample its radiance
            if (distToScene < 0.5) {
                float distFalloff = pow(1.0 - stepT, uFalloffExponent);
                float lod = stepT * 4.0;  // Use higher mip for distant samples
                vec3 radiance = sampleRadiance(sampleUV, lod);
                accumulatedRadiance += radiance * distFalloff;
                rayWeight += distFalloff;
                break;  // Hit something, stop marching
            }
        }
        
        if (rayWeight > 0.0) {
            accumulatedRadiance /= rayWeight;
            
            // Project onto SH basis
            vec4 basis = shBasis(rayDir);
            float NoL = max(dot(normal, rayDir), 0.0);
            vec3 contribution = accumulatedRadiance * NoL;
            
            sh0 += contribution * basis.x;
            sh1 += contribution * basis.y;
            sh2 += contribution * basis.z;
            sh3 += contribution * basis.w;
            totalWeight += 1.0;
        }
    }
    
    // Normalize
    if (totalWeight > 0.0) {
        float invWeight = uIntensity / totalWeight;
        sh0 *= invWeight;
        sh1 *= invWeight;
        sh2 *= invWeight;
        sh3 *= invWeight;
    }
    
    // Store SH coefficients
    imageStore(uSH0, pixelCoord, vec4(sh0, 1.0));
    imageStore(uSH1, pixelCoord, vec4(sh1, 0.0));
    imageStore(uSH2, pixelCoord, vec4(sh2, 0.0));
    imageStore(uSH3, pixelCoord, vec4(sh3, 0.0));
}
)";

// Resolve compute shader source (bilateral upscale + SH decode)
static const char* kResolveShaderSource = R"(
#version 430 core

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(rgba16f, binding = 0) uniform writeonly image2D uOutputTex;

// SH coefficient inputs (at work resolution)
layout(binding = 0) uniform sampler2D uSH0;
layout(binding = 1) uniform sampler2D uSH1;
layout(binding = 2) uniform sampler2D uSH2;
layout(binding = 3) uniform sampler2D uSH3;

// Full-res normal and depth for bilateral weighting
layout(binding = 4) uniform sampler2D uNormalTex;
layout(binding = 5) uniform sampler2D uDepthTex;

uniform vec2 uOutputResolution;
uniform vec2 uWorkResolution;
uniform int uBlurRadius;
uniform float uDepthThreshold;
uniform float uNormalThreshold;
uniform int uDebugMode;

// SH basis functions (L1)
vec4 shBasis(vec3 dir) {
    return vec4(
        0.282095,              // L0
        0.488603 * dir.y,      // L1-1
        0.488603 * dir.z,      // L1+0
        0.488603 * dir.x       // L1+1
    );
}

// Decode SH to irradiance for a given direction
vec3 decodeSH(vec3 sh0, vec3 sh1, vec3 sh2, vec3 sh3, vec3 dir) {
    vec4 basis = shBasis(dir);
    return max(vec3(0.0), 
        sh0 * basis.x +
        sh1 * basis.y +
        sh2 * basis.z +
        sh3 * basis.w
    );
}

void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (float(pixelCoord.x) >= uOutputResolution.x || float(pixelCoord.y) >= uOutputResolution.y) {
        return;
    }
    
    vec2 uv = (vec2(pixelCoord) + 0.5) / uOutputResolution;
    
    // Get full-res surface data
    vec3 normal = normalize(texture(uNormalTex, uv).xyz);
    float depth = texture(uDepthTex, uv).r;
    
    // Debug modes
    if (uDebugMode == 3) {
        imageStore(uOutputTex, pixelCoord, vec4(normal * 0.5 + 0.5, 1.0));
        return;
    }
    
    // Skip sky
    if (depth >= 1.0) {
        imageStore(uOutputTex, pixelCoord, vec4(0.0, 0.0, 0.0, 1.0));
        return;
    }
    
    // Bilateral upscale
    vec3 totalSH0 = vec3(0.0);
    vec3 totalSH1 = vec3(0.0);
    vec3 totalSH2 = vec3(0.0);
    vec3 totalSH3 = vec3(0.0);
    float totalWeight = 0.0;
    
    vec2 workUV = (vec2(pixelCoord) + 0.5) / uOutputResolution;
    
    for (int dy = -uBlurRadius; dy <= uBlurRadius; dy++) {
        for (int dx = -uBlurRadius; dx <= uBlurRadius; dx++) {
            vec2 offset = vec2(float(dx), float(dy)) / uWorkResolution;
            vec2 sampleUV = workUV + offset;
            
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }
            
            // Sample neighbor normal and depth (at full res for accuracy)
            vec2 fullResSampleUV = sampleUV;
            vec3 sampleNormal = normalize(texture(uNormalTex, fullResSampleUV).xyz);
            float sampleDepth = texture(uDepthTex, fullResSampleUV).r;
            
            // Bilateral weights
            float normalWeight = pow(max(0.0, dot(normal, sampleNormal)), 32.0);
            float depthWeight = exp(-abs(depth - sampleDepth) / uDepthThreshold);
            float spatialWeight = exp(-float(dx*dx + dy*dy) / float(uBlurRadius * uBlurRadius + 1));
            
            float weight = normalWeight * depthWeight * spatialWeight;
            
            if (weight < 0.001) continue;
            
            // Sample SH coefficients
            totalSH0 += texture(uSH0, sampleUV).rgb * weight;
            totalSH1 += texture(uSH1, sampleUV).rgb * weight;
            totalSH2 += texture(uSH2, sampleUV).rgb * weight;
            totalSH3 += texture(uSH3, sampleUV).rgb * weight;
            totalWeight += weight;
        }
    }
    
    if (totalWeight > 0.0) {
        totalSH0 /= totalWeight;
        totalSH1 /= totalWeight;
        totalSH2 /= totalWeight;
        totalSH3 /= totalWeight;
    }
    
    // Debug mode: show SH coefficients
    if (uDebugMode == 1) {
        vec3 shViz = abs(totalSH0) + abs(totalSH1) + abs(totalSH2) + abs(totalSH3);
        imageStore(uOutputTex, pixelCoord, vec4(shViz, 1.0));
        return;
    }
    
    // Decode SH using surface normal
    vec3 irradiance = decodeSH(totalSH0, totalSH1, totalSH2, totalSH3, normal);
    
    // Debug mode: raw radiance
    if (uDebugMode == 2) {
        imageStore(uOutputTex, pixelCoord, vec4(irradiance * 2.0, 1.0));
        return;
    }
    
    imageStore(uOutputTex, pixelCoord, vec4(irradiance, 1.0));
}
)";

SSGIPass::SSGIPass() = default;

SSGIPass::~SSGIPass() {
    Shutdown();
}

bool SSGIPass::Init(int width, int height) {
    if (initialized_) {
        SE_LOG_WARN("[SSGI] Already initialized, call Shutdown first");
        return false;
    }
    
    screenWidth_ = width;
    screenHeight_ = height;
    workWidth_ = static_cast<int>(width * config_.ResolutionScale);
    workHeight_ = static_cast<int>(height * config_.ResolutionScale);
    
    SE_LOG_INFO("[SSGI] Initializing SSGI pass ({}x{} -> {}x{} work resolution)",
                width, height, workWidth_, workHeight_);
    
    // Create compute shaders
    raymarchShader_ = std::make_shared<ComputeShader>();
    if (!raymarchShader_->LoadFromSource(kRaymarchShaderSource)) {
        SE_LOG_ERROR("[SSGI] Failed to compile raymarch shader");
        return false;
    }
    
    resolveShader_ = std::make_shared<ComputeShader>();
    if (!resolveShader_->LoadFromSource(kResolveShaderSource)) {
        SE_LOG_ERROR("[SSGI] Failed to compile resolve shader");
        return false;
    }
    
    CreateTextures();
    
    initialized_ = true;
    SE_LOG_INFO("[SSGI] SSGI pass initialized successfully");
    return true;
}

void SSGIPass::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("[SSGI] Shutting down SSGI pass");
    
    DestroyTextures();
    
    raymarchShader_.reset();
    resolveShader_.reset();
    
    initialized_ = false;
}

void SSGIPass::Resize(int width, int height) {
    if (screenWidth_ == width && screenHeight_ == height) return;
    
    screenWidth_ = width;
    screenHeight_ = height;
    workWidth_ = static_cast<int>(width * config_.ResolutionScale);
    workHeight_ = static_cast<int>(height * config_.ResolutionScale);
    
    SE_LOG_INFO("[SSGI] Resizing to {}x{} (work: {}x{})", 
                width, height, workWidth_, workHeight_);
    
    DestroyTextures();
    CreateTextures();
}

void SSGIPass::CreateTextures() {
    // SH coefficient textures at work resolution
    for (int i = 0; i < 4; i++) {
        glGenTextures(1, &shCoeffTex_[i]);
        glBindTexture(GL_TEXTURE_2D, shCoeffTex_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, workWidth_, workHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // Final radiance texture at full resolution
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    SE_LOG_DEBUG("[SSGI] Created textures: SH[4] at {}x{}, Final at {}x{}",
                 workWidth_, workHeight_, screenWidth_, screenHeight_);
}

void SSGIPass::DestroyTextures() {
    for (int i = 0; i < 4; i++) {
        if (shCoeffTex_[i]) {
            glDeleteTextures(1, &shCoeffTex_[i]);
            shCoeffTex_[i] = 0;
        }
    }
    
    if (finalRadianceTex_) {
        glDeleteTextures(1, &finalRadianceTex_);
        finalRadianceTex_ = 0;
    }
}

void SSGIPass::Execute(
    uint32_t positionTex,
    uint32_t normalTex,
    uint32_t albedoTex,
    uint32_t emissiveTex,
    uint32_t depthTex,
    const glm::mat4& projection,
    const glm::mat4& view,
    const glm::mat4& invProjection,
    const glm::mat4& invView,
    const glm::vec3& cameraPos
) {
    if (!initialized_ || !config_.Enabled) return;
    
    // Debug log first few frames
    static int execCount = 0;
    if (execCount++ < 5) {
        printf("[SSGI] Execute: initialized=%d, pos=%u, norm=%u, albedo=%u, emiss=%u, depth=%u, radiance=%u\n",
            initialized_, positionTex, normalTex, albedoTex, emissiveTex, depthTex, finalRadianceTex_);
    }
    
    // Cache inputs
    lastPositionTex_ = positionTex;
    lastNormalTex_ = normalTex;
    lastAlbedoTex_ = albedoTex;
    lastEmissiveTex_ = emissiveTex;
    lastDepthTex_ = depthTex;
    projection_ = projection;
    view_ = view;
    invProjection_ = invProjection;
    invView_ = invView;
    cameraPos_ = cameraPos;
    
    // Step 1: Raymarch and encode to SH
    RaymarchAndEncode();
    
    // Step 2: Bilateral upscale and resolve
    BilateralResolve();
}

void SSGIPass::RaymarchAndEncode() {
    if (!raymarchShader_ || !raymarchShader_->IsValid()) return;
    
    raymarchShader_->Bind();
    
    // Bind output SH textures
    glBindImageTexture(0, shCoeffTex_[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glBindImageTexture(1, shCoeffTex_[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glBindImageTexture(2, shCoeffTex_[2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    glBindImageTexture(3, shCoeffTex_[3], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Bind input textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lastPositionTex_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, lastNormalTex_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, lastAlbedoTex_);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, lastEmissiveTex_);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, lastDepthTex_);
    
    // Set uniforms
    raymarchShader_->SetMat4("uProjection", projection_);
    raymarchShader_->SetMat4("uView", view_);
    raymarchShader_->SetMat4("uInvProjection", invProjection_);
    raymarchShader_->SetMat4("uInvView", invView_);
    raymarchShader_->SetVec3("uCameraPos", cameraPos_);
    raymarchShader_->SetVec2("uResolution", glm::vec2(workWidth_, workHeight_));
    raymarchShader_->SetInt("uRayCount", config_.RayCount);
    raymarchShader_->SetInt("uStepsPerRay", config_.StepsPerRay);
    raymarchShader_->SetFloat("uMaxDistance", config_.MaxDistance);
    raymarchShader_->SetFloat("uFalloffExponent", config_.FalloffExponent);
    raymarchShader_->SetFloat("uIntensity", config_.Intensity);
    
    uint32_t groupsX = (workWidth_ + 7) / 8;
    uint32_t groupsY = (workHeight_ + 7) / 8;
    raymarchShader_->DispatchAndWait(groupsX, groupsY, 1);
}

void SSGIPass::BilateralResolve() {
    if (!resolveShader_ || !resolveShader_->IsValid()) return;
    
    resolveShader_->Bind();
    
    // Bind output
    glBindImageTexture(0, finalRadianceTex_, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
    
    // Bind SH inputs
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shCoeffTex_[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shCoeffTex_[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shCoeffTex_[2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shCoeffTex_[3]);
    
    // Full-res normal and depth
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, lastNormalTex_);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, lastDepthTex_);
    
    // Set uniforms
    resolveShader_->SetVec2("uOutputResolution", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetVec2("uWorkResolution", glm::vec2(workWidth_, workHeight_));
    resolveShader_->SetInt("uBlurRadius", config_.BlurRadius);
    resolveShader_->SetFloat("uDepthThreshold", config_.DepthThreshold);
    resolveShader_->SetFloat("uNormalThreshold", config_.NormalThreshold);
    resolveShader_->SetInt("uDebugMode", config_.DebugMode);
    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
}

} // namespace se
