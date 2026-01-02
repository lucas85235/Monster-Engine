#pragma once

#include "engine/ui/native/UIControl.h"

#include <functional>
#include <string>

namespace se::ui {

class UIStyleBox;

/**
 * @class UIAbilitySlot
 * @brief Ability slot widget for HUD - displays an ability icon with keybind.
 *
 * Features:
 * - Rounded square container
 * - Icon display area
 * - Key binding label at bottom
 * - Cooldown overlay display
 * - Active/highlight state
 */
class UIAbilitySlot : public UIControl {
public:
    using OnClickCallback = std::function<void()>;

    UIAbilitySlot();
    explicit UIAbilitySlot(const std::string& keyLabel);
    ~UIAbilitySlot() override = default;

    // Key binding
    void SetKeyLabel(const std::string& label) { keyLabel_ = label; QueueRedraw(); }
    const std::string& GetKeyLabel() const { return keyLabel_; }

    // Icon
    void SetIconTexture(uint32_t textureId) { iconTexture_ = textureId; QueueRedraw(); }
    uint32_t GetIconTexture() const { return iconTexture_; }

    void SetIconColor(const glm::vec4& color) { iconColor_ = color; QueueRedraw(); }
    glm::vec4 GetIconColor() const { return iconColor_; }

    // Placeholder icon (when no texture set)
    void SetPlaceholderChar(char c) { placeholderChar_ = c; QueueRedraw(); }
    char GetPlaceholderChar() const { return placeholderChar_; }

    // Cooldown
    void SetCooldownPercent(float percent);
    float GetCooldownPercent() const { return cooldownPercent_; }

    void SetCooldownColor(const glm::vec4& color) { cooldownColor_ = color; QueueRedraw(); }
    glm::vec4 GetCooldownColor() const { return cooldownColor_; }

    // States
    void SetActive(bool active) { isActive_ = active; QueueRedraw(); }
    bool IsActive() const { return isActive_; }

    void SetEnabled(bool enabled) { isEnabled_ = enabled; QueueRedraw(); }
    bool IsEnabled() const { return isEnabled_; }

    // Styling
    void SetStyleBoxBackground(std::shared_ptr<UIStyleBox> style) { styleBackground_ = style; QueueRedraw(); }
    void SetStyleBoxActive(std::shared_ptr<UIStyleBox> style) { styleActive_ = style; QueueRedraw(); }

    // Font
    void SetKeyLabelFontSize(float size) { keyLabelFontSize_ = size; QueueRedraw(); }
    float GetKeyLabelFontSize() const { return keyLabelFontSize_; }

    void SetKeyLabelColor(const glm::vec4& color) { keyLabelColor_ = color; QueueRedraw(); }
    glm::vec4 GetKeyLabelColor() const { return keyLabelColor_; }

    // Callbacks
    void SetOnClick(OnClickCallback callback) { onClick_ = std::move(callback); }

    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    void OnInput(const InputEvent& inputEvent) override;
    void OnNotification(ControlNotification notification) override;

private:
    void DrawCooldownOverlay(const glm::vec2& pos, const glm::vec2& size);
    void DrawKeyLabel(const glm::vec2& pos, const glm::vec2& size);

private:
    std::string keyLabel_;
    uint32_t iconTexture_ = 0;
    glm::vec4 iconColor_{1.0f, 1.0f, 1.0f, 1.0f};
    char placeholderChar_ = '?';

    float cooldownPercent_ = 0.0f;
    glm::vec4 cooldownColor_{0.0f, 0.0f, 0.0f, 0.6f};

    bool isActive_ = false;
    bool isEnabled_ = true;
    bool isHovered_ = false;
    bool isPressed_ = false;

    float keyLabelFontSize_ = 12.0f;
    glm::vec4 keyLabelColor_{1.0f, 1.0f, 1.0f, 0.9f};

    std::shared_ptr<UIStyleBox> styleBackground_;
    std::shared_ptr<UIStyleBox> styleActive_;

    OnClickCallback onClick_;
};

}  // namespace se::ui
