#pragma once

#include <cstdint>
#include <string>

#include "Engine.h"
#include "engine/Layer.h"
#include "engine/input/InputManager.h"

namespace se {

class DeveloperConsoleLayer : public Layer {
   public:
    DeveloperConsoleLayer();
    ~DeveloperConsoleLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;

   private:
    struct UiRect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;

        bool Contains(float px, float py) const {
            return px >= x && py >= y && px <= (x + w) && py <= (y + h);
        }
    };

    void InitializeLayoutIfNeeded(float viewportWidth, float viewportHeight);
    void HandleMouseInteraction(float viewportWidth, float viewportHeight, bool toggledThisFrame);
    void HandleKeyboardAndTextInput(bool toggledThisFrame);
    void UpdateScrollTracking();
    void ClampWindowToViewport(float viewportWidth, float viewportHeight);

    void ExecuteCurrentInput();
    void HandleAutoComplete();
    void HandleHistoryUp();
    void HandleHistoryDown();
    void SyncCursorCaptureState();
    void InvalidateVisual();

    UiRect GetWindowRect() const;
    UiRect GetHeaderRect() const;
    UiRect GetClearButtonRect() const;
    UiRect GetCloseButtonRect() const;
    UiRect GetOutputRect() const;
    UiRect GetInputRect() const;
    UiRect GetSendButtonRect() const;
    UiRect GetResizeHandleRect() const;

    std::string inputBuffer_;
    std::string statusHint_;
    int         historyIndex_      = -1;
    float       caretBlinkSeconds_ = 0.0f;
    bool        caretVisible_      = true;
    bool        inputFocused_      = true;
    bool        layoutInitialized_ = false;

    // Window geometry
    float windowX_ = 40.0f;
    float windowY_ = 40.0f;
    float windowWidth_ = 980.0f;
    float windowHeight_ = 520.0f;
    float minWindowWidth_ = 520.0f;
    float minWindowHeight_ = 260.0f;

    // Drag / resize state
    bool  draggingWindow_ = false;
    bool  resizingWindow_ = false;
    float dragOffsetX_ = 0.0f;
    float dragOffsetY_ = 0.0f;
    float resizeStartMouseX_ = 0.0f;
    float resizeStartMouseY_ = 0.0f;
    float resizeStartWidth_  = 0.0f;
    float resizeStartHeight_ = 0.0f;

    // Log scroll state (0 = bottom)
    int      scrollOffsetLines_    = 0;
    uint64_t lastOutputVersion_    = 0;
    size_t   cachedOutputLineCount_ = 0;
    bool     stickyToBottom_       = true;

    bool        cursorOverridden_  = false;
    CursorMode  previousCursorMode_ = CursorMode::Normal;

    uint64_t visualRevision_   = 1;
    uint64_t renderedRevision_ = 0;

    bool lastClearHover_  = false;
    bool lastCloseHover_  = false;
    bool lastSendHover_   = false;
    bool lastInputHover_  = false;
    bool lastResizeHover_ = false;
};

}  // namespace se
