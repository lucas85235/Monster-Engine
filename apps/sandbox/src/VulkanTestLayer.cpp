#include "VulkanTestLayer.h"

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
#include <engine/resources/ModelLoader.h>
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
    
    // Try to load the spaceship model
    if (useModel_) {
        SE_LOG_INFO("Loading spaceship model...");
        model_ = se::ModelLoader::Load("assets/models/car/Intergalactic_Spaceship-(Wavefront).obj");
        
        if (model_ && model_->GetMeshCount() > 0) {
            SE_LOG_INFO("Model loaded successfully with {} submeshes", model_->GetMeshCount());
        } else {
            SE_LOG_WARN("Failed to load model, falling back to cube");
            useModel_ = false;
        }
    }
    
    // Create a simple cube mesh as fallback
    if (!useModel_) {
        SE_LOG_INFO("Creating cube mesh...");
        cubeVAO_ = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
        
        if (!cubeVAO_) {
            SE_LOG_ERROR("Failed to create cube mesh!");
            return;
        }
        SE_LOG_INFO("Cube mesh created");
    }
    
    // Setup camera - position further back for model
    if (useModel_) {
        camera_.SetPosition(glm::vec3(0.0f, 2.0f, 15.0f));
    } else {
        camera_.SetPosition(glm::vec3(0.0f, 2.0f, 5.0f));
    }
    camera_.SetPitch(-15.0f);
    camera_.SetYaw(-90.0f);
    
    SE_LOG_INFO("VulkanTestLayer::OnAttach - DONE");
}

void VulkanTestLayer::OnDetach() {
    SE_LOG_INFO("VulkanTestLayer::OnDetach");
    cubeVAO_.reset();
    material_.reset();
    model_.reset();
}

void VulkanTestLayer::OnUpdate(float ts) {
    rotation_ += ts * 45.0f;  // 45 degrees per second
    
    auto& input = se::InputManager::Get();
    
    // Mouse camera rotation using InputManager axis
    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");
    camera_.SetYaw(camera_.GetYaw() + mouseX * 0.1f);
    camera_.SetPitch(camera_.GetPitch() - mouseY * 0.1f);
    
    // WASD camera movement
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
    
    // Arrow keys for camera rotation (alternative)
    if (se::Input::IsKeyPressed(se::Key::Left)) {
        camera_.SetYaw(camera_.GetYaw() - 60.0f * ts);
    }
    if (se::Input::IsKeyPressed(se::Key::Right)) {
        camera_.SetYaw(camera_.GetYaw() + 60.0f * ts);
    }
    if (se::Input::IsKeyPressed(se::Key::Up)) {
        camera_.SetPitch(camera_.GetPitch() + 60.0f * ts);
    }
    if (se::Input::IsKeyPressed(se::Key::Down)) {
        camera_.SetPitch(camera_.GetPitch() - 60.0f * ts);
    }
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
    
    // Calculate view/proj matrices
    glm::mat4 view = camera_.getViewMatrix();
    glm::mat4 proj = camera_.getProjectionMatrix(aspectRatio);
    
    if (useModel_ && model_) {
        // Render each submesh of the model
        for (size_t i = 0; i < model_->GetMeshCount(); ++i) {
            auto& submesh = model_->GetSubMesh(i);
            
            // Use the RHI VertexArray (works with both OpenGL and Vulkan)
            if (!submesh.vertexArray) {
                SE_LOG_WARN("Submesh {} has no vertexArray", i);
                continue;
            }
            
            auto& vao = submesh.vertexArray;
            auto& vertexBuffers = vao->GetVertexBuffers();
            if (vertexBuffers.empty()) continue;
            
            const auto& layout = vertexBuffers[0]->GetLayout();
            
            // Use submesh material or default material
            auto mat = submesh.material ? submesh.material : material_;
            if (!mat) continue;
            
            // Get or create pipeline
            RHI::PipelineHandle pipeline = mat->GetPipeline(layout);
            if (!RHI::IsValid(pipeline)) {
                SE_LOG_ERROR("Failed to create pipeline for submesh {}", i);
                continue;
            }
            
            // Bind the pipeline
            device->BindPipeline(pipeline);
            
            // Bind material (sets textures and uniforms)
            mat->Bind();
            
            // Bind submesh textures if available
            if (submesh.diffuseTexture) {
                device->BindTexture(0, submesh.diffuseTexture->GetHandle());
            }
            
            // Set uniforms
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotation_), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(1.0f));  // Adjust scale as needed
            
            mat->SetMatrix4("uView", view);
            mat->SetMatrix4("uProj", proj);
            mat->SetMatrix4("uModel", model);
            
            // Bind VAO and draw
            vao->Bind();
            
            auto indexBuffer = vao->GetIndexBuffer();
            if (indexBuffer) {
                RHI::DrawIndexedCommand cmd{};
                cmd.indexCount = indexBuffer->GetCount();
                cmd.instanceCount = 1;
                device->DrawIndexed(cmd);
            }
            
            mat->Unbind();
        }
    } else if (cubeVAO_ && material_) {
        // Fallback: render cube
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
}

void VulkanTestLayer::OnImGuiRender() {
    ImGui::Begin("Vulkan Test Layer");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Rotation: %.1f", rotation_);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", 
        camera_.GetPosition().x, camera_.GetPosition().y, camera_.GetPosition().z);
    
    if (useModel_ && model_) {
        ImGui::Text("Model: %zu submeshes", model_->GetMeshCount());
    } else {
        ImGui::Text("Rendering: Cube");
    }
    
    ImGui::Separator();
    ImGui::Text("Controls: WASD to move, Mouse to look");
    ImGui::Text("Press ESC to exit");
    
    ImGui::End();
}
