#include "PreviewPanel.h"
#include "../core/EditorContext.h"
#include "../core/GraphEvaluator.h"

#include <imgui.h>
#include <glad/glad.h>
#include <cmath>
#include <filesystem>
#include <algorithm>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>
#include <engine/animation/SkinnedModelManager.h>
#include <engine/animation/SkinnedModel.h>
#include <engine/Shader.h>
#include <engine/animation/AnimationManager.h>
#include <engine/animation/AnimationClip.h>
#include <engine/animation/advanced/Pose.h>
#include <engine/resources/ModelData.h>
#include <engine/Log.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PreviewPanel::PreviewPanel(EditorContext& context) : context_(context) {
    InitFramebuffer();
    
    evaluator_ = std::make_unique<GraphEvaluator>(context);
    currentPose_ = std::make_unique<se::anim::Pose>();
    
    try {
        modelShader_ = se::Shader::CreateFromFiles(
            "assets/shaders/preview/model.vert",
            "assets/shaders/preview/model.frag"
        );
    } catch (...) {
        modelShader_ = nullptr;
    }
}

PreviewPanel::~PreviewPanel() {
    DestroyFramebuffer();
}

void PreviewPanel::InitFramebuffer() {
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    
    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, viewportWidth_, viewportHeight_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture_, 0);
    
    glGenRenderbuffers(1, &depthRenderbuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, viewportWidth_, viewportHeight_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer_);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PreviewPanel::DestroyFramebuffer() {
    if (framebuffer_) {
        glDeleteFramebuffers(1, &framebuffer_);
        framebuffer_ = 0;
    }
    if (colorTexture_) {
        glDeleteTextures(1, &colorTexture_);
        colorTexture_ = 0;
    }
    if (depthRenderbuffer_) {
        glDeleteRenderbuffers(1, &depthRenderbuffer_);
        depthRenderbuffer_ = 0;
    }
}

void PreviewPanel::ResizeFramebuffer(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == viewportWidth_ && height == viewportHeight_) return;
    
    viewportWidth_ = width;
    viewportHeight_ = height;
    
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

void PreviewPanel::Render() {
    ImGui::Begin("Preview");

    RenderPlaybackControls();
    ImGui::Separator();
    
    float parametersHeight = showParameters_ ? 120.0f : 0.0f;
    
    ImVec2 available = ImGui::GetContentRegionAvail();
    float vpHeight = available.y - 40.0f - parametersHeight;
    if (vpHeight < 100.0f) vpHeight = 100.0f;
    
    int newWidth = static_cast<int>(available.x);
    int newHeight = static_cast<int>(vpHeight);
    ResizeFramebuffer(newWidth, newHeight);
    
    RenderToFramebuffer();
    
    ImVec2 imagePos = ImGui::GetCursorScreenPos();
    ImGui::Image((void*)(intptr_t)colorTexture_, 
                 ImVec2(static_cast<float>(viewportWidth_), static_cast<float>(viewportHeight_)),
                 ImVec2(0, 1), ImVec2(1, 0));
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    if (showGrid_) {
        RenderGrid(drawList, imagePos);
    }
    
    if (showSkeleton_) {
        RenderSkeletonOverlay(drawList, imagePos);
    }
    
    viewportHovered_ = ImGui::IsItemHovered();
    if (viewportHovered_) {
        HandleCameraInput();
    }
    
    ImGui::Separator();
    RenderTimeline();
    
    if (showParameters_) {
        ImGui::Separator();
        RenderParametersPanel();
    }

    ImGui::End();
}

