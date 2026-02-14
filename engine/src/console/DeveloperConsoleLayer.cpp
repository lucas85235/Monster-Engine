#include "engine/console/DeveloperConsoleLayer.h"

#include <algorithm>
#include <cmath>

#include "engine/console/ConsoleSystem.h"
#include "engine/input/InputManager.h"
#include "engine/ui/native/NativeUiRenderer.h"

namespace se {

namespace {

float ClampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

std::string TruncateWithEllipsis(ui::NativeUiRenderer& renderer, const std::string& source,
                                 float maxWidth, float scale) {
    if (source.empty() || maxWidth <= 0.0f) {
        return {};
    }

    if (renderer.MeasureTextWidth(source, scale) <= maxWidth) {
        return source;
    }

    static constexpr const char* kEllipsis = "...";
    const float ellipsisWidth = renderer.MeasureTextWidth(kEllipsis, scale);
    if (ellipsisWidth >= maxWidth) {
        return {};
    }

    size_t lo = 0;
    size_t hi = source.size();
    while (lo < hi) {
        const size_t mid = lo + (hi - lo + 1) / 2;
        const std::string candidate = source.substr(0, mid) + kEllipsis;
        if (renderer.MeasureTextWidth(candidate, scale) <= maxWidth) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }

    return source.substr(0, lo) + kEllipsis;
}

}  // namespace

DeveloperConsoleLayer::DeveloperConsoleLayer() : Layer("DeveloperConsoleLayer") {}

void DeveloperConsoleLayer::OnAttach() {
    inputBuffer_.clear();
    statusHint_.clear();
    historyIndex_        = -1;
    caretBlinkSeconds_   = 0.0f;
    caretVisible_        = true;
    inputFocused_        = true;
    draggingWindow_      = false;
    resizingWindow_      = false;
    scrollOffsetLines_   = 0;
    lastOutputVersion_   = ConsoleSystem::Get().GetOutputVersion();
    cachedOutputLineCount_ = ConsoleSystem::Get().GetOutputLineCount();
    stickyToBottom_      = true;
    visualRevision_      = 1;
    renderedRevision_    = 0;
    lastClearHover_      = false;
    lastCloseHover_      = false;
    lastSendHover_       = false;
    lastInputHover_      = false;
    lastResizeHover_     = false;
}

void DeveloperConsoleLayer::OnDetach() {
    if (cursorOverridden_) {
        InputManager::Get().SetCursorMode(previousCursorMode_);
        cursorOverridden_ = false;
    }
}

void DeveloperConsoleLayer::OnUpdate(float ts) {
    auto& console = ConsoleSystem::Get();
    auto& input   = InputManager::Get();

    const bool toggledThisFrame = input.IsActionJustPressed("engine_toggle_console");
    if (toggledThisFrame) {
        console.ToggleVisible();
        InvalidateVisual();
        if (console.IsVisible()) {
            inputFocused_      = true;
            stickyToBottom_    = true;
            scrollOffsetLines_ = 0;
            historyIndex_      = -1;
            statusHint_.clear();
            InvalidateVisual();
        }
    }

    SyncCursorCaptureState();

    if (!console.IsVisible()) {
        draggingWindow_ = false;
        resizingWindow_ = false;
        return;
    }

    auto& nativeUi = ui::NativeUiRenderer::Get();
    const float viewportWidth  = nativeUi.GetViewportWidth();
    const float viewportHeight = nativeUi.GetViewportHeight();
    InitializeLayoutIfNeeded(viewportWidth, viewportHeight);
    ClampWindowToViewport(viewportWidth, viewportHeight);

    HandleMouseInteraction(viewportWidth, viewportHeight, toggledThisFrame);
    HandleKeyboardAndTextInput(toggledThisFrame);
    UpdateScrollTracking();

    caretBlinkSeconds_ += ts;
    if (caretBlinkSeconds_ >= 0.5f) {
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = !caretVisible_;
        InvalidateVisual();
    }
}

void DeveloperConsoleLayer::OnRender() {
    auto& console = ConsoleSystem::Get();
    if (!console.IsVisible()) return;

    auto& nativeUi = ui::NativeUiRenderer::Get();
    if (!nativeUi.IsInitialized()) return;

    const UiRect window = GetWindowRect();
    const UiRect header = GetHeaderRect();
    const UiRect clear  = GetClearButtonRect();
    const UiRect close  = GetCloseButtonRect();
    const UiRect output = GetOutputRect();
    const UiRect inputBox = GetInputRect();
    const UiRect send   = GetSendButtonRect();
    const UiRect resize = GetResizeHandleRect();

    const auto mouse = InputManager::Get().GetMousePosition();
    const float mx = mouse.x;
    const float my = mouse.y;

    const bool clearHover  = clear.Contains(mx, my);
    const bool closeHover  = close.Contains(mx, my);
    const bool sendHover   = send.Contains(mx, my);
    const bool inputHover  = inputBox.Contains(mx, my);
    const bool resizeHover = resize.Contains(mx, my);

    if (clearHover != lastClearHover_ || closeHover != lastCloseHover_ ||
        sendHover != lastSendHover_ || inputHover != lastInputHover_ ||
        resizeHover != lastResizeHover_) {
        InvalidateVisual();
    }

    if (visualRevision_ == renderedRevision_) {
        nativeUi.RetainPreviousFrameGeometry();
        lastClearHover_  = clearHover;
        lastCloseHover_  = closeHover;
        lastSendHover_   = sendHover;
        lastInputHover_  = inputHover;
        lastResizeHover_ = resizeHover;
        return;
    }

    const ui::NativeUiColor shadow{0.0f, 0.0f, 0.0f, 0.35f};
    const ui::NativeUiColor panelBg{0.025f, 0.03f, 0.04f, 0.93f};
    const ui::NativeUiColor panelBorder{0.18f, 0.28f, 0.40f, 1.0f};
    const ui::NativeUiColor headerBg{0.09f, 0.15f, 0.22f, 0.98f};
    const ui::NativeUiColor titleColor{0.91f, 0.95f, 1.0f, 1.0f};
    const ui::NativeUiColor outputBg{0.01f, 0.015f, 0.025f, 0.95f};
    const ui::NativeUiColor outputBorder{0.15f, 0.24f, 0.34f, 1.0f};
    const ui::NativeUiColor textColor{0.85f, 0.9f, 0.96f, 1.0f};
    const ui::NativeUiColor hintColor{0.63f, 0.74f, 0.86f, 1.0f};
    const ui::NativeUiColor inputBg =
        inputFocused_ ? ui::NativeUiColor{0.07f, 0.12f, 0.17f, 0.98f}
                      : ui::NativeUiColor{0.05f, 0.08f, 0.12f, 0.95f};
    const ui::NativeUiColor buttonBg =
        sendHover ? ui::NativeUiColor{0.23f, 0.47f, 0.75f, 1.0f}
                  : ui::NativeUiColor{0.18f, 0.36f, 0.58f, 1.0f};
    const ui::NativeUiColor clearBg =
        clearHover ? ui::NativeUiColor{0.24f, 0.34f, 0.48f, 1.0f}
                   : ui::NativeUiColor{0.17f, 0.25f, 0.35f, 1.0f};
    const ui::NativeUiColor closeBg =
        closeHover ? ui::NativeUiColor{0.65f, 0.18f, 0.18f, 1.0f}
                   : ui::NativeUiColor{0.42f, 0.14f, 0.14f, 1.0f};

    nativeUi.DrawFilledRect(window.x + 6.0f, window.y + 6.0f, window.w, window.h, shadow);
    nativeUi.DrawFilledRect(window.x, window.y, window.w, window.h, panelBg);
    nativeUi.DrawRect(window.x, window.y, window.w, window.h, 2.0f, panelBorder);

    nativeUi.DrawFilledRect(header.x, header.y, header.w, header.h, headerBg);
    nativeUi.DrawText("Developer Console", header.x + 10.0f, header.y + 7.0f, titleColor, 1.0f);
    nativeUi.DrawText("` toggle | drag title to move | drag corner to resize",
                      header.x + 220.0f, header.y + 8.0f, hintColor, 0.90f);

    nativeUi.DrawFilledRect(clear.x, clear.y, clear.w, clear.h, clearBg);
    nativeUi.DrawRect(clear.x, clear.y, clear.w, clear.h, 1.0f, panelBorder);
    nativeUi.DrawText("Clear", clear.x + 8.0f, clear.y + 5.0f, titleColor, 0.95f);

    nativeUi.DrawFilledRect(close.x, close.y, close.w, close.h, closeBg);
    nativeUi.DrawRect(close.x, close.y, close.w, close.h, 1.0f, panelBorder);
    nativeUi.DrawText("X", close.x + 9.0f, close.y + 5.0f, titleColor, 1.0f);

    nativeUi.DrawFilledRect(output.x, output.y, output.w, output.h, outputBg);
    nativeUi.DrawRect(output.x, output.y, output.w, output.h, 1.0f, outputBorder);

    const float lineHeight = std::max(14.0f, nativeUi.GetLineHeight(0.95f) + 3.0f);
    const int visibleLines =
        std::max(1, static_cast<int>((output.h - 12.0f) / lineHeight));

    const int totalLines = static_cast<int>(console.GetOutputLineCount());
    const int maxOffset = std::max(0, totalLines - visibleLines);
    scrollOffsetLines_  = std::clamp(scrollOffsetLines_, 0, maxOffset);
    if (scrollOffsetLines_ == 0) {
        stickyToBottom_ = true;
    }

    const int startLine = std::max(0, totalLines - visibleLines - scrollOffsetLines_);
    const int lineCount = std::max(0, std::min(visibleLines, totalLines - startLine));
    const auto lines = console.GetOutputLinesRangeSnapshot(static_cast<size_t>(startLine),
                                                           static_cast<size_t>(lineCount));
    float lineY = output.y + 6.0f;
    const float maxTextWidth = output.w - 14.0f;

    for (const std::string& sourceLine : lines) {
        const std::string text = TruncateWithEllipsis(nativeUi, sourceLine, maxTextWidth, 0.95f);
        nativeUi.DrawText(text, output.x + 8.0f, lineY, textColor, 0.95f);
        lineY += lineHeight;
    }

    nativeUi.DrawFilledRect(inputBox.x, inputBox.y, inputBox.w, inputBox.h, inputBg);
    nativeUi.DrawRect(inputBox.x, inputBox.y, inputBox.w, inputBox.h, inputHover ? 2.0f : 1.0f,
                      outputBorder);

    nativeUi.DrawFilledRect(send.x, send.y, send.w, send.h, buttonBg);
    nativeUi.DrawRect(send.x, send.y, send.w, send.h, 1.0f, panelBorder);
    nativeUi.DrawText("Send", send.x + 18.0f, send.y + 8.0f, titleColor, 1.0f);

    std::string inputText = inputBuffer_;
    if (inputFocused_ && caretVisible_) {
        inputText.push_back('_');
    }
    nativeUi.DrawText("> " + inputText, inputBox.x + 8.0f, inputBox.y + 8.0f, titleColor, 1.0f);

    if (!statusHint_.empty()) {
        nativeUi.DrawText(statusHint_, window.x + 10.0f, window.y + window.h - 18.0f, hintColor, 0.90f);
    }

    // Resize grip
    const ui::NativeUiColor gripColor =
        resizeHover || resizingWindow_ ? ui::NativeUiColor{0.28f, 0.54f, 0.83f, 1.0f}
                                       : ui::NativeUiColor{0.17f, 0.27f, 0.39f, 1.0f};
    nativeUi.DrawFilledRect(resize.x, resize.y, resize.w, resize.h, gripColor);
    nativeUi.DrawText("::", resize.x + 4.0f, resize.y + 2.0f, titleColor, 0.9f);

    // Signal the renderer that this frame's geometry is eligible for reuse.
    // On the *next* frame, if OnRender early-returns (nothing changed), BeginFrame
    // will see the retain flag and skip clearing the buffers — keeping the console
    // visible without re-emitting every draw call.
    nativeUi.RetainPreviousFrameGeometry();

    renderedRevision_ = visualRevision_;
    lastClearHover_  = clearHover;
    lastCloseHover_  = closeHover;
    lastSendHover_   = sendHover;
    lastInputHover_  = inputHover;
    lastResizeHover_ = resizeHover;
}

void DeveloperConsoleLayer::InitializeLayoutIfNeeded(float viewportWidth, float viewportHeight) {
    if (layoutInitialized_) return;
    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) return;

