#include "engine/ui/native/UISystem.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/Application.h"
#include "engine/input/InputManager.h"
#include "engine/Log.h"
#include "engine/debug/FrameProfiler.h"

namespace {
    // Helper to get current viewport from window
    glm::vec2 GetCurrentViewportSize() {
        auto& window = se::Application::Get().GetWindow();
        return {static_cast<float>(window.GetWidth()), static_cast<float>(window.GetHeight())};
    }
}

namespace se::ui {

namespace {
    UIControl::Ptr rootControl_;
    glm::vec2 viewportSize_{1920.0f, 1080.0f};
    glm::vec2 lastViewportSize_{1920.0f, 1080.0f};
    UIControl* focusedControl_ = nullptr;
    
    // Input tracking state
    glm::vec2 lastMousePos_{0.0f};
    bool lastMousePressed_ = false;
    UIControl* hoveredControl_ = nullptr;
    bool mouseWasCaptured_ = false;
    
    // Resize callback
    std::function<void(float, float)> onResizeCallback_;
}

void Initialize(float viewportWidth, float viewportHeight) {
    viewportSize_ = {viewportWidth, viewportHeight};
    
    // Initialize font manager FIRST
    auto& fontMgr = UIFontManager::Get();
    if (!fontMgr.Initialize()) {
        SE_LOG_ERROR("[UISystem] Failed to initialize font manager!");
    }
    
    // Initialize theme database
    auto& themeDB = UIThemeDB::Get();
    
    // Create a default theme with basic styling
    auto defaultTheme = std::make_shared<UITheme>();
    
    // Set default button styles
    auto buttonNormal = std::make_shared<UIStyleBoxFlat>();
    buttonNormal->SetBackgroundColor({0.3f, 0.3f, 0.35f, 1.0f});
    buttonNormal->SetBorderColor({0.5f, 0.5f, 0.55f, 1.0f});
    buttonNormal->SetBorderWidthAll(2.0f);
    buttonNormal->SetCornerRadiusAll(40.0f);
    buttonNormal->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "normal", buttonNormal);
    
    auto buttonHover = std::make_shared<UIStyleBoxFlat>();
    buttonHover->SetBackgroundColor({0.4f, 0.4f, 0.45f, 1.0f});
    buttonHover->SetBorderColor({0.6f, 0.6f, 0.65f, 1.0f});
    buttonHover->SetBorderWidthAll(2.0f);
    buttonHover->SetCornerRadiusAll(40.0f);
    buttonHover->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "hover", buttonHover);
    
    auto buttonPressed = std::make_shared<UIStyleBoxFlat>();
    buttonPressed->SetBackgroundColor({0.2f, 0.4f, 0.6f, 1.0f});
    buttonPressed->SetBorderColor({0.3f, 0.5f, 0.7f, 1.0f});
    buttonPressed->SetBorderWidthAll(2.0f);
    buttonPressed->SetCornerRadiusAll(40.0f);
    buttonPressed->SetContentMarginAll(8.0f);
    defaultTheme->SetStyleBox("Button", "pressed", buttonPressed);
    
    // Set default panel style
    auto panelStyle = std::make_shared<UIStyleBoxFlat>();
    panelStyle->SetBackgroundColor({0.15f, 0.15f, 0.18f, 1.0f});
    panelStyle->SetBorderColor({0.25f, 0.25f, 0.28f, 1.0f});
    panelStyle->SetBorderWidthAll(2.0f);
    defaultTheme->SetStyleBox("Panel", "panel", panelStyle);
    
    // Set default colors
    defaultTheme->SetColor("Label", "font_color", {0.9f, 0.9f, 0.9f, 1.0f});
    defaultTheme->SetColor("Button", "font_color", {1.0f, 1.0f, 1.0f, 1.0f});
    
    // Set default constants
    defaultTheme->SetConstant("BoxContainer", "separation", 4);
    
    themeDB.SetDefaultTheme(defaultTheme);
    
    SE_LOG_INFO("Native UI System initialized ({}x{})", viewportWidth, viewportHeight);
}

