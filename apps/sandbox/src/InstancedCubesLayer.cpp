#include "InstancedCubesLayer.h"

#include <imgui.h>
#include <engine/core/Application.h>
#include <engine/core/Log.h>
#include <engine/input/Input.h>
#include <engine/input/InputManager.h>
#include <engine/renderer/GraphicsContext.h>
#include <engine/renderer/MeshFactory.h>
#include <engine/renderer/Renderer.h>
#include <engine/renderer/Shader.h>
#include <engine/resources/MaterialManager.h>
#include <engine/resources/MeshManager.h>
#include <engine/rhi/vulkan/vulkan_device.h>
#include <gtc/matrix_transform.hpp>

InstancedCubesLayer::InstancedCubesLayer() : Layer("InstancedCubesLayer"), camera_(glm::vec3(0.0f, 8.0f, 25.0f)) {}

void InstancedCubesLayer::OnAttach() {
    SE_LOG_INFO("InstancedCubesLayer::OnAttach - START");
    
    // Load the instanced shader compatible with VulkanDevice UBO/push constants
    SE_LOG_INFO("[InstancedCubesLayer] Loading instanced shader...");
    auto instancedShader = se::Shader::CreateFromFiles(
        "assets/shaders/test_vulkan_instanced.vert",
        "assets/shaders/test_vulkan_instanced.frag"
    );
    
    if (!instancedShader || !RHI::IsValid(instancedShader->GetHandle())) {
        SE_LOG_ERROR("[InstancedCubesLayer] Failed to load instanced shader! Falling back to default material.");
        material_ = se::MaterialManager::GetDefaultMaterial();
    } else {
        SE_LOG_INFO("[InstancedCubesLayer] Instanced shader loaded, creating material...");
        material_ = std::make_shared<se::Material>(instancedShader);
    }
    
    if (!material_) {
        SE_LOG_ERROR("[InstancedCubesLayer] Failed to create material!");
        return;
    }
    SE_LOG_INFO("[InstancedCubesLayer] Material created successfully");
    
    SE_LOG_INFO("[InstancedCubesLayer] Creating cube mesh...");
    cubeVAO_ = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
    
    if (!cubeVAO_) {
        SE_LOG_ERROR("[InstancedCubesLayer] Failed to create cube mesh!");
        return;
    }
    SE_LOG_INFO("[InstancedCubesLayer] Cube mesh created successfully");
    
    SE_LOG_INFO("[InstancedCubesLayer] Creating InstancedMesh with {} instances...", instanceCount_);
    instancedMesh_ = std::make_shared<se::InstancedMesh>(cubeVAO_, instanceCount_);
    
    if (!instancedMesh_) {
        SE_LOG_ERROR("[InstancedCubesLayer] Failed to create InstancedMesh!");
        return;
    }
    SE_LOG_INFO("[InstancedCubesLayer] InstancedMesh created successfully");
    
    std::vector<se::InstanceData> instances(instanceCount_);
    
    SE_LOG_INFO("[InstancedCubesLayer] Setting up {} cube instances scattered in 3D space...", instanceCount_);
    for (uint32_t i = 0; i < instanceCount_; ++i) {
        // Scatter cubes in different positions
        float angle = static_cast<float>(i) * (360.0f / static_cast<float>(instanceCount_));
        float radius = 5.0f + static_cast<float>(i % 3) * 2.0f;
        float x = radius * std::cos(glm::radians(angle));
        float z = radius * std::sin(glm::radians(angle));
        float y = 0.5f + static_cast<float>(i % 4) * 1.5f;
        
        instances[i].Transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
        
        // Each cube gets a unique color
        float hue = static_cast<float>(i) / static_cast<float>(instanceCount_);
        float r = std::abs(std::sin(hue * 3.14159f * 2.0f));
        float g = std::abs(std::sin((hue + 0.33f) * 3.14159f * 2.0f));
        float b = std::abs(std::sin((hue + 0.66f) * 3.14159f * 2.0f));
        instances[i].Color = glm::vec4(r, g, b, 1.0f);
        
        SE_LOG_DEBUG("[InstancedCubesLayer] Instance {} at ({:.2f}, {:.2f}, {:.2f}) with color ({:.2f}, {:.2f}, {:.2f})", 
            i, x, y, z, r, g, b);
    }
    
    instancedMesh_->SetInstances(instances);
    SE_LOG_INFO("[InstancedCubesLayer] Instance data uploaded to GPU");
    
    camera_.SetPosition(glm::vec3(0.0f, 8.0f, 25.0f));
    camera_.SetPitch(-15.0f);
    camera_.SetYaw(-90.0f);
    SE_LOG_INFO("[InstancedCubesLayer] Camera configured");
    
    SE_LOG_INFO("InstancedCubesLayer::OnAttach - DONE (total instances: {})", instanceCount_);
}

void InstancedCubesLayer::OnDetach() {
    SE_LOG_INFO("InstancedCubesLayer::OnDetach");
    instancedMesh_.reset();
    cubeVAO_.reset();
    material_.reset();
}

