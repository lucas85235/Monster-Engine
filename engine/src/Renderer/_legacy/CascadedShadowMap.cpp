#include "engine/renderer/CascadedShadowMap.h"

#include <glad/glad.h>
#include <gtc/matrix_transform.hpp>

#include "engine/Camera.h"
#include "engine/Log.h"

namespace se {

CascadedShadowMap::~CascadedShadowMap() {
    Shutdown();
}

void CascadedShadowMap::Init(int resolution) {
    if (initialized_) return;
    
    resolution_ = resolution;
    
    // Create FBO
    glGenFramebuffers(1, &shadowFBO_);
    
    // Create texture array for cascades
    glGenTextures(1, &shadowTextureArray_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, shadowTextureArray_);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, 
                 resolution_, resolution_, CASCADE_COUNT, 
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach to FBO
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO_);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTextureArray_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("[CSM] Framebuffer incomplete");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    initialized_ = true;
    SE_LOG_INFO("[CSM] Initialized: {} cascades at {}x{}", CASCADE_COUNT, resolution_, resolution_);
}

void CascadedShadowMap::Shutdown() {
    if (shadowFBO_) {
        glDeleteFramebuffers(1, &shadowFBO_);
        shadowFBO_ = 0;
    }
    if (shadowTextureArray_) {
        glDeleteTextures(1, &shadowTextureArray_);
        shadowTextureArray_ = 0;
    }
    initialized_ = false;
}

void CascadedShadowMap::BeginShadowPass() {
    if (!initialized_) return;
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO_);
    glViewport(0, 0, resolution_, resolution_);
}

void CascadedShadowMap::BeginCascade(int cascadeIndex) {
    if (!initialized_ || cascadeIndex < 0 || cascadeIndex >= CASCADE_COUNT) return;
    glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowTextureArray_, 0, cascadeIndex);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CascadedShadowMap::EndShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMap::CalculateSplitDepths(float nearPlane, float farPlane) {
    // Practical split scheme (mix of uniform and logarithmic)
    for (int i = 0; i < CASCADE_COUNT; i++) {
        float p = static_cast<float>(i + 1) / static_cast<float>(CASCADE_COUNT);
        float logSplit = nearPlane * std::pow(farPlane / nearPlane, p);
        float uniformSplit = nearPlane + (farPlane - nearPlane) * p;
        cascadeData_[i].SplitDepth = splitLambda_ * logSplit + (1.0f - splitLambda_) * uniformSplit;
    }
}

glm::mat4 CascadedShadowMap::CalculateLightSpaceMatrix(const Camera& camera, const glm::vec3& lightDir, float aspectRatio, float nearSplit, float farSplit) {
    float fov = glm::radians(camera.GetZoom());
    
    // Calculate frustum corners in world space
    glm::mat4 proj = glm::perspective(fov, aspectRatio, nearSplit, farSplit);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);
    
    glm::vec3 frustumCorners[8] = {
        glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3( 1.0f,  1.0f,  1.0f)
    };
    
    glm::vec3 center(0.0f);
    for (int i = 0; i < 8; i++) {
        glm::vec4 corner = invVP * glm::vec4(frustumCorners[i], 1.0f);
        frustumCorners[i] = glm::vec3(corner) / corner.w;
        center += frustumCorners[i];
    }
    center /= 8.0f;
    
    // Calculate max radius to make the shadow map stable under rotation
    float radius = 0.0f;
    for (int i = 0; i < 8; i++) {
        float dist = glm::length(frustumCorners[i] - center);
        radius = glm::max(radius, dist);
    }
    radius = std::ceil(radius * 16.0f) / 16.0f; // Round up for stability
    
    // Create light view matrix
    glm::vec3 up = glm::abs(lightDir.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
    // Position light far away to catch tall casters
    glm::vec3 lightPos = center - lightDir * 200.0f; 
    glm::mat4 lightView = glm::lookAt(lightPos, center, up);
    
    // Create orthographic projection based on sphere radius
    // This makes the projection invariant to camera rotation
    glm::vec3 minBounds = glm::vec3(-radius, -radius, -400.0f);
    glm::vec3 maxBounds = glm::vec3(radius, radius, 400.0f);
    
    // Stabilization: Snap center to texel grid
    glm::mat4 lightProj = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, minBounds.z, maxBounds.z);
    glm::mat4 shadowMatrix = lightProj * lightView;
    
    glm::vec4 shadowOrigin = shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    shadowOrigin *= (float)resolution_ / 2.0f;
    
    glm::vec4 roundedOrigin = glm::round(shadowOrigin);
    glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
    roundOffset *= 2.0f / (float)resolution_;
    roundOffset.z = 0.0f;
    roundOffset.w = 0.0f;
    
    lightProj[3] += roundOffset;
    
    return lightProj * lightView;
}

void CascadedShadowMap::CalculateCascades(const Camera& camera, const glm::vec3& lightDir, float aspectRatio, float maxDistance) {
    float nearPlane = 0.1f;
    float farPlane = glm::min(maxDistance, 300.0f); // Increase max distance
    
    CalculateSplitDepths(nearPlane, farPlane);
    
    float prevSplit = nearPlane;
    for (int i = 0; i < CASCADE_COUNT; i++) {
        cascadeData_[i].ViewProjection = CalculateLightSpaceMatrix(camera, lightDir, aspectRatio, prevSplit, cascadeData_[i].SplitDepth);
        prevSplit = cascadeData_[i].SplitDepth;
    }
}

}  // namespace se
