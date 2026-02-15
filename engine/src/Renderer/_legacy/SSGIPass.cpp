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

// Light uniforms for proper sun contribution
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform float uLightIntensity;

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
// Includes direct lighting contribution for proper indirect illumination
vec3 sampleRadiance(vec2 uv, float lod) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        return vec3(0.0);
    }
    
    // Sample surface data
    vec3 albedo = textureLod(uAlbedoTex, uv, lod).rgb;
    vec3 emissive = textureLod(uEmissiveTex, uv, lod).rgb;
    vec3 sampleNormal = textureLod(uNormalTex, uv, lod).xyz;
    
    // Skip invalid samples
    if (length(sampleNormal) < 0.01) {
        return vec3(0.0);
    }
    sampleNormal = normalize(sampleNormal);
    
    // Use actual scene light direction and color
    vec3 lightDir = normalize(uLightDirection);
    float NdotL = max(dot(sampleNormal, lightDir), 0.0);
    vec3 directLight = albedo * uLightColor * NdotL * uLightIntensity * 0.3;
    
    // Combine: emissive + direct light + ambient from albedo
    return emissive + directLight + albedo * 0.1;
}


void main() {
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (float(pixelCoord.x) >= uResolution.x || float(pixelCoord.y) >= uResolution.y) {
        return;
    }
    
    vec2 uv = (vec2(pixelCoord) + 0.5) / uResolution;
    
    // Get surface data from G-Buffer
    vec3 worldPos = texture(uPositionTex, uv).xyz;
    vec4 normalData = texture(uNormalTex, uv);
    vec3 albedo = texture(uAlbedoTex, uv).rgb;
    
    vec3 normal = normalize(normalData.xyz);

    // Use position to detect sky instead of depth (depth texture has issues)
    // If position is at origin or normal is invalid, this is sky/empty
    bool isSky = length(worldPos) < 0.01 || length(normalData.xyz) < 0.01;
    
    if (isSky) {
        imageStore(uSH0, pixelCoord, vec4(0.0));
        imageStore(uSH1, pixelCoord, vec4(0.0));
        imageStore(uSH2, pixelCoord, vec4(0.0));
        imageStore(uSH3, pixelCoord, vec4(0.0));
        return;
    }
    
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
            
            // Offset along normal to avoid self-intersection at origin
            vec3 samplePos = worldPos + rayDir * dist + normal * 0.05;
            vec2 sampleUV = projectToScreen(samplePos);
            
            // Check valid screen bounds
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }
            
            // Get scene data at sample position
            vec3 scenePosAtSample = texture(uPositionTex, sampleUV).xyz;
            
            // Skip invalid surface (sky/cleared buffer)
            if (length(scenePosAtSample) < 0.01) continue;

            vec3 sceneNormal = normalize(texture(uNormalTex, sampleUV).xyz);
            
            // Depth/Distance test
            float rayDist = length(samplePos - uCameraPos);
            float surfDist = length(scenePosAtSample - uCameraPos);
            
            // Adaptive thickness based on step size to prevent stepping over thin objects
            float stepSize = uMaxDistance / float(uStepsPerRay);
            float thickness = max(0.5, stepSize * 1.5);
            
            // Front face check: Ray should hit front of surface (opposing normal)
            float rayDotNormal = dot(rayDir, sceneNormal);
            bool isFrontFace = rayDotNormal < 0.1;
            
            // Hit condition: Ray is behind visible surface, within thickness, and hitting front face
            if (rayDist > surfDist && rayDist < surfDist + thickness && isFrontFace) {
                float distFalloff = pow(1.0 - stepT, uFalloffExponent);
                float lod = stepT * 4.0;
                vec3 radiance = sampleRadiance(sampleUV, lod);
                
                // Clamp radiance to prevent SH ringing/rainbow artifacts from strong highlights
                radiance = clamp(radiance, vec3(0.0), vec3(10.0));
                
                accumulatedRadiance += radiance * distFalloff;
                rayWeight += distFalloff;
                break;
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

// Full-res G-Buffer for bilateral weighting and debug
layout(binding = 4) uniform sampler2D uNormalTex;
layout(binding = 5) uniform sampler2D uDepthTex;
layout(binding = 6) uniform sampler2D uPositionTex;

uniform vec2 uOutputResolution;
uniform vec2 uWorkResolution;
uniform int uBlurRadius;
uniform float uDepthThreshold;
uniform float uNormalThreshold;
uniform int uDebugMode;
uniform vec3 uCameraPos;


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
    vec4 normalData = texture(uNormalTex, uv);
    vec3 normal = normalize(normalData.xyz);
    
    // Use normal to detect sky instead of depth (depth texture has issues)
    bool isSky = length(normalData.xyz) < 0.01;
    
    // Debug modes
    if (uDebugMode == 3) {
        // Raw normal texture (without normalize, just abs to see)
        imageStore(uOutputTex, pixelCoord, vec4(abs(normalData.xyz), 1.0));
        return;
    }
    
    if (uDebugMode == 4) {
        // Depth visualization using position distance from camera
        vec3 worldPos = texture(uPositionTex, uv).xyz;
        float depth = length(worldPos - uCameraPos);
        float normalizedDepth = clamp(depth / 50.0, 0.0, 1.0);  // Normalize to 0-50 units
        imageStore(uOutputTex, pixelCoord, vec4(vec3(normalizedDepth), 1.0));
        return;
    }
    
    // Skip sky
    if (isSky) {
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
            
            // Sample neighbor normal (at full res for accuracy)
            vec2 fullResSampleUV = sampleUV;
            vec4 sampleNormalData = texture(uNormalTex, fullResSampleUV);
            vec3 sampleNormal = normalize(sampleNormalData.xyz);
            
            // Bilateral weights (normal-based only since depth texture has issues)
            float normalWeight = pow(max(0.0, dot(normal, sampleNormal)), 32.0);
            float spatialWeight = exp(-float(dx*dx + dy*dy) / float(uBlurRadius * uBlurRadius + 1));
            
            float weight = normalWeight * spatialWeight;
            
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
        printf("[SSGI] ERROR: Failed to compile raymarch shader!\n");
        SE_LOG_ERROR("[SSGI] Failed to compile raymarch shader");
        return false;
    }
    printf("[SSGI] Raymarch shader compiled successfully, valid=%d\n", raymarchShader_->IsValid());
    
    resolveShader_ = std::make_shared<ComputeShader>();
    if (!resolveShader_->LoadFromSource(kResolveShaderSource)) {
        printf("[SSGI] ERROR: Failed to compile resolve shader!\n");
        SE_LOG_ERROR("[SSGI] Failed to compile resolve shader");
        return false;
    }
    printf("[SSGI] Resolve shader compiled successfully, valid=%d\n", resolveShader_->IsValid());
    
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
    int newWorkW = static_cast<int>(width * config_.ResolutionScale);
    int newWorkH = static_cast<int>(height * config_.ResolutionScale);
    
    if (screenWidth_ == width && screenHeight_ == height && 
        workWidth_ == newWorkW && workHeight_ == newWorkH && finalRadianceTex_ != 0) return;
    
    screenWidth_ = width;
    screenHeight_ = height;
    workWidth_ = newWorkW;
    workHeight_ = newWorkH;
    
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
        printf("[SSGI] Created SH%d texture: ID=%u, size=%dx%d\n", i, shCoeffTex_[i], workWidth_, workHeight_);
    }
    
    // Final radiance texture at full resolution
    glGenTextures(1, &finalRadianceTex_);
    glBindTexture(GL_TEXTURE_2D, finalRadianceTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    printf("[SSGI] Created final radiance texture: ID=%u, size=%dx%d\n", finalRadianceTex_, screenWidth_, screenHeight_);
    
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
    
    // Explicitly bind samplers (some drivers ignore layout(binding=X) in compute shaders)
    raymarchShader_->SetInt("uPositionTex", 0);
    raymarchShader_->SetInt("uNormalTex", 1);
    raymarchShader_->SetInt("uAlbedoTex", 2);
    raymarchShader_->SetInt("uEmissiveTex", 3);
    raymarchShader_->SetInt("uDepthTex", 4);
    
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
    
    // Light uniforms for direct light contribution in GI
    raymarchShader_->SetVec3("uLightDirection", lightDirection_);
    raymarchShader_->SetVec3("uLightColor", lightColor_);
    raymarchShader_->SetFloat("uLightIntensity", lightIntensity_);

    
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
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, lastPositionTex_);
    
    // Explicitly bind samplers
    resolveShader_->SetInt("uSH0", 0);
    resolveShader_->SetInt("uSH1", 1);
    resolveShader_->SetInt("uSH2", 2);
    resolveShader_->SetInt("uSH3", 3);
    resolveShader_->SetInt("uNormalTex", 4);
    resolveShader_->SetInt("uDepthTex", 5);
    resolveShader_->SetInt("uPositionTex", 6);
    
    // Set uniforms
    resolveShader_->SetVec2("uOutputResolution", glm::vec2(screenWidth_, screenHeight_));
    resolveShader_->SetVec2("uWorkResolution", glm::vec2(workWidth_, workHeight_));
    resolveShader_->SetInt("uBlurRadius", config_.BlurRadius);
    resolveShader_->SetFloat("uDepthThreshold", config_.DepthThreshold);
    resolveShader_->SetFloat("uNormalThreshold", config_.NormalThreshold);
    resolveShader_->SetInt("uDebugMode", config_.DebugMode);
    resolveShader_->SetVec3("uCameraPos", cameraPos_);

    
    uint32_t groupsX = (screenWidth_ + 7) / 8;
    uint32_t groupsY = (screenHeight_ + 7) / 8;
    resolveShader_->DispatchAndWait(groupsX, groupsY, 1);
}

} // namespace se
