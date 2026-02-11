#pragma once

#include "engine/ui/native/UIControl.h"

#include <string>

namespace se::ui {

/**
 * @class UILabel
 * @brief Text display widget
 */
class UILabel : public UIControl {
public:
    UILabel();
    explicit UILabel(const std::string& text);
    ~UILabel() override = default;
    
    // Text
    void SetText(const std::string& text);
    const std::string& GetText() const { return text_; }
    
    // Alignment
    enum class HorizontalAlign : uint8_t { LEFT, CENTER, RIGHT };
    enum class VerticalAlign : uint8_t { TOP, CENTER, BOTTOM };
    
    void SetHorizontalAlignment(HorizontalAlign align) { hAlign_ = align; QueueRedraw(); }
    void SetVerticalAlignment(VerticalAlign align) { vAlign_ = align; QueueRedraw(); }
    HorizontalAlign GetHorizontalAlignment() const { return hAlign_; }
    VerticalAlign GetVerticalAlignment() const { return vAlign_; }
    
    // Font
    void SetFontSize(float size) { fontSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    float GetFontSize() const { return fontSize_; }
    
    // Color
    void SetFontColor(const glm::vec4& color) { fontColor_ = color; QueueRedraw(); }
    glm::vec4 GetFontColor() const { return fontColor_; }
    
    // Autowrap
    void SetAutowrap(bool enabled) { autowrap_ = enabled; UpdateMinimumSize(); }
    bool GetAutowrap() const { return autowrap_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    
private:
    std::string text_;
    HorizontalAlign hAlign_ = HorizontalAlign::LEFT;
    VerticalAlign vAlign_ = VerticalAlign::TOP;
    float fontSize_ = 16.0f;
    glm::vec4 fontColor_{1.0f, 1.0f, 1.0f, 1.0f};
    bool autowrap_ = false;
};

}  // namespace se::ui