void InstancedCubesLayer::OnUpdate(float ts) {
    rotation_ += ts * 30.0f;
    
    auto& input = se::InputManager::Get();
    
    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");
    camera_.SetYaw(camera_.GetYaw() + mouseX * 0.1f);
    camera_.SetPitch(camera_.GetPitch() - mouseY * 0.1f);
    
    if (se::Input::IsKeyPressed(se::Key::W)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::FORWARD, ts);
    }
    if (se::Input::IsKeyPressed(se::Key::S)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::BACKWARD, ts);
    }
    if (se::Input::IsKeyPressed(se::Key::A)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::LEFT, ts);
    }
    if (se::Input::IsKeyPressed(se::Key::D)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::RIGHT, ts);
    }
    if (se::Input::IsKeyPressed(se::Key::Space)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::UP, ts);
    }
    if (se::Input::IsKeyPressed(se::Key::LeftShift)) {
        camera_.ProcessKeyboard(Camera::CameraMovement::DOWN, ts);
    }
    
    // Update cube transforms with animation
    if (instancedMesh_) {
        std::vector<se::InstanceData> instances(instanceCount_);
        for (uint32_t i = 0; i < instanceCount_; ++i) {
            float angle = static_cast<float>(i) * (360.0f / static_cast<float>(instanceCount_));
            float radius = 5.0f + static_cast<float>(i % 3) * 2.0f;
            float x = radius * std::cos(glm::radians(angle + rotation_ * 0.5f));
            float z = radius * std::sin(glm::radians(angle + rotation_ * 0.5f));
            float y = 0.5f + static_cast<float>(i % 4) * 1.5f + 0.5f * std::sin(rotation_ * 0.05f + static_cast<float>(i));
            
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
            transform = glm::rotate(transform, glm::radians(rotation_ + static_cast<float>(i) * 36.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            
            instances[i].Transform = transform;
            
            float hue = static_cast<float>(i) / static_cast<float>(instanceCount_);
            float r = std::abs(std::sin(hue * 3.14159f * 2.0f));
            float g = std::abs(std::sin((hue + 0.33f) * 3.14159f * 2.0f));
            float b = std::abs(std::sin((hue + 0.66f) * 3.14159f * 2.0f));
            instances[i].Color = glm::vec4(r, g, b, 1.0f);
        }
        instancedMesh_->SetInstances(instances);
    }
}

void InstancedCubesLayer::OnRender() {
    SE_LOG_DEBUG("InstancedCubesLayer::OnRender");
    
    if (!instancedMesh_ || !material_) {
        SE_LOG_WARN("[InstancedCubesLayer::OnRender] Missing instancedMesh or material");
        return;
    }
    
    auto& app = se::Application::Get();
    auto& window = app.GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    
    auto* context = window.GetContext();
    if (!context) {
        SE_LOG_ERROR("[InstancedCubesLayer::OnRender] No graphics context");
        return;
    }
    
    auto* device = context->GetDevice();
    if (!device) {
        SE_LOG_ERROR("[InstancedCubesLayer::OnRender] No device");
        return;
    }
    
    // Store draw call count before rendering
    if (auto* vulkanDevice = dynamic_cast<RHI::VulkanDevice*>(device)) {
        drawCallCountBeforeRender_ = vulkanDevice->GetDrawCallCount();
    }
    
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 proj = camera_.getProjectionMatrix(aspectRatio);
    
    auto& vertexBuffers = instancedMesh_->GetVertexArray()->GetVertexBuffers();
    if (vertexBuffers.empty()) {
        SE_LOG_ERROR("[InstancedCubesLayer::OnRender] No vertex buffers in instanced mesh");
        return;
    }
    
    const auto& layout = vertexBuffers[0]->GetLayout();
    
    RHI::PipelineHandle pipeline = material_->GetPipeline(layout);
    if (!RHI::IsValid(pipeline)) {
        SE_LOG_ERROR("[InstancedCubesLayer::OnRender] Failed to create/get pipeline");
        return;
    }
    
    SE_LOG_DEBUG("[InstancedCubesLayer::OnRender] Binding pipeline...");
    device->BindPipeline(pipeline);
    
    SE_LOG_DEBUG("[InstancedCubesLayer::OnRender] Binding material...");
    material_->Bind();
    
    glm::mat4 model = glm::mat4(1.0f);
    material_->SetMatrix4("uView", view);
    material_->SetMatrix4("uProj", proj);
    material_->SetMatrix4("uModel", model);
    
    SE_LOG_DEBUG("[InstancedCubesLayer::OnRender] Drawing instanced mesh with {} instances...", instancedMesh_->GetInstanceCount());
    instancedMesh_->DrawWithoutMaterial();
    
    // Capture draw call count after rendering
    if (auto* vulkanDevice = dynamic_cast<RHI::VulkanDevice*>(device)) {
        lastDrawCallCount_ = vulkanDevice->GetDrawCallCount() - drawCallCountBeforeRender_;
    }
    
    material_->Unbind();
    SE_LOG_DEBUG("[InstancedCubesLayer::OnRender] Done");
}

void InstancedCubesLayer::OnImGuiRender() {
    ImGui::Begin("Instanced Cubes Test - Draw Call Verification");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "INSTANCING PERFORMANCE:");
    ImGui::Text("Instance Count: %u cubes", instanceCount_);
    
    // This is the critical metric - should be 1 if instancing works
    if (lastDrawCallCount_ == 1) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Draw Calls: %u (INSTANCING WORKING!)", lastDrawCallCount_);
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Draw Calls: %u (instancing NOT working)", lastDrawCallCount_);
    }
    
    auto& app = se::Application::Get();
    if (auto* context = app.GetWindow().GetContext()) {
        if (auto* device = context->GetDevice()) {
            if (auto* vulkanDevice = dynamic_cast<RHI::VulkanDevice*>(device)) {
                ImGui::Text("Total Frame Draw Calls: %u", vulkanDevice->GetDrawCallCount());
            }
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Rotation: %.1f", rotation_);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", 
        camera_.GetPosition().x, camera_.GetPosition().y, camera_.GetPosition().z);
    
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "If Draw Calls = 1, instancing is working correctly!");
    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "All %u cubes rendered in a single draw call.", instanceCount_);
    
    ImGui::Separator();
    ImGui::Text("Controls: WASD to move, Mouse to look");
    ImGui::Text("Press ESC to exit");
    
    ImGui::End();
}
