#include "engine/console/DeveloperConsoleLayer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

#include <GLFW/glfw3.h>

#include "engine/Application.h"
#include "engine/console/ConsoleSystem.h"
#include "engine/input/InputManager.h"
#include "engine/ui/native/NativeUiRenderer.h"

namespace se {

namespace {

float ClampFloat(float value, float minValue, float maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

constexpr float kOutputTextScale = 0.95f;

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

struct OutputSnapshot {
    int                      totalLines    = 0;
    int                      visibleLines  = 0;
    int                      maxOffset     = 0;
    int                      clampedOffset = 0;
    int                      startLine     = 0;
    float                    maxTextWidth  = 0.0f;
    float                    lineHeight    = 0.0f;
    float                    textX         = 0.0f;
    float                    firstLineY    = 0.0f;
    std::vector<std::string> renderedLines;
};

OutputSnapshot BuildOutputSnapshot(ConsoleSystem& console, ui::NativeUiRenderer& renderer, float outputX,
                                   float outputY, float outputW, float outputH, int scrollOffsetLines) {
    OutputSnapshot snapshot{};
    snapshot.lineHeight = std::max(14.0f, renderer.GetLineHeight(kOutputTextScale) + 3.0f);
    snapshot.visibleLines = std::max(1, static_cast<int>((outputH - 12.0f) / snapshot.lineHeight));

    snapshot.totalLines    = static_cast<int>(console.GetOutputLineCount());
    snapshot.maxOffset     = std::max(0, snapshot.totalLines - snapshot.visibleLines);
    snapshot.clampedOffset = std::clamp(scrollOffsetLines, 0, snapshot.maxOffset);

    snapshot.startLine = std::max(0, snapshot.totalLines - snapshot.visibleLines - snapshot.clampedOffset);
    const int lineCount =
        std::max(0, std::min(snapshot.visibleLines, snapshot.totalLines - snapshot.startLine));
    const auto lines = console.GetOutputLinesRangeSnapshot(static_cast<size_t>(snapshot.startLine),
                                                           static_cast<size_t>(lineCount));

    snapshot.maxTextWidth = std::max(0.0f, outputW - 14.0f);
    snapshot.textX        = std::round(outputX + 8.0f);
    snapshot.firstLineY   = std::round(outputY + 6.0f);
    snapshot.renderedLines.reserve(lines.size());
    for (const std::string& sourceLine : lines) {
        snapshot.renderedLines.push_back(
            TruncateWithEllipsis(renderer, sourceLine, snapshot.maxTextWidth, kOutputTextScale));
    }
    return snapshot;
}

float MeasureTextPrefixWidth(ui::NativeUiRenderer& renderer, const std::string& text, size_t column,
                             float scale) {
    const size_t clampedColumn = std::min(column, text.size());
    if (clampedColumn == 0) {
        return 0.0f;
    }
    return renderer.MeasureTextWidth(std::string_view(text.data(), clampedColumn), scale);
}

bool IsControlPressed(const InputManager& input) {
    return input.IsKeyDown(Key::LeftControl) || input.IsKeyDown(Key::RightControl);
}

void NormalizeSelection(int lineA, size_t colA, int lineB, size_t colB, int* outStartLine,
                        size_t* outStartCol, int* outEndLine, size_t* outEndCol) {
    if (!outStartLine || !outStartCol || !outEndLine || !outEndCol) return;

    const bool aComesFirst = (lineA < lineB) || (lineA == lineB && colA <= colB);
    if (aComesFirst) {
        *outStartLine = lineA;
        *outStartCol  = colA;
        *outEndLine   = lineB;
        *outEndCol    = colB;
    } else {
        *outStartLine = lineB;
        *outStartCol  = colB;
        *outEndLine   = lineA;
        *outEndCol    = colA;
    }
}

}  // namespace

DeveloperConsoleLayer::DeveloperConsoleLayer() : Layer("DeveloperConsoleLayer") {}

void DeveloperConsoleLayer::OnAttach() {
    inputBuffer_.clear();
    statusHint_.clear();
    historyIndex_        = -1;
    caretBlinkSeconds_   = 0.0f;
    caretVisible_        = true;
    caretIndex_          = 0;
    inputFocused_        = true;
    draggingWindow_      = false;
    resizingWindow_      = false;
    scrollOffsetLines_   = 0;
    lastOutputVersion_   = ConsoleSystem::Get().GetOutputVersion();
    cachedOutputLineCount_ = ConsoleSystem::Get().GetOutputLineCount();
    stickyToBottom_      = true;
    outputSelecting_      = false;
    outputSelectionActive_ = false;
    selectionAnchorLine_  = -1;
    selectionAnchorColumn_ = 0;
    selectionCaretLine_   = -1;
    selectionCaretColumn_ = 0;
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

    const bool toggledThisFrame = input.IsActionJustPressed("engine_toggle_console") ||
                                  input.IsKeyJustPressed(Key::GraveAccent) ||
                                  input.IsKeyJustPressed(Key::F1);
    if (toggledThisFrame) {
        console.ToggleVisible();
        ClearOutputSelection();
        InvalidateVisual();
        if (console.IsVisible()) {
            inputFocused_      = true;
            stickyToBottom_    = true;
            scrollOffsetLines_ = 0;
            historyIndex_      = -1;
            caretIndex_        = inputBuffer_.size();
            statusHint_.clear();
            InvalidateVisual();
        }
    }

    SyncCursorCaptureState();

    if (!console.IsVisible()) {
        draggingWindow_ = false;
        resizingWindow_ = false;
        outputSelecting_ = false;
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
    if (!nativeUi.IsInitialized()) {
        static bool warnedUninitialized = false;
        if (!warnedUninitialized) {
            std::cerr
                << "DeveloperConsoleLayer: NativeUiRenderer is not initialized; console cannot render."
                << std::endl;
            warnedUninitialized = true;
        }
        return;
    }

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

    constexpr float kHeaderTitleScale = 1.0f;
    constexpr float kHeaderHintScale  = 0.90f;
    nativeUi.DrawFilledRect(header.x, header.y, header.w, header.h, headerBg);
    const float titleAreaX = std::round(header.x + 10.0f);

    const std::string headerHint = "` / F1 toggle | drag title to move | drag corner to resize";
    const float hintX            = std::round(header.x + 220.0f);
    const float hintMaxWidth     = std::max(0.0f, clear.x - hintX - 8.0f);
    const float titleAreaW       = std::max(0.0f, hintX - titleAreaX - 10.0f);
    nativeUi.DrawTextAligned("Developer Console", titleAreaX, header.y, titleAreaW, header.h,
                             titleColor, kHeaderTitleScale,
                             ui::NativeUiRenderer::TextHorizontalAlign::Left,
                             ui::NativeUiRenderer::TextVerticalAlign::Center);

    const std::string hintText =
        TruncateWithEllipsis(nativeUi, headerHint, hintMaxWidth, kHeaderHintScale);
    if (!hintText.empty()) {
        nativeUi.DrawTextAligned(hintText, hintX, header.y, hintMaxWidth, header.h, hintColor,
                                 kHeaderHintScale,
                                 ui::NativeUiRenderer::TextHorizontalAlign::Left,
                                 ui::NativeUiRenderer::TextVerticalAlign::Center);
    }

    constexpr float kButtonScale = 0.95f;
    nativeUi.DrawFilledRect(clear.x, clear.y, clear.w, clear.h, clearBg);
    nativeUi.DrawRect(clear.x, clear.y, clear.w, clear.h, 1.0f, panelBorder);
    nativeUi.DrawTextAligned("Clear", clear.x, clear.y, clear.w, clear.h, titleColor, kButtonScale,
                             ui::NativeUiRenderer::TextHorizontalAlign::Center,
                             ui::NativeUiRenderer::TextVerticalAlign::Center);

    nativeUi.DrawFilledRect(close.x, close.y, close.w, close.h, closeBg);
    nativeUi.DrawRect(close.x, close.y, close.w, close.h, 1.0f, panelBorder);
    nativeUi.DrawTextAligned("X", close.x, close.y, close.w, close.h, titleColor, kHeaderTitleScale,
                             ui::NativeUiRenderer::TextHorizontalAlign::Center,
                             ui::NativeUiRenderer::TextVerticalAlign::Center);

    nativeUi.DrawFilledRect(output.x, output.y, output.w, output.h, outputBg);
    nativeUi.DrawRect(output.x, output.y, output.w, output.h, 1.0f, outputBorder);

    const OutputSnapshot outputSnapshot =
        BuildOutputSnapshot(console, nativeUi, output.x, output.y, output.w, output.h, scrollOffsetLines_);
    scrollOffsetLines_ = outputSnapshot.clampedOffset;
    if (scrollOffsetLines_ == 0) {
        stickyToBottom_ = true;
    }

    if (HasOutputSelection() && !outputSnapshot.renderedLines.empty()) {
        int startLine = 0;
        int endLine   = 0;
        size_t startCol = 0;
        size_t endCol   = 0;
        NormalizeSelection(selectionAnchorLine_, selectionAnchorColumn_, selectionCaretLine_,
                           selectionCaretColumn_, &startLine, &startCol, &endLine, &endCol);

        const int visibleStartLine = outputSnapshot.startLine;
        const int visibleEndLine =
            visibleStartLine + static_cast<int>(outputSnapshot.renderedLines.size()) - 1;
        const int drawStartLine = std::max(startLine, visibleStartLine);
        const int drawEndLine   = std::min(endLine, visibleEndLine);
        if (drawStartLine <= drawEndLine) {
            const ui::NativeUiColor selectionColor{0.19f, 0.34f, 0.53f, 0.60f};
            for (int line = drawStartLine; line <= drawEndLine; ++line) {
                const int localIndex = line - visibleStartLine;
                if (localIndex < 0 ||
                    localIndex >= static_cast<int>(outputSnapshot.renderedLines.size())) {
                    continue;
                }

                const std::string& renderedLine = outputSnapshot.renderedLines[static_cast<size_t>(localIndex)];
                size_t lineStartCol = 0;
                size_t lineEndCol   = renderedLine.size();
                if (line == startLine) {
                    lineStartCol = startCol;
                }
                if (line == endLine) {
                    lineEndCol = endCol;
                }
                lineStartCol = std::min(lineStartCol, renderedLine.size());
                lineEndCol   = std::min(lineEndCol, renderedLine.size());
                if (lineStartCol >= lineEndCol) {
                    continue;
                }

                const float x0 = outputSnapshot.textX +
                                 MeasureTextPrefixWidth(nativeUi, renderedLine, lineStartCol,
                                                        kOutputTextScale);
                const float x1 = outputSnapshot.textX +
                                 MeasureTextPrefixWidth(nativeUi, renderedLine, lineEndCol,
                                                        kOutputTextScale);
                const float y = std::round(outputSnapshot.firstLineY +
                                           outputSnapshot.lineHeight * static_cast<float>(localIndex));
                const float w = std::max(0.0f, std::round(x1 - x0));
                if (w > 0.0f) {
                    nativeUi.DrawFilledRect(std::round(x0), y, w,
                                            std::round(outputSnapshot.lineHeight), selectionColor);
                }
            }
        }
    }

    float lineY = outputSnapshot.firstLineY;
    for (const std::string& line : outputSnapshot.renderedLines) {
        nativeUi.DrawText(line, outputSnapshot.textX, std::round(lineY), textColor, kOutputTextScale);
        lineY += outputSnapshot.lineHeight;
    }

    nativeUi.DrawFilledRect(inputBox.x, inputBox.y, inputBox.w, inputBox.h, inputBg);
    nativeUi.DrawRect(inputBox.x, inputBox.y, inputBox.w, inputBox.h, inputHover ? 2.0f : 1.0f,
                      outputBorder);

    nativeUi.DrawFilledRect(send.x, send.y, send.w, send.h, buttonBg);
    nativeUi.DrawRect(send.x, send.y, send.w, send.h, 1.0f, panelBorder);
    nativeUi.DrawTextAligned("Send", send.x, send.y, send.w, send.h, titleColor, kHeaderTitleScale,
                             ui::NativeUiRenderer::TextHorizontalAlign::Center,
                             ui::NativeUiRenderer::TextVerticalAlign::Center);

    const std::string inputText = "> " + inputBuffer_;
    const size_t clampedCaretIndex = std::min(caretIndex_, inputBuffer_.size());
    const std::string caretText = "> " + inputBuffer_.substr(0, clampedCaretIndex);
    ui::NativeUiRenderer::TextPadding inputPadding{};
    inputPadding.left  = 8.0f;
    inputPadding.right = 8.0f;
    const auto inputMetrics = nativeUi.MeasureTextLayout(inputText, kHeaderTitleScale);
    const auto caretMetrics = nativeUi.MeasureTextLayout(caretText, kHeaderTitleScale);
    const float inputContentX = inputBox.x + inputPadding.left;
    const float inputContentY = inputBox.y + inputPadding.top;
    const float inputContentH =
        std::max(0.0f, inputBox.h - inputPadding.top - inputPadding.bottom);
    const float inputOriginX = inputContentX - inputMetrics.minX;
    const float inputOriginY = inputContentY + (inputContentH - inputMetrics.InkHeight()) * 0.5f -
                               inputMetrics.minY;
    nativeUi.DrawTextAligned(inputText, inputBox.x, inputBox.y, inputBox.w, inputBox.h, titleColor,
                             kHeaderTitleScale, ui::NativeUiRenderer::TextHorizontalAlign::Left,
                             ui::NativeUiRenderer::TextVerticalAlign::Center, inputPadding, true);

    if (inputFocused_ && caretVisible_) {
        const float caretX =
            std::round(inputOriginX + caretMetrics.advanceWidth + 1.0f);
        const float caretY = std::round(inputOriginY + inputMetrics.minY);
        const float caretH = std::max(8.0f, inputMetrics.InkHeight());
        nativeUi.DrawFilledRect(caretX, caretY, 1.0f, caretH, titleColor);
    }

    if (!statusHint_.empty()) {
        nativeUi.DrawText(statusHint_, std::round(window.x + 10.0f),
                          std::round(window.y + window.h - 18.0f), hintColor, 0.90f);
    }

    // Resize grip
    const ui::NativeUiColor gripColor =
        resizeHover || resizingWindow_ ? ui::NativeUiColor{0.28f, 0.54f, 0.83f, 1.0f}
                                       : ui::NativeUiColor{0.17f, 0.27f, 0.39f, 1.0f};
    nativeUi.DrawFilledRect(resize.x, resize.y, resize.w, resize.h, gripColor);
    nativeUi.DrawTextAligned("::", resize.x, resize.y, resize.w, resize.h, titleColor, 0.9f,
                             ui::NativeUiRenderer::TextHorizontalAlign::Center,
                             ui::NativeUiRenderer::TextVerticalAlign::Center);

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
            ClearOutputSelection();
            inputFocused_ = true;
            stickyToBottom_ = true;
            scrollOffsetLines_ = 0;
            InvalidateVisual();
            return;
        }

        if (close.Contains(mx, my)) {
            console.SetVisible(false);
            SyncCursorCaptureState();
            ClearOutputSelection();
            InvalidateVisual();
            return;
        }

        if (send.Contains(mx, my)) {
            ExecuteCurrentInput();
            ClearOutputSelection();
            inputFocused_ = true;
            InvalidateVisual();
            return;
        }

        if (resize.Contains(mx, my)) {
            resizingWindow_    = true;
            draggingWindow_    = false;
            outputSelecting_   = false;
            resizeStartMouseX_ = mx;
            resizeStartMouseY_ = my;
            resizeStartWidth_  = windowWidth_;
            resizeStartHeight_ = windowHeight_;
            ClearOutputSelection();
            InvalidateVisual();
            return;
        }

        if (header.Contains(mx, my)) {
            draggingWindow_ = true;
            resizingWindow_ = false;
            outputSelecting_ = false;
            dragOffsetX_    = mx - windowX_;
            dragOffsetY_    = my - windowY_;
            inputFocused_   = false;
            ClearOutputSelection();
            InvalidateVisual();
            return;
        }

        if (output.Contains(mx, my)) {
            int line = -1;
            size_t column = 0;
            if (GetOutputCursorFromMouse(mx, my, &line, &column, true)) {
                outputSelecting_       = true;
                outputSelectionActive_ = true;
                selectionAnchorLine_   = line;
                selectionAnchorColumn_ = column;
                selectionCaretLine_    = line;
                selectionCaretColumn_  = column;
            } else {
                ClearOutputSelection();
            }
            if (inputFocused_) {
                inputFocused_ = false;
            }
            InvalidateVisual();
            return;
        }

        if (inputBox.Contains(mx, my)) {
            const bool hadSelection = outputSelectionActive_ || outputSelecting_;
            ClearOutputSelection();
            if (!inputFocused_ || hadSelection) {
                InvalidateVisual();
            }
            inputFocused_ = true;
        } else if (window.Contains(mx, my)) {
            const bool hadSelection = outputSelectionActive_ || outputSelecting_;
            ClearOutputSelection();
            if (inputFocused_ || hadSelection) {
                InvalidateVisual();
            }
            inputFocused_ = false;
        } else {
            const bool hadSelection = outputSelectionActive_ || outputSelecting_;
            ClearOutputSelection();
            if (inputFocused_ || hadSelection) {
                InvalidateVisual();
            }
            inputFocused_ = false;
        }
    }