void PreviewPanel::RenderParametersPanel() {
    if (ImGui::CollapsingHeader("Graph Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Float Parameters:");
        
        static char newFloatName[64] = "";
        ImGui::InputText("##NewFloat", newFloatName, sizeof(newFloatName));
        ImGui::SameLine();
        if (ImGui::Button("Add Float")) {
            if (newFloatName[0] != '\0') {
                floatParams_[newFloatName] = 0.0f;
                newFloatName[0] = '\0';
            }
        }
        
        for (auto& [name, value] : floatParams_) {
            ImGui::PushID(name.c_str());
            ImGui::SetNextItemWidth(100.0f);
            if (ImGui::DragFloat(name.c_str(), &value, 0.01f, -10.0f, 10.0f)) {
                if (evaluator_) {
                    evaluator_->SetFloat(name, value);
                }
            }
            ImGui::PopID();
        }
        
        ImGui::Separator();
        ImGui::Text("Bool Parameters (Triggers):");
        
        static char newBoolName[64] = "";
        ImGui::InputText("##NewBool", newBoolName, sizeof(newBoolName));
        ImGui::SameLine();
        if (ImGui::Button("Add Bool")) {
            if (newBoolName[0] != '\0') {
                boolParams_[newBoolName] = false;
                newBoolName[0] = '\0';
            }
        }
        
        for (auto& [name, value] : boolParams_) {
            ImGui::PushID(name.c_str());
            if (ImGui::Checkbox(name.c_str(), &value)) {
                if (evaluator_) {
                    evaluator_->SetBool(name, value);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Trigger")) {
                if (evaluator_) {
                    evaluator_->SetTrigger(name);
                }
            }
            ImGui::PopID();
        }
    }
}

void PreviewPanel::RenderViewport() {
}

void PreviewPanel::RenderToFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, viewportWidth_, viewportHeight_);
    
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_DEPTH_TEST);
    
    UpdateAnimation();
    Render3DModel();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PreviewPanel::Render3DModel() {
    const std::string& selectedPath = context_.GetSelectedModel();
    
    if (selectedPath.empty()) {
        currentModel_ = nullptr;
        loadedModelPath_.clear();
        return;
    }
    
    if (selectedPath != loadedModelPath_) {
        currentModel_ = se::SkinnedModelManager::Load(selectedPath);
        loadedModelPath_ = selectedPath;
        
        if (currentModel_ && currentModel_->IsValid()) {
            const auto& bounds = currentModel_->GetBoundingBox();
            float modelSize = glm::length(bounds.Max - bounds.Min);
            cameraDistance_ = modelSize * 2.0f;
            cameraTargetY_ = (bounds.Min.y + bounds.Max.y) * 0.5f;
            
            if (cameraDistance_ < 1.0f) cameraDistance_ = 5.0f;
            if (cameraDistance_ > 500.0f) cameraDistance_ = 500.0f;
            
            if (currentModel_->HasSkeleton()) {
                auto modelData = currentModel_->GetModelData();
                currentPose_ = std::make_unique<se::anim::Pose>(modelData->Bones.size());
                evaluator_->MarkNeedsRebuild();
            }
        }
    }
    
    if (!currentModel_ || !currentModel_->IsValid()) {
        return;
    }
    
    float yawRad = cameraYaw_ * static_cast<float>(M_PI) / 180.0f;
    float pitchRad = cameraPitch_ * static_cast<float>(M_PI) / 180.0f;
    
    float camX = cameraDistance_ * cosf(pitchRad) * sinf(yawRad);
    float camY = cameraDistance_ * sinf(pitchRad) + cameraTargetY_;
    float camZ = cameraDistance_ * cosf(pitchRad) * cosf(yawRad);
    
    glm::vec3 cameraPos(camX, camY, camZ);
    glm::vec3 target(0.0f, cameraTargetY_, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(cameraPos, target, up);
    float aspect = static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_);
    float farPlane = cameraDistance_ * 10.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, farPlane);
    glm::mat4 model = glm::mat4(1.0f);
    
    if (modelShader_) {
        modelShader_->bind();
        modelShader_->setMat4("view", view);
        modelShader_->setMat4("projection", projection);
        modelShader_->setMat4("model", model);
        modelShader_->setVec3("lightDir", glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
        modelShader_->setVec3("lightColor", glm::vec3(1.0f));
        modelShader_->setVec3("viewPos", cameraPos);
        
        if (!boneMatrices_.empty()) {
            modelShader_->setInt("hasBones", 1);
            for (size_t i = 0; i < boneMatrices_.size() && i < 128; ++i) {
                std::string name = "boneMatrices[" + std::to_string(i) + "]";
                modelShader_->setMat4(name.c_str(), boneMatrices_[i]);
            }
        } else {
            modelShader_->setInt("hasBones", 0);
        }
        
        currentModel_->Draw();
    }
}

void PreviewPanel::RenderGrid(ImDrawList* drawList, ImVec2 imagePos) {
    float centerX = imagePos.x + viewportWidth_ / 2.0f;
    float bottomY = imagePos.y + viewportHeight_ * 0.85f;
    float gridWidth = viewportWidth_ * 0.8f;
    
    ImU32 gridColor = IM_COL32(50, 50, 60, 150);
    int numLines = 11;
    float step = gridWidth / (numLines - 1);
    
    float startX = centerX - gridWidth / 2.0f;
    float endX = centerX + gridWidth / 2.0f;
    
    for (int i = 0; i < numLines; ++i) {
        float x = startX + i * step;
        float perspScale = 1.0f - (float)i / numLines * 0.3f;
        drawList->AddLine(ImVec2(x, bottomY), ImVec2(centerX + (x - centerX) * 0.5f, bottomY - 80), gridColor, 1.0f);
    }
    
    for (int i = 0; i < 5; ++i) {
        float y = bottomY - i * 20;
        float scale = 1.0f - i * 0.1f;
        drawList->AddLine(ImVec2(startX + (1 - scale) * gridWidth / 2, y), 
                         ImVec2(endX - (1 - scale) * gridWidth / 2, y), gridColor, 1.0f);
    }
}