void Shutdown() {
    rootControl_.reset();
    focusedControl_ = nullptr;
    
    // Shutdown font manager
    UIFontManager::Get().Shutdown();
    
    SE_LOG_INFO("Native UI System shutdown");
}

void Update() {
    // Auto-detect viewport resize
    glm::vec2 currentViewport = GetCurrentViewportSize();
    if (currentViewport != lastViewportSize_) {
        lastViewportSize_ = currentViewport;
        Resize(currentViewport.x, currentViewport.y);
        
        // Fire callback if registered
        if (onResizeCallback_) {
            onResizeCallback_(currentViewport.x, currentViewport.y);
        }
    }
    
    if (!rootControl_) return;
    
    // Process deferred layout updates
    // Walk through all containers and sort if pending
    std::function<void(UIControl*)> processContainer = [&](UIControl* control) {
        if (auto* container = dynamic_cast<UIContainer*>(control)) {
            if (container->IsSortPending()) {
                container->SortChildren();
            }
        }
        
        for (const auto& child : control->GetChildren()) {
            processContainer(child.get());
        }
    };
    
    processContainer(rootControl_.get());
}

void Render() {
    SE_PROFILE_SCOPE_COLOR("NativeUI", ProfilerColors::Carrot);
    if (!rootControl_) return;
    
    // Update viewport size dynamically from window
    viewportSize_ = GetCurrentViewportSize();
    
    auto& canvas = UICanvas2D::Get();
    canvas.SetViewport(viewportSize_.x, viewportSize_.y);
    
    // Check if any control needs redraw
    std::function<bool(UIControl*)> checkNeedsRedraw = [&](UIControl* ctrl) -> bool {
        if (!ctrl || !ctrl->IsVisible()) return false;
        if (ctrl->NeedsRedraw()) return true;
        for (const auto& child : ctrl->GetChildren()) {
            if (checkNeedsRedraw(child.get())) return true;
        }
        return false;
    };
    
    bool anyNeedsRedraw = checkNeedsRedraw(rootControl_.get());
    
    // Regenerate commands only if something changed
    if (anyNeedsRedraw || !canvas.HasCachedCommands()) {
        SE_PROFILE_SCOPE_COLOR("NativeUI::BuildCommands", ProfilerColors::Alizarin);
        canvas.BeginFrame();
        
        std::function<void(UIControl*)> drawControl = [&](UIControl* ctrl) {
            if (!ctrl || !ctrl->IsVisible()) return;
            ctrl->Draw();
            ctrl->ClearRedrawFlag();
            for (const auto& child : ctrl->GetChildren()) {
                drawControl(child.get());
            }
        };
        
        drawControl(rootControl_.get());
        canvas.EndFrame();
    }
    
    {
        SE_PROFILE_SCOPE_COLOR("NativeUI::Render", ProfilerColors::Amethyst);
        canvas.Render();
    }
}

void SetRoot(UIControl::Ptr root) {
    rootControl_ = std::move(root);
    // Note: we don't modify the root's anchors/size here.
    // The caller is responsible for positioning.
    // viewportSize_ is used for hit testing bounds.
}

UIControl* GetRoot() {
    return rootControl_.get();
}

bool ProcessInput(const InputEvent& inputEvent) {
    if (!rootControl_) return false;
    
    // For mouse events, find the control under the cursor
    if (inputEvent.IsMouseMotion() || inputEvent.IsMouseButton()) {
        // Transform mouse position to local space and test
        std::function<UIControl*(UIControl*, const glm::vec2&)> findControl = 
            [&](UIControl* control, const glm::vec2& pos) -> UIControl* {
            
            if (!control->IsVisible()) return nullptr;
            if (control->GetMouseFilter() == MouseFilter::MOUSE_IGNORE) return nullptr;
            
            // Transform to local space
            glm::vec2 localPos = pos - control->GetPosition();
            
            // Check children first (front to back, last child is on top)
            const auto& children = control->GetChildren();
            for (auto it = children.rbegin(); it != children.rend(); ++it) {
                if (UIControl* found = findControl(it->get(), localPos)) {
                    return found;
                }
            }
            
            // Check self
            if (control->HasPoint(localPos)) {
                return control;
            }
            
            return nullptr;
        };
        
        UIControl* target = findControl(rootControl_.get(), inputEvent.mousePosition);
        
        if (target) {
            target->OnInput(inputEvent);
            
            if (target->GetMouseFilter() == MouseFilter::MOUSE_STOP) {
                inputEvent.Accept();
            }
        }
    }
    
    // For keyboard events, send to focused control
    if (inputEvent.IsKey() && focusedControl_) {
        focusedControl_->OnInput(inputEvent);
    }
    
    return inputEvent.IsConsumed();
}