    windowWidth_  = std::max(minWindowWidth_, viewportWidth * 0.78f);
    windowHeight_ = std::max(minWindowHeight_, viewportHeight * 0.62f);
    windowX_      = std::max(20.0f, (viewportWidth - windowWidth_) * 0.5f);
    windowY_      = std::max(20.0f, viewportHeight * 0.08f);
    layoutInitialized_ = true;
}

void DeveloperConsoleLayer::HandleMouseInteraction(float viewportWidth, float viewportHeight,
                                                   bool toggledThisFrame) {
    auto& console = ConsoleSystem::Get();
    auto& input   = InputManager::Get();

    // Avoid consuming stale click state on the same frame the console is toggled.
    if (toggledThisFrame) return;

    const auto mousePos = input.GetMousePosition();
    const float mx = mousePos.x;
    const float my = mousePos.y;

    const bool leftPressed  = input.IsMouseButtonJustPressed(Mouse::ButtonLeft);
    const bool leftDown     = input.IsMouseButtonDown(Mouse::ButtonLeft);
    const bool leftReleased = input.IsMouseButtonJustReleased(Mouse::ButtonLeft);

    const UiRect window = GetWindowRect();
    const UiRect header = GetHeaderRect();
    const UiRect clear  = GetClearButtonRect();
    const UiRect close  = GetCloseButtonRect();
    const UiRect output = GetOutputRect();
    const UiRect inputBox = GetInputRect();
    const UiRect send   = GetSendButtonRect();
    const UiRect resize = GetResizeHandleRect();

    if (leftPressed) {
        if (clear.Contains(mx, my)) {
            console.ClearOutput();
            inputFocused_ = true;
            stickyToBottom_ = true;
            scrollOffsetLines_ = 0;
            InvalidateVisual();
            return;
        }

        if (close.Contains(mx, my)) {
            console.SetVisible(false);
            SyncCursorCaptureState();
            InvalidateVisual();
            return;
        }

        if (send.Contains(mx, my)) {
            ExecuteCurrentInput();
            inputFocused_ = true;
            InvalidateVisual();
            return;
        }

        if (resize.Contains(mx, my)) {
            resizingWindow_    = true;
            draggingWindow_    = false;
            resizeStartMouseX_ = mx;
            resizeStartMouseY_ = my;
            resizeStartWidth_  = windowWidth_;
            resizeStartHeight_ = windowHeight_;
            InvalidateVisual();
            return;
        }

        if (header.Contains(mx, my)) {
            draggingWindow_ = true;
            resizingWindow_ = false;
            dragOffsetX_    = mx - windowX_;
            dragOffsetY_    = my - windowY_;
            inputFocused_   = false;
            InvalidateVisual();
            return;
        }

        if (inputBox.Contains(mx, my)) {
            if (!inputFocused_) {
                InvalidateVisual();
            }
            inputFocused_ = true;
        } else if (window.Contains(mx, my)) {
            if (inputFocused_) {
                InvalidateVisual();
            }
            inputFocused_ = false;
        } else {
            if (inputFocused_) {
                InvalidateVisual();
            }
            inputFocused_ = false;
        }
    }

    if (draggingWindow_) {
        if (leftDown) {
            const float oldX = windowX_;
            const float oldY = windowY_;
            windowX_ = mx - dragOffsetX_;
            windowY_ = my - dragOffsetY_;
            ClampWindowToViewport(viewportWidth, viewportHeight);
            if (std::abs(windowX_ - oldX) > 0.001f || std::abs(windowY_ - oldY) > 0.001f) {
                InvalidateVisual();
            }
        } else {
            draggingWindow_ = false;
            InvalidateVisual();
        }
    }

    if (resizingWindow_) {
        if (leftDown) {
            const float oldW = windowWidth_;
            const float oldH = windowHeight_;
            windowWidth_  = resizeStartWidth_ + (mx - resizeStartMouseX_);
            windowHeight_ = resizeStartHeight_ + (my - resizeStartMouseY_);
            ClampWindowToViewport(viewportWidth, viewportHeight);
            if (std::abs(windowWidth_ - oldW) > 0.001f || std::abs(windowHeight_ - oldH) > 0.001f) {
                InvalidateVisual();
            }
        } else {
            resizingWindow_ = false;
            InvalidateVisual();
        }
    }

    if (leftReleased) {
        if (draggingWindow_ || resizingWindow_) {
            InvalidateVisual();
        }
        draggingWindow_ = false;
        resizingWindow_ = false;
    }

    if (output.Contains(mx, my)) {
        const float scroll = input.GetScrollDelta();
        if (std::abs(scroll) > 0.01f) {
            const int deltaLines = std::max(1, static_cast<int>(std::round(std::abs(scroll) * 3.0f)));
            if (scroll > 0.0f) {
                scrollOffsetLines_ += deltaLines;
            } else {
                scrollOffsetLines_ -= deltaLines;
            }
            if (scrollOffsetLines_ < 0) {
                scrollOffsetLines_ = 0;
            }
            stickyToBottom_ = (scrollOffsetLines_ == 0);
            InvalidateVisual();
        }
    }
}

