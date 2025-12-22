#include "VulkanTestLayer.h"

#include <engine/core/Application.h>
#include <engine/core/Log.h>
#include <engine/renderer/GraphicsContext.h>
#include <engine/renderer/MeshFactory.h>
#include <engine/renderer/Renderer.h>
#include <engine/renderer/Shader.h>
#include <engine/resources/MaterialManager.h>
#include <engine/resources/MeshManager.h>
#include <gtc/matrix_transform.hpp>

VulkanTestLayer::VulkanTestLayer() : Layer("VulkanTestLayer"), camera_(glm::vec3(0.0f, 3.0f, 8.0f)) {}

void VulkanTestLayer::OnAttach() {
    SE_LOG_INFO("VulkanTestLayer::OnAttach - START");
    
    // Get default material from MaterialManager
    SE_LOG_INFO("Getting default material...");
    material_ = se::MaterialManager::GetDefaultMaterial();
    
    if (!material_) {
        SE_LOG_ERROR("Failed to get default material!");
        return;
    }
    SE_LOG_INFO("Material obtained");
    
    // Create a simple cube mesh
    SE_LOG_INFO("Creating cube mesh...");
    cubeVAO_ = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
    
    if (!cubeVAO_) {
        SE_LOG_ERROR("Failed to create cube mesh!");
        return;
    }
    SE_LOG_INFO("Cube mesh created");
    
    // Setup camera
    camera_.SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));
    camera_.SetPitch(-15.0f);
    camera_.SetYaw(-90.0f);
    
    SE_LOG_INFO("VulkanTestLayer::OnAttach - DONE");
}

void VulkanTestLayer::OnDetach() {
    SE_LOG_INFO("VulkanTestLayer::OnDetach");
    cubeVAO_.reset();
    material_.reset();
}

void VulkanTestLayer::OnUpdate(float ts) {
    rotation_ += ts * 45.0f;  // 45 degrees per second
}

void VulkanTestLayer::OnRender() {
    SE_LOG_DEBUG("VulkanTestLayer::OnRender");
    
    auto& app = se::Application::Get();
    auto& window = app.GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    
    // Get the renderer and device
    auto* context = window.GetContext();
    if (!context) return;
    
    auto* device = context->GetDevice();
    if (!device) return;
    
    if (!cubeVAO_ || !material_) return;
    
    // Get vertex layout from the cube VAO
    auto& vertexBuffers = cubeVAO_->GetVertexBuffers();
    if (vertexBuffers.empty()) {
        SE_LOG_ERROR("No vertex buffers in cube VAO");
        return;
    }
    
    const auto& layout = vertexBuffers[0]->GetLayout();
    
    // Get or create pipeline
    RHI::PipelineHandle pipeline = material_->GetPipeline(layout);
    if (!RHI::IsValid(pipeline)) {
        SE_LOG_ERROR("Failed to create pipeline for material");
        return;
    }
    
    // Bind the pipeline
    device->BindPipeline(pipeline);
    
    // Bind material (sets textures and uniforms)
    SE_LOG_DEBUG("Binding material...");
    material_->Bind();
    
    // Set uniforms
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 proj = camera_.getProjectionMatrix(aspectRatio);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
    model = glm::rotate(model, glm::radians(rotation_), glm::vec3(0.0f, 1.0f, 0.0f));
    
    material_->SetMatrix4("uView", view);
    material_->SetMatrix4("uProj", proj);
    material_->SetMatrix4("uModel", model);
    
    // Bind VAO and draw
    SE_LOG_DEBUG("Binding VAO and drawing...");
    cubeVAO_->Bind();
    
    RHI::DrawIndexedCommand cmd{};
    cmd.indexCount = cubeVAO_->GetIndexBuffer()->GetCount();
    cmd.instanceCount = 1;
    
    device->DrawIndexed(cmd);
    
    material_->Unbind();
}
