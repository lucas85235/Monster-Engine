#include "engine/perf/PerfOverlay.h"

#include "engine/ui/native/widgets/UILabel.h"
#include "engine/ui/native/UISystem.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <cstdio>

namespace se::perf {

// Static member definitions
std::unique_ptr<ui::UILabel> PerfOverlay::label_ = nullptr;
bool PerfOverlay::initialized_ = false;
bool PerfOverlay::visible_ = true;
float PerfOverlay::updateInterval_ = 0.5f;
float PerfOverlay::accumulatedTime_ = 0.0f;
float PerfOverlay::accumulatedFrames_ = 0.0f;
float PerfOverlay::lastFps_ = 0.0f;
float PerfOverlay::lastFrameTime_ = 0.0f;
float PerfOverlay::offsetX_ = 10.0f;
float PerfOverlay::offsetY_ = 10.0f;
float PerfOverlay::fontSize_ = 16.0f;

void PerfOverlay::Initialize(float fontSize) {
    if (initialized_) return;
    
    fontSize_ = fontSize;
    
    label_ = std::make_unique<ui::UILabel>("FPS: --");
    label_->SetFontSize(fontSize_);
    label_->SetFontColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));  // Green
    label_->SetHorizontalAlignment(ui::UILabel::HorizontalAlign::RIGHT);
    label_->SetVerticalAlignment(ui::UILabel::VerticalAlign::TOP);
    
    // Position in top-right corner
    UpdateLabelPosition();
    
    initialized_ = true;
}

void PerfOverlay::Shutdown() {
    if (!initialized_) return;
    
    label_.reset();
    initialized_ = false;
}

void PerfOverlay::Update(float deltaTime) {
    // Auto-initialize if not yet initialized and viewport is valid
    if (!initialized_) {
        glm::vec2 viewportSize = ui::GetViewportSize();
        if (viewportSize.x > 0 && viewportSize.y > 0) {
            Initialize();
        } else {
            return;  // Can't initialize yet
        }
    }
    
    if (!label_) return;
    
    // Accumulate frame data
    accumulatedTime_ += deltaTime;
    accumulatedFrames_ += 1.0f;
    lastFrameTime_ = deltaTime * 1000.0f;  // Convert to milliseconds
    
    // Update displayed FPS at the configured interval
    if (accumulatedTime_ >= updateInterval_) {
        lastFps_ = accumulatedFrames_ / accumulatedTime_;
        accumulatedTime_ = 0.0f;
        accumulatedFrames_ = 0.0f;
        
        // Update label text
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.1f FPS | %.2f ms", lastFps_, lastFrameTime_);
        label_->SetText(buffer);
    }
    
    // Update position in case viewport changed
    UpdateLabelPosition();
    
    // Update visibility
    label_->SetVisible(visible_);
}

void PerfOverlay::Render() {
    if (!initialized_ || !label_ || !visible_) return;
    
    glm::vec2 viewportSize = ui::GetViewportSize();
    if (viewportSize.x <= 0 || viewportSize.y <= 0) return;
    
    // Use the canvas directly with proper frame management
    auto& canvas = ui::UICanvas2D::Get();
    canvas.SetViewport(viewportSize.x, viewportSize.y);
    
    // Begin frame, draw, end frame, render  
    canvas.BeginFrame();
    label_->Draw();
    canvas.EndFrame();
    canvas.Render();
}

bool PerfOverlay::IsVisible() {
    return visible_;
}

void PerfOverlay::SetVisible(bool visible) {
    visible_ = visible;
    if (label_) {
        label_->SetVisible(visible_);
    }
}

void PerfOverlay::ToggleVisible() {
    SetVisible(!visible_);
}

void PerfOverlay::SetUpdateInterval(float interval) {
    updateInterval_ = interval > 0.0f ? interval : 0.5f;
}

void PerfOverlay::SetPositionOffset(float x, float y) {
    offsetX_ = x;
    offsetY_ = y;
    UpdateLabelPosition();
}

void PerfOverlay::UpdateLabelPosition() {
    if (!label_) return;
    
    glm::vec2 viewportSize = ui::GetViewportSize();
    if (viewportSize.x <= 0 || viewportSize.y <= 0) return;
    
    // Calculate width needed for the label (approximate)
    float labelWidth = 180.0f;  // Approximate width for "999.9 FPS | 99.99 ms"
    float labelHeight = fontSize_ + 8.0f;
    
    // Position in top-right corner with offset
    float x = viewportSize.x - labelWidth - offsetX_;
    float y = offsetY_;
    
    label_->SetPosition(glm::vec2(x, y));
    label_->SetSize(glm::vec2(labelWidth, labelHeight));
}

}  // namespace se::perf