void DeveloperConsoleLayer::HandleKeyboardAndTextInput(bool toggledThisFrame) {
    auto& console = ConsoleSystem::Get();
    auto& input   = InputManager::Get();

    if (input.IsKeyJustPressed(Key::Escape)) {
        console.SetVisible(false);
        SyncCursorCaptureState();
        InvalidateVisual();
        return;
    }

    if (!inputFocused_) return;

    for (uint32_t codepoint : input.ConsumeTextInput()) {
        if (codepoint == static_cast<uint32_t>('`') && toggledThisFrame) {
            continue;
        }
        if (codepoint < 32 || codepoint > 126) {
            continue;
        }
        if (inputBuffer_.size() >= 512) {
            continue;
        }
        inputBuffer_.push_back(static_cast<char>(codepoint));
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Backspace) && !inputBuffer_.empty()) {
        inputBuffer_.pop_back();
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Up)) {
        HandleHistoryUp();
    }
    if (input.IsKeyJustPressed(Key::Down)) {
        HandleHistoryDown();
    }
    if (input.IsKeyJustPressed(Key::Tab)) {
        HandleAutoComplete();
    }
    if (input.IsKeyJustPressed(Key::Enter)) {
        ExecuteCurrentInput();
        InvalidateVisual();
    }
}