void PollInput() {
    if (!rootControl_) return;
    
    auto& input = se::InputManager::Get();
    glm::vec2 mousePos = input.GetMousePosition();
    bool mousePressed = input.IsMouseButtonDown(0);
    glm::vec2 mouseDelta = mousePos - lastMousePos_;
    
    // Hit test to find control under cursor
    std::function<UIControl*(UIControl*, const glm::vec2&)> hitTest = 
        [&hitTest](UIControl* ctrl, const glm::vec2& pos) -> UIControl* {
        if (!ctrl || !ctrl->IsVisible()) return nullptr;
        
        // Check children first (reverse order for front-to-back)
        const auto& children = ctrl->GetChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (auto* hit = hitTest(it->get(), pos)) {
                return hit;
            }
        }
        
        // Check this control
        glm::vec2 globalPos = ctrl->GetGlobalPosition();
        glm::vec2 size = ctrl->GetSize();
        if (pos.x >= globalPos.x && pos.x < globalPos.x + size.x &&
            pos.y >= globalPos.y && pos.y < globalPos.y + size.y) {
            if (ctrl->GetMouseFilter() != MouseFilter::MOUSE_IGNORE) {
                return ctrl;
            }
        }
        return nullptr;
    };
    
    UIControl* newHovered = hitTest(rootControl_.get(), mousePos);
    
    // Handle hover state changes
    if (newHovered != hoveredControl_) {
        if (hoveredControl_) {
            hoveredControl_->OnNotification(ControlNotification::MOUSE_EXIT);
        }
        if (newHovered) {
            newHovered->OnNotification(ControlNotification::MOUSE_ENTER);
        }
        hoveredControl_ = newHovered;
    }
    
    mouseWasCaptured_ = false;
    
    // Generate and dispatch mouse motion events
    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
        auto motionEvent = InputEvent::MouseMotion(mousePos, mouseDelta);
        if (hoveredControl_) {
            hoveredControl_->OnInput(motionEvent);
            if (motionEvent.IsConsumed()) mouseWasCaptured_ = true;
        }
    }
    
    // Mouse button state changes
    if (mousePressed && !lastMousePressed_) {
        auto pressEvent = InputEvent::MouseButtonPressed(MouseButton::LEFT, mousePos);
        if (hoveredControl_) {
            hoveredControl_->OnInput(pressEvent);
            if (pressEvent.IsConsumed()) mouseWasCaptured_ = true;
        }
    } else if (!mousePressed && lastMousePressed_) {
        auto releaseEvent = InputEvent::MouseButtonReleased(MouseButton::LEFT, mousePos);
        if (hoveredControl_) {
            hoveredControl_->OnInput(releaseEvent);
            if (releaseEvent.IsConsumed()) mouseWasCaptured_ = true;
        }
    }
    
    lastMousePos_ = mousePos;
    lastMousePressed_ = mousePressed;
}

void Resize(float viewportWidth, float viewportHeight) {
    viewportSize_ = {viewportWidth, viewportHeight};
    
    if (rootControl_) {
        rootControl_->SetSize(viewportSize_);
    }
    
    UICanvas2D::Get().SetViewport(viewportWidth, viewportHeight);
}

UIControl* GetHoveredControl() {
    return hoveredControl_;
}

bool WantsMouseCapture() {
    return mouseWasCaptured_ || hoveredControl_ != nullptr;
}

glm::vec2 GetViewportSize() {
    return viewportSize_;
}

void SetOnResizeCallback(std::function<void(float, float)> callback) {
    onResizeCallback_ = std::move(callback);
}

}  // namespace se::ui