    if (outputSelecting_) {
        if (leftDown) {
            int line = -1;
            size_t column = 0;
            if (GetOutputCursorFromMouse(mx, my, &line, &column, true)) {
                if (line != selectionCaretLine_ || column != selectionCaretColumn_) {
                    selectionCaretLine_   = line;
                    selectionCaretColumn_ = column;
                    outputSelectionActive_ = true;
                    InvalidateVisual();
                }
            }
        } else {
            outputSelecting_ = false;
            if (!HasOutputSelection()) {
                outputSelectionActive_ = false;
            }
            InvalidateVisual();
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
        if (draggingWindow_ || resizingWindow_ || outputSelecting_) {
            InvalidateVisual();
        }
        if (outputSelecting_) {
            outputSelecting_ = false;
            if (!HasOutputSelection()) {
                outputSelectionActive_ = false;
            }
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

    if (IsControlPressed(input) && input.IsKeyJustPressed(Key::C)) {
        std::string copiedText;
        if (HasOutputSelection()) {
            copiedText = BuildSelectedOutputText();
        } else if (inputFocused_ && !inputBuffer_.empty()) {
            copiedText = inputBuffer_;
        }

        if (!copiedText.empty()) {
            CopyToClipboard(copiedText);
            statusHint_ = "copied";
            InvalidateVisual();
        }
        return;
    }

    if (IsControlPressed(input) && inputFocused_ && input.IsKeyJustPressed(Key::V)) {
        if (PasteFromClipboard()) {
            statusHint_.clear();
            caretBlinkSeconds_ = 0.0f;
            caretVisible_      = true;
            InvalidateVisual();
        }
        return;
    }

    if (!inputFocused_) {
        input.ConsumeTextInput();
        return;
    }

    caretIndex_ = std::min(caretIndex_, inputBuffer_.size());

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
        inputBuffer_.insert(caretIndex_, 1, static_cast<char>(codepoint));
        ++caretIndex_;
        historyIndex_ = -1;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Backspace) && caretIndex_ > 0 && !inputBuffer_.empty()) {
        inputBuffer_.erase(caretIndex_ - 1, 1);
        --caretIndex_;
        historyIndex_ = -1;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Delete) && caretIndex_ < inputBuffer_.size()) {
        inputBuffer_.erase(caretIndex_, 1);
        historyIndex_ = -1;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Left) && caretIndex_ > 0) {
        --caretIndex_;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Right) && caretIndex_ < inputBuffer_.size()) {
        ++caretIndex_;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::Home) && caretIndex_ != 0) {
        caretIndex_        = 0;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
    }

    if (input.IsKeyJustPressed(Key::End) && caretIndex_ != inputBuffer_.size()) {
        caretIndex_        = inputBuffer_.size();
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

    // Snap to pixel grid to avoid blurry edges/text from fractional coordinates.
    windowX_      = std::round(windowX_);
    windowY_      = std::round(windowY_);
    windowWidth_  = std::round(windowWidth_);
    windowHeight_ = std::round(windowHeight_);
}