void DeveloperConsoleLayer::UpdateScrollTracking() {
    auto& nativeUi = ui::NativeUiRenderer::Get();
    const float lineHeight = std::max(14.0f, nativeUi.GetLineHeight(0.95f) + 3.0f);
    const UiRect output = GetOutputRect();
    const int visibleLines = std::max(1, static_cast<int>((output.h - 12.0f) / lineHeight));

    auto& console = ConsoleSystem::Get();
    const uint64_t outputVersion = console.GetOutputVersion();
    cachedOutputLineCount_ = console.GetOutputLineCount();
    if (outputVersion != lastOutputVersion_) {
        if (stickyToBottom_) {
            scrollOffsetLines_ = 0;
        }
        lastOutputVersion_ = outputVersion;
        InvalidateVisual();
    }

    const int totalLines = static_cast<int>(cachedOutputLineCount_);
    const int maxOffset = std::max(0, totalLines - visibleLines);
    scrollOffsetLines_   = std::clamp(scrollOffsetLines_, 0, maxOffset);
    if (scrollOffsetLines_ == 0) {
        stickyToBottom_ = true;
    }
}

void DeveloperConsoleLayer::ClampWindowToViewport(float viewportWidth, float viewportHeight) {
    const float maxWidth  = std::max(minWindowWidth_, viewportWidth - 8.0f);
    const float maxHeight = std::max(minWindowHeight_, viewportHeight - 8.0f);

    windowWidth_  = ClampFloat(windowWidth_, minWindowWidth_, maxWidth);
    windowHeight_ = ClampFloat(windowHeight_, minWindowHeight_, maxHeight);

    const float maxX = std::max(0.0f, viewportWidth - windowWidth_);
    const float maxY = std::max(0.0f, viewportHeight - windowHeight_);
    windowX_ = ClampFloat(windowX_, 0.0f, maxX);
    windowY_ = ClampFloat(windowY_, 0.0f, maxY);
}