void PreviewPanel::RenderSkeletonOverlay(ImDrawList* drawList, ImVec2 imagePos) {
    if (!currentModel_ || !currentModel_->HasSkeleton() || globalBoneTransforms_.empty()) {
        return;
    }
    
    auto modelData = currentModel_->GetModelData();
    if (!modelData) {
        return;
    }
    
    float pitchRad = cameraPitch_ * static_cast<float>(M_PI) / 180.0f;
    float yawRad = cameraYaw_ * static_cast<float>(M_PI) / 180.0f;
    
    float camX = cameraDistance_ * cosf(pitchRad) * sinf(yawRad);
    float camY = cameraDistance_ * sinf(pitchRad) + cameraTargetY_;
    float camZ = cameraDistance_ * cosf(pitchRad) * cosf(yawRad);
    
    glm::vec3 cameraPos(camX, camY, camZ);
    glm::vec3 target(0.0f, cameraTargetY_, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(cameraPos, target, up);
    float aspect = static_cast<float>(viewportWidth_) / static_cast<float>(viewportHeight_);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, cameraDistance_ * 10.0f);
    
    glm::mat4 viewProj = projection * view;
    
    auto worldToScreen = [&](const glm::vec3& worldPos) -> ImVec2 {
        glm::vec4 clipPos = viewProj * glm::vec4(worldPos, 1.0f);
        if (clipPos.w <= 0.0f) {
            return ImVec2(-1000, -1000);
        }
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        float screenX = (ndc.x * 0.5f + 0.5f) * viewportWidth_;
        float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportHeight_;
        return ImVec2(imagePos.x + screenX, imagePos.y + screenY);
    };
    
    ImU32 boneColor = IM_COL32(100, 200, 100, 255);
    ImU32 jointColor = IM_COL32(255, 200, 100, 255);
    float boneThickness = 2.0f;
    float jointRadius = 4.0f;
    
    for (size_t i = 0; i < modelData->Bones.size(); ++i) {
        const auto& bone = modelData->Bones[i];
        
        glm::vec3 bonePos = glm::vec3(globalBoneTransforms_[i][3]);
        ImVec2 screenPos = worldToScreen(bonePos);
        
        if (bone.ParentIndex >= 0 && bone.ParentIndex < static_cast<int>(modelData->Bones.size())) {
            glm::vec3 parentPos = glm::vec3(globalBoneTransforms_[bone.ParentIndex][3]);
            ImVec2 parentScreenPos = worldToScreen(parentPos);
            
            drawList->AddLine(parentScreenPos, screenPos, boneColor, boneThickness);
        }
        
        drawList->AddCircleFilled(screenPos, jointRadius, jointColor);
    }
}

void PreviewPanel::HandleCameraInput() {
    ImGuiIO& io = ImGui::GetIO();
    
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        cameraYaw_ += io.MouseDelta.x * 0.5f;
        cameraPitch_ -= io.MouseDelta.y * 0.5f;
        if (cameraPitch_ > 89.0f) cameraPitch_ = 89.0f;
        if (cameraPitch_ < -89.0f) cameraPitch_ = -89.0f;
    }
    
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        cameraTargetY_ -= io.MouseDelta.y * 0.01f;
    }
    
    if (io.MouseWheel != 0.0f) {
        float zoomSpeed = cameraDistance_ * 0.1f;
        cameraDistance_ -= io.MouseWheel * zoomSpeed;
        if (cameraDistance_ < 0.5f) cameraDistance_ = 0.5f;
        if (cameraDistance_ > 500.0f) cameraDistance_ = 500.0f;
    }
}

void PreviewPanel::RenderTimeline() {
    float time = context_.GetPreviewTime();
    float maxTime = 2.0f;
    
    if (currentAnimation_) {
        maxTime = currentAnimation_->GetDurationInSeconds();
        if (maxTime < 0.1f) maxTime = 0.1f;
    }

    ImGui::PushItemWidth(-1);
    if (ImGui::SliderFloat("##Timeline", &time, 0.0f, maxTime, "%.2f s")) {
        context_.SetPreviewTime(time);
        context_.SetPlaying(false);
    }
    ImGui::PopItemWidth();
}

