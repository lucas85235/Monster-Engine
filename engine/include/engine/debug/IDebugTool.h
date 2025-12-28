#pragma once

#include "DebugConfig.h"

#if SE_ENABLE_DEBUG_TOOLS

namespace se::debug {

/**
 * Interface for debug tools that can be registered with DebugToolsManager.
 * Each tool provides a name and an ImGui render callback.
 */
class IDebugTool {
public:
    virtual ~IDebugTool() = default;
    
    // Display name shown in the menu
    virtual const char* GetName() const = 0;
    
    // Render the tool's ImGui content
    virtual void OnImGuiRender() = 0;
    
    // Called at the start of each frame (for timing tools)
    virtual void OnFrameStart() {}
    
    // Called at the end of each frame (for timing tools)
    virtual void OnFrameEnd() {}
    
    // Whether this tool should be shown by default
    virtual bool IsEnabledByDefault() const { return false; }
};

} // namespace se::debug

#endif // SE_ENABLE_DEBUG_TOOLS