void DeveloperConsoleLayer::ExecuteCurrentInput() {
    if (inputBuffer_.empty()) {
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        return;
    }

    auto& console = ConsoleSystem::Get();
    console.AddOutput("> " + inputBuffer_);
    console.Execute(inputBuffer_);

    inputBuffer_.clear();
    statusHint_.clear();
    historyIndex_      = -1;
    stickyToBottom_    = true;
    scrollOffsetLines_ = 0;
    caretBlinkSeconds_ = 0.0f;
    caretVisible_      = true;
    InvalidateVisual();
}

void DeveloperConsoleLayer::HandleAutoComplete() {
    auto& console = ConsoleSystem::Get();
    if (inputBuffer_.empty()) return;

    const size_t tokenEnd = inputBuffer_.find_first_of(" \t");
    const std::string token =
        tokenEnd == std::string::npos ? inputBuffer_ : inputBuffer_.substr(0, tokenEnd);

    if (token.empty()) return;

    auto matches = console.AutoComplete(token);
    if (matches.empty()) {
        statusHint_ = "no match";
        InvalidateVisual();
        return;
    }

    if (matches.size() == 1) {
        if (tokenEnd == std::string::npos) {
            inputBuffer_ = matches[0] + " ";
        } else {
            inputBuffer_.replace(0, tokenEnd, matches[0]);
        }
        statusHint_.clear();
        InvalidateVisual();
        return;
    }

    console.AddOutput("Completions for '" + token + "':");
    for (const auto& match : matches) {
        console.AddOutput("  " + match);
    }
    stickyToBottom_    = true;
    scrollOffsetLines_ = 0;
    statusHint_        = std::to_string(matches.size()) + " matches";
    InvalidateVisual();
}