void PreviewPanel::RenderPlaybackControls() {
    bool isPlaying = context_.IsPlaying();
    float speed = context_.GetPlaybackSpeed();

    if (ImGui::Button(isPlaying ? "||" : ">", ImVec2(30, 0))) {
        context_.SetPlaying(!isPlaying);
    }
    ImGui::SameLine();

    if (ImGui::Button("|<", ImVec2(30, 0))) {
        context_.SetPreviewTime(0.0f);
    }
    ImGui::SameLine();

    ImGui::PushItemWidth(60.0f);
    if (ImGui::DragFloat("##Speed", &speed, 0.01f, 0.1f, 3.0f, "%.2f")) {
        context_.SetPlaybackSpeed(speed);
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::Text("Speed");
    
    ImGui::SameLine();
    ImGui::Checkbox("Skeleton", &showSkeleton_);
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &showGrid_);
    ImGui::SameLine();
    ImGui::Checkbox("Params", &showParameters_);
    
    const std::string& selectedModel = context_.GetSelectedModel();
    std::string modelDisplay = selectedModel.empty() ? "Model..." : selectedModel;
    size_t lastSlash = modelDisplay.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        modelDisplay = modelDisplay.substr(lastSlash + 1);
    }
    
    ImGui::SameLine();
    ImGui::PushItemWidth(120.0f);
    if (ImGui::BeginCombo("##Model", modelDisplay.c_str())) {
        if (ImGui::Selectable("None##model", selectedModel.empty())) {
            context_.SetSelectedModel("");
        }
        
        static std::vector<std::string> foundModels;
        static bool scanned = false;
        if (!scanned) {
            scanned = true;
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator("assets")) {
                    if (!entry.is_regular_file()) continue;
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".fbx" || ext == ".gltf" || ext == ".glb") {
                        foundModels.push_back(entry.path().string());
                    }
                }
            } catch (...) {}
        }
        
        for (size_t i = 0; i < foundModels.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            std::string display = foundModels[i];
            size_t slash = display.find_last_of("/\\");
            if (slash != std::string::npos) display = display.substr(slash + 1);
            
            if (ImGui::Selectable(display.c_str(), foundModels[i] == selectedModel)) {
                context_.SetSelectedModel(foundModels[i]);
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

void PreviewPanel::LoadModel(const std::string& path) {
}

void PreviewPanel::LoadAnimation(const std::string& path) {
    if (path.empty()) {
        currentAnimation_ = nullptr;
        loadedAnimationPath_.clear();
        return;
    }
    
    if (path != loadedAnimationPath_) {
        try {
            currentAnimation_ = se::AnimationManager::Load(path);
            loadedAnimationPath_ = path;
        } catch (...) {
            currentAnimation_ = nullptr;
        }
    }
}

void PreviewPanel::UpdateAnimation() {
    if (!currentModel_ || !currentModel_->HasSkeleton() || !currentPose_ || !evaluator_) {
        globalBoneTransforms_.clear();
        boneMatrices_.clear();
        return;
    }
    
    auto modelData = currentModel_->GetModelData();
    if (!modelData) {
        return;
    }
    
    float currentTime = static_cast<float>(ImGui::GetTime());
    float deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;
    
    if (context_.IsPlaying()) {
        evaluator_->Update(deltaTime * context_.GetPlaybackSpeed());
        
        float newTime = context_.GetPreviewTime() + deltaTime * context_.GetPlaybackSpeed();
        context_.SetPreviewTime(newTime);
    }
    
    if (context_.IsGraphDirty()) {
        evaluator_->MarkNeedsRebuild();
        context_.ClearGraphDirty();
    }
    
    evaluator_->Evaluate(*currentPose_, modelData.get());
    
    ComputeBoneMatrices();
}

void PreviewPanel::ComputeBoneMatrices() {
    if (!currentModel_ || !currentModel_->HasSkeleton() || !currentPose_) {
        globalBoneTransforms_.clear();
        boneMatrices_.clear();
        return;
    }
    
    auto modelData = currentModel_->GetModelData();
    if (!modelData) {
        return;
    }
    
    size_t boneCount = modelData->Bones.size();
    globalBoneTransforms_.resize(boneCount);
    boneMatrices_.resize(boneCount);
    
    for (size_t i = 0; i < boneCount; ++i) {
        const auto& bone = modelData->Bones[i];
        const auto& transform = (*currentPose_)[i];
        
        glm::mat4 localMatrix = glm::translate(glm::mat4(1.0f), transform.position);
        localMatrix *= glm::mat4_cast(transform.rotation);
        localMatrix = glm::scale(localMatrix, transform.scale);
        
        if (bone.ParentIndex >= 0 && bone.ParentIndex < static_cast<int>(boneCount)) {
            globalBoneTransforms_[i] = globalBoneTransforms_[bone.ParentIndex] * localMatrix;
        } else {
            globalBoneTransforms_[i] = localMatrix;
        }
        
        boneMatrices_[i] = globalBoneTransforms_[i] * bone.OffsetMatrix;
    }
}
