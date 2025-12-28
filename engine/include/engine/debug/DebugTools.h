#pragma once

#include "DebugConfig.h"

#if SE_ENABLE_DEBUG_TOOLS

#include "IDebugTool.h"
#include "FrameProfiler.h"

#include <memory>
#include <vector>
#include <string>

namespace se::debug {

/**
 * Central manager for all debug tools.
 * Provides a unified ImGui window with menu and tool panels.
 */
class DebugToolsManager {
public:
    static DebugToolsManager& Get();
    
    // Register a custom debug tool
    void RegisterTool(std::unique_ptr<IDebugTool> tool);
    
    // Frame lifecycle hooks
    void OnFrameStart();
    void OnFrameEnd();
    
    // Render the debug tools UI
    void OnImGuiRender();
    
    // Toggle visibility of the main debug window
    void SetVisible(bool visible) { showMainWindow_ = visible; }
    bool IsVisible() const { return showMainWindow_; }
    void ToggleVisibility() { showMainWindow_ = !showMainWindow_; }
    
private:
    DebugToolsManager();
    ~DebugToolsManager();
    
    void RenderMainWindow();
    void RenderMenuBar();
    void RenderToolsMenu();
    
    std::vector<std::unique_ptr<IDebugTool>> tools_;
    std::vector<bool> toolVisibility_;
    
    bool showMainWindow_ = true;
    bool showProfiler_ = true;
    bool showImGuiDemo_ = false;
    bool showStatistics_ = true;
    
    // Statistics
    float frameTimeMs_ = 0.0f;
    float fps_ = 0.0f;
    int frameCount_ = 0;
    float fpsUpdateTimer_ = 0.0f;
};

/**
 * Simple statistics display tool.
 */
class StatisticsTool : public IDebugTool {
public:
    const char* GetName() const override { return "Statistics"; }
    void OnImGuiRender() override;
    bool IsEnabledByDefault() const override { return true; }
    
    void SetFrameTime(float ms) { frameTimeMs_ = ms; }
    void SetFPS(float fps) { fps_ = fps; }
    void SetDrawCalls(int count) { drawCalls_ = count; }
    void SetBatches(int count) { batches_ = count; }
    void SetInstancedObjects(int count) { instancedObjects_ = count; }
    
private:
    float frameTimeMs_ = 0.0f;
    float fps_ = 0.0f;
    int drawCalls_ = 0;
    int batches_ = 0;
    int instancedObjects_ = 0;
};

} // namespace se::debug

// Convenience macros for use in Application
#define SE_DEBUG_TOOLS_FRAME_BEGIN() ::se::debug::DebugToolsManager::Get().OnFrameStart()
#define SE_DEBUG_TOOLS_FRAME_END()   ::se::debug::DebugToolsManager::Get().OnFrameEnd()
#define SE_DEBUG_TOOLS_RENDER()      ::se::debug::DebugToolsManager::Get().OnImGuiRender()
#define SE_DEBUG_TOOLS_TOGGLE()      ::se::debug::DebugToolsManager::Get().ToggleVisibility()

#else // SE_ENABLE_DEBUG_TOOLS

// Release build - all macros expand to nothing
#define SE_DEBUG_TOOLS_FRAME_BEGIN()
#define SE_DEBUG_TOOLS_FRAME_END()
#define SE_DEBUG_TOOLS_RENDER()
#define SE_DEBUG_TOOLS_TOGGLE()

#endif // SE_ENABLE_DEBUG_TOOLS
