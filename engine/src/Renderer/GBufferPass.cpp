#include "engine/renderer/GBufferPass.h"
#include "engine/Log.h"

#include <glad/glad.h>

namespace se {

GBufferPass::GBufferPass() = default;

GBufferPass::~GBufferPass() {
    Shutdown();
}

void GBufferPass::Init(int width, int height) {
    if (initialized_) {
        SE_LOG_WARN("GBufferPass already initialized");
        return;
    }
    
    SE_LOG_INFO("Initializing GBufferPass ({}x{})", width, height);
    
    width_ = width;
    height_ = height;
    
    CreateResources();
    initialized_ = true;
}

void GBufferPass::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down GBufferPass");
    DestroyResources();
    initialized_ = false;
}

void GBufferPass::Resize(int width, int height) {
    if (width_ == width && height_ == height) return;
    if (width <= 0 || height <= 0) return;
    
    SE_LOG_INFO("Resizing GBufferPass to {}x{}", width, height);
    
    width_ = width;
    height_ = height;
    
    DestroyResources();
    CreateResources();
}

void GBufferPass::CreateResources() {
    SE_LOG_INFO("Creating GBuffer resources ({}x{})", width_, height_);
    
    // Create framebuffer
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    
    // Position texture (RGB16F - world space position)
    glGenTextures(1, &positionTex_);
    glBindTexture(GL_TEXTURE_2D, positionTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width_, height_, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTex_, 0);
    
    // Normal texture (RGB16F - world space normal)
    glGenTextures(1, &normalTex_);
    glBindTexture(GL_TEXTURE_2D, normalTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width_, height_, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTex_, 0);
    
    // Albedo texture (RGBA8 - albedo + alpha)
    glGenTextures(1, &albedoTex_);
    glBindTexture(GL_TEXTURE_2D, albedoTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedoTex_, 0);
    
    // Emissive texture (RGB16F - emissive color, can be HDR)
    glGenTextures(1, &emissiveTex_);
    glBindTexture(GL_TEXTURE_2D, emissiveTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width_, height_, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, emissiveTex_, 0);
    
    // Depth texture
    glGenTextures(1, &depthTex_);
    glBindTexture(GL_TEXTURE_2D, depthTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex_, 0);
    
    // Set draw buffers
    GLenum attachments[4] = { 
        GL_COLOR_ATTACHMENT0, 
        GL_COLOR_ATTACHMENT1, 
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3 
    };
    glDrawBuffers(4, attachments);
    
    // Check completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("GBuffer framebuffer incomplete, status: 0x{:X}", status);
    } else {
        SE_LOG_INFO("GBuffer framebuffer created successfully");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBufferPass::DestroyResources() {
    if (positionTex_) { glDeleteTextures(1, &positionTex_); positionTex_ = 0; }
    if (normalTex_) { glDeleteTextures(1, &normalTex_); normalTex_ = 0; }
    if (albedoTex_) { glDeleteTextures(1, &albedoTex_); albedoTex_ = 0; }
    if (emissiveTex_) { glDeleteTextures(1, &emissiveTex_); emissiveTex_ = 0; }
    if (depthTex_) { glDeleteTextures(1, &depthTex_); depthTex_ = 0; }
    if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
}

void GBufferPass::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    
    // Need to set draw buffers each time we bind (state may have been changed by other FBOs)
    GLenum attachments[4] = { 
        GL_COLOR_ATTACHMENT0, 
        GL_COLOR_ATTACHMENT1, 
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3 
    };
    glDrawBuffers(4, attachments);
}

void GBufferPass::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBufferPass::Clear() {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace se
