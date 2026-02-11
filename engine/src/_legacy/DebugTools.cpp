#include "engine/debug/DebugTools.h"

#if SE_ENABLE_DEBUG_TOOLS

#include <imgui.h>
#include "engine/ecs/RenderSystem.h"
#include "engine/core/Time.h"

namespace se::debug {

// ============================================================================
// DebugToolsManager
// ============================================================================

DebugToolsManager& DebugToolsManager::Get() {
    static DebugToolsManager instance;
    return instance;
}

DebugToolsManager::DebugToolsManager() {
    // Register built-in tools
    auto stats = std::make_unique<StatisticsTool>();
    tools_.push_back(std::move(stats));
    toolVisibility_.push_back(true);
}

DebugToolsManager::~DebugToolsManager() = default;

void DebugToolsManager::RegisterTool(std::unique_ptr<IDebugTool> tool) {
    toolVisibility_.push_back(tool->IsEnabledByDefault());
    tools_.push_back(std::move(tool));
}

void DebugToolsManager::OnFrameStart() {
    FrameProfiler::Get().OnFrameStart();
    
    for (auto& tool : tools_) {
        tool->OnFrameStart();
    }
}

void DebugToolsManager::OnFrameEnd() {
    // Update statistics
    frameCount_++;
    fpsUpdateTimer_ += Time::DeltaTime();
    
    if (fpsUpdateTimer_ >= 0.5f) {
        fps_ = frameCount_ / fpsUpdateTimer_;
        frameTimeMs_ = 1000.0f / fps_;
        frameCount_ = 0;
        fpsUpdateTimer_ = 0.0f;
    }
    
    // Update statistics tool
    if (!tools_.empty()) {
        auto* stats = dynamic_cast<StatisticsTool*>(tools_[0].get());
        if (stats) {
            stats->SetFPS(fps_);
            stats->SetFrameTime(frameTimeMs_);
            stats->SetBatches(RenderSystem::GetLastBatchCount());
            stats->SetInstancedObjects(RenderSystem::GetLastInstancedObjects());
        }
    }
    
    FrameProfiler::Get().OnFrameEnd();
    
    for (auto& tool : tools_) {
        tool->OnFrameEnd();
    }
}

void DebugToolsManager::OnImGuiRender() {
    if (!showMainWindow_) return;
    
    RenderMainWindow();
    
    // Render ImGui demo if requested
    if (showImGuiDemo_) {
        ImGui::ShowDemoWindow(&showImGuiDemo_);
    }
}

void DebugToolsManager::RenderMainWindow() {
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar;
    
    ImGui::Begin("Debug Tools", &showMainWindow_, windowFlags);
    
    RenderMenuBar();
    
    // Render profiler if enabled
    if (showProfiler_) {
        if (ImGui::CollapsingHeader("Frame Profiler", ImGuiTreeNodeFlags_DefaultOpen)) {
            FrameProfiler::Get().OnImGuiRender();
        }
    }
    
    // Render registered tools
    for (size_t i = 0; i < tools_.size(); ++i) {
        if (i < toolVisibility_.size() && toolVisibility_[i]) {
            if (ImGui::CollapsingHeader(tools_[i]->GetName(), ImGuiTreeNodeFlags_DefaultOpen)) {
                tools_[i]->OnImGuiRender();
            }
        }
    }
    
    ImGui::End();
}

void DebugToolsManager::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Frame Profiler", nullptr, &showProfiler_);
            ImGui::Separator();
            
            for (size_t i = 0; i < tools_.size(); ++i) {
                if (i < toolVisibility_.size()) {
                    bool visible = toolVisibility_[i];
                    if (ImGui::MenuItem(tools_[i]->GetName(), nullptr, &visible)) {
                        toolVisibility_[i] = visible;
                    }
                }
            }
            
            ImGui::Separator();
            ImGui::MenuItem("ImGui Demo", nullptr, &showImGuiDemo_);
            
            ImGui::EndMenu();
        }
        
        RenderToolsMenu();
        
        // Quick stats in menu bar
        ImGui::Separator();
        ImGui::Text("%.1f FPS (%.2f ms)", fps_, frameTimeMs_);
        
        ImGui::EndMenuBar();
    }
}

void DebugToolsManager::RenderToolsMenu() {
    if (ImGui::BeginMenu("Tools")) {
        if (ImGui::MenuItem("Map Editor", "External")) {
            // Launch map editor - external application
        }
        if (ImGui::MenuItem("UI Editor", "External")) {
            // Launch UI editor - external application
        }
        ImGui::EndMenu();
    }
}

// ============================================================================
// StatisticsTool
// ============================================================================

void StatisticsTool::OnImGuiRender() {
    ImGui::Text("Performance");
    ImGui::Separator();
    
    ImGui::Columns(2, nullptr, false);
    
    ImGui::Text("FPS:");
    ImGui::NextColumn();
    ImGui::TextColored(
        fps_ >= 60.0f ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
        fps_ >= 30.0f ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                        ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
        "%.1f", fps_);
    ImGui::NextColumn();
    
    ImGui::Text("Frame Time:");
    ImGui::NextColumn();
    ImGui::Text("%.2f ms", frameTimeMs_);
    ImGui::NextColumn();
    
    ImGui::Columns(1);
    
    ImGui::Spacing();
    ImGui::Text("Rendering");
    ImGui::Separator();
    
    ImGui::Columns(2, nullptr, false);
    
    ImGui::Text("Batches:");
    ImGui::NextColumn();
    ImGui::Text("%d", batches_);
    ImGui::NextColumn();
    
    ImGui::Text("Instanced Objects:");
    ImGui::NextColumn();
    ImGui::Text("%d", instancedObjects_);
    ImGui::NextColumn();
    
    ImGui::Text("Draw Calls:");
    ImGui::NextColumn();
    ImGui::Text("%d", drawCalls_);
    ImGui::NextColumn();
    
    ImGui::Columns(1);
}

} // namespace se::debug

#endif // SE_ENABLE_DEBUG_TOOLS