void DeveloperConsoleLayer::HandleHistoryUp() {
    const auto history = ConsoleSystem::Get().GetCommandHistorySnapshot();
    if (history.empty()) return;

    if (historyIndex_ < 0) {
        historyIndex_ = static_cast<int>(history.size()) - 1;
    } else if (historyIndex_ > 0) {
        historyIndex_--;
    }

    if (historyIndex_ >= 0 && historyIndex_ < static_cast<int>(history.size())) {
        inputBuffer_ = history[static_cast<size_t>(historyIndex_)];
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }
}

void DeveloperConsoleLayer::HandleHistoryDown() {
    const auto history = ConsoleSystem::Get().GetCommandHistorySnapshot();
    if (history.empty()) return;

    if (historyIndex_ < 0) return;

    historyIndex_++;
    if (historyIndex_ >= static_cast<int>(history.size())) {
        historyIndex_ = -1;
        inputBuffer_.clear();
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
        return;
    }

    inputBuffer_ = history[static_cast<size_t>(historyIndex_)];
    caretBlinkSeconds_ = 0.0f;
    caretVisible_      = true;
    InvalidateVisual();
}

void DeveloperConsoleLayer::SyncCursorCaptureState() {
    auto& console = ConsoleSystem::Get();
    auto& input   = InputManager::Get();

    if (console.IsVisible()) {
        if (!cursorOverridden_) {
            previousCursorMode_ = input.GetCursorMode();
            input.SetCursorMode(CursorMode::Normal);
            cursorOverridden_ = true;
        }
    } else if (cursorOverridden_) {
        input.SetCursorMode(previousCursorMode_);
        cursorOverridden_ = false;
    }
}

void DeveloperConsoleLayer::InvalidateVisual() {
    ++visualRevision_;
    if (visualRevision_ == 0) {
        visualRevision_ = 1;
    }
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetWindowRect() const {
    return UiRect{windowX_, windowY_, windowWidth_, windowHeight_};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetHeaderRect() const {
    return UiRect{windowX_, windowY_, windowWidth_, 34.0f};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetClearButtonRect() const {
    return UiRect{windowX_ + windowWidth_ - 96.0f, windowY_ + 5.0f, 58.0f, 24.0f};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetCloseButtonRect() const {
    return UiRect{windowX_ + windowWidth_ - 30.0f, windowY_ + 5.0f, 24.0f, 24.0f};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetOutputRect() const {
    const float pad = 10.0f;
    const float headerHeight = 34.0f;
    const float inputRowHeight = 44.0f;
    const float x = windowX_ + pad;
    const float y = windowY_ + headerHeight + pad;
    const float w = windowWidth_ - pad * 2.0f;
    const float h = windowHeight_ - headerHeight - inputRowHeight - pad * 3.0f;
    return UiRect{x, y, w, std::max(60.0f, h)};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetInputRect() const {
    const float pad = 10.0f;
    const float rowHeight = 34.0f;
    const float sendWidth = 84.0f;
    const float x = windowX_ + pad;
    const float y = windowY_ + windowHeight_ - rowHeight - pad;
    const float w = windowWidth_ - pad * 3.0f - sendWidth;
    return UiRect{x, y, std::max(120.0f, w), rowHeight};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetSendButtonRect() const {
    const UiRect inputRect = GetInputRect();
    return UiRect{inputRect.x + inputRect.w + 10.0f, inputRect.y, 84.0f, inputRect.h};
}

DeveloperConsoleLayer::UiRect DeveloperConsoleLayer::GetResizeHandleRect() const {
    return UiRect{windowX_ + windowWidth_ - 16.0f, windowY_ + windowHeight_ - 16.0f, 12.0f, 12.0f};
}

}  // namespace se