void DeveloperConsoleLayer::ExecuteCurrentInput() {
    if (inputBuffer_.empty()) {
        caretIndex_        = 0;
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
    caretIndex_        = 0;
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
        caretIndex_ = inputBuffer_.size();
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
        caretIndex_ = inputBuffer_.size();
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
        caretIndex_ = 0;
        caretBlinkSeconds_ = 0.0f;
        caretVisible_      = true;
        InvalidateVisual();
        return;
    }

    inputBuffer_ = history[static_cast<size_t>(historyIndex_)];
    caretIndex_ = inputBuffer_.size();
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

void DeveloperConsoleLayer::ClearOutputSelection() {
    outputSelecting_       = false;
    outputSelectionActive_ = false;
    selectionAnchorLine_   = -1;
    selectionAnchorColumn_ = 0;
    selectionCaretLine_    = -1;
    selectionCaretColumn_  = 0;
}

bool DeveloperConsoleLayer::HasOutputSelection() const {
    if (!outputSelectionActive_) {
        return false;
    }
    if (selectionAnchorLine_ < 0 || selectionCaretLine_ < 0) {
        return false;
    }
    return selectionAnchorLine_ != selectionCaretLine_ ||
           selectionAnchorColumn_ != selectionCaretColumn_;
}

bool DeveloperConsoleLayer::GetOutputCursorFromMouse(float mx, float my, int* outLine,
                                                      size_t* outColumn,
                                                      bool clampToBounds) const {
    if (!outLine || !outColumn) {
        return false;
    }

    auto& console  = ConsoleSystem::Get();
    auto& nativeUi = ui::NativeUiRenderer::Get();
    if (!nativeUi.IsInitialized()) {
        return false;
    }

    const UiRect output = GetOutputRect();
    float clampedX = mx;
    float clampedY = my;
    if (clampToBounds) {
        clampedX = ClampFloat(clampedX, output.x, output.x + output.w);
        clampedY = ClampFloat(clampedY, output.y, output.y + output.h);
    } else if (!output.Contains(mx, my)) {
        return false;
    }

    const OutputSnapshot snapshot =
        BuildOutputSnapshot(console, nativeUi, output.x, output.y, output.w, output.h, scrollOffsetLines_);
    if (snapshot.renderedLines.empty()) {
        return false;
    }

    const float textTop = snapshot.firstLineY;
    const float textBottom =
        snapshot.firstLineY + snapshot.lineHeight * static_cast<float>(snapshot.renderedLines.size());
    if (!clampToBounds && (clampedY < textTop || clampedY >= textBottom)) {
        return false;
    }
    clampedY = ClampFloat(clampedY, textTop, std::max(textTop, textBottom - 0.0001f));

    int localLine = static_cast<int>((clampedY - textTop) / snapshot.lineHeight);
    localLine =
        std::clamp(localLine, 0, static_cast<int>(snapshot.renderedLines.size()) - 1);

    const std::string& lineText = snapshot.renderedLines[static_cast<size_t>(localLine)];
    const float xRelative = clampedX - snapshot.textX;
    size_t column = lineText.size();
    if (xRelative <= 0.0f) {
        column = 0;
    } else {
        for (size_t i = 0; i <= lineText.size(); ++i) {
            const float width = MeasureTextPrefixWidth(nativeUi, lineText, i, kOutputTextScale);
            if (xRelative <= width) {
                column = i;
                break;
            }
        }
    }

    *outLine   = snapshot.startLine + localLine;
    *outColumn = column;
    return true;
}

std::string DeveloperConsoleLayer::BuildSelectedOutputText() const {
    if (!HasOutputSelection()) {
        return {};
    }

    auto& console  = ConsoleSystem::Get();
    auto& nativeUi = ui::NativeUiRenderer::Get();
    if (!nativeUi.IsInitialized()) {
        return {};
    }

    int startLine = 0;
    int endLine   = 0;
    size_t startCol = 0;
    size_t endCol   = 0;
    NormalizeSelection(selectionAnchorLine_, selectionAnchorColumn_, selectionCaretLine_,
                       selectionCaretColumn_, &startLine, &startCol, &endLine, &endCol);

    const int totalLines = static_cast<int>(console.GetOutputLineCount());
    if (totalLines <= 0) {
        return {};
    }

    startLine = std::clamp(startLine, 0, totalLines - 1);
    endLine   = std::clamp(endLine, 0, totalLines - 1);
    if (startLine > endLine) {
        std::swap(startLine, endLine);
        std::swap(startCol, endCol);
    }

    const size_t lineCount = static_cast<size_t>(endLine - startLine + 1);
    const auto sourceLines =
        console.GetOutputLinesRangeSnapshot(static_cast<size_t>(startLine), lineCount);
    if (sourceLines.empty()) {
        return {};
    }

    const UiRect outputRect = GetOutputRect();
    const float maxTextWidth = std::max(0.0f, outputRect.w - 14.0f);

    std::string selected;
    for (size_t i = 0; i < sourceLines.size(); ++i) {
        const int absoluteLine = startLine + static_cast<int>(i);
        const std::string renderedLine =
            TruncateWithEllipsis(nativeUi, sourceLines[i], maxTextWidth, kOutputTextScale);

        size_t lineStart = 0;
        size_t lineEnd   = renderedLine.size();
        if (absoluteLine == startLine) {
            lineStart = std::min(startCol, renderedLine.size());
        }
        if (absoluteLine == endLine) {
            lineEnd = std::min(endCol, renderedLine.size());
        }

        if (lineStart < lineEnd) {
            selected.append(renderedLine.substr(lineStart, lineEnd - lineStart));
        }
        if (absoluteLine < endLine) {
            selected.push_back('\n');
        }
    }

    return selected;
}

void DeveloperConsoleLayer::CopyToClipboard(const std::string& text) {
    if (text.empty()) {
        return;
    }

    WindowHandle window = Application::Get().GetWindow().GetNativeWindow();
    if (!window) {
        return;
    }
    glfwSetClipboardString(window, text.c_str());
}

bool DeveloperConsoleLayer::PasteFromClipboard() {
    WindowHandle window = Application::Get().GetWindow().GetNativeWindow();
    if (!window) {
        return false;
    }

    const char* clipboard = glfwGetClipboardString(window);
    if (!clipboard || *clipboard == '\0') {
        return false;
    }

    bool pastedAny = false;
    caretIndex_ = std::min(caretIndex_, inputBuffer_.size());
    for (const char* it = clipboard; *it != '\0'; ++it) {
        const unsigned char ch = static_cast<unsigned char>(*it);
        if (ch == '\r' || ch == '\n') {
            continue;
        }
        if (ch < 32u || ch > 126u) {
            continue;
        }
        if (inputBuffer_.size() >= 512) {
            break;
        }
        inputBuffer_.insert(caretIndex_, 1, static_cast<char>(ch));
        ++caretIndex_;
        historyIndex_ = -1;
        pastedAny = true;
    }

    return pastedAny;
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
