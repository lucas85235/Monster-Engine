#pragma once

#include "engine/ui/native/theme/UIStyleBox.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace se::ui {

/**
 * @class UITheme
 * @brief Theme resource for consistent UI styling (equivalent to Godot's Theme).
 *
 * UITheme provides:
 * - Per-control-type style definitions
 * - Hierarchical lookup (local → parent → default)
 * - Style variations (e.g., "ButtonDanger" inherits from "Button")
 */
class UITheme {
public:
    using Ptr = std::shared_ptr<UITheme>;

    UITheme();
    ~UITheme();

    // ─────────────────────────────────────────────────────────
    // Data Types
    // ─────────────────────────────────────────────────────────

    enum class DataType {
        COLOR,
        CONSTANT,
        FONT,
        FONT_SIZE,
        ICON,
        STYLEBOX
    };

    // ─────────────────────────────────────────────────────────
    // Colors
    // ─────────────────────────────────────────────────────────

    void SetColor(const std::string& controlType, const std::string& name, const glm::vec4& color);
    glm::vec4 GetColor(const std::string& controlType, const std::string& name) const;
    bool HasColor(const std::string& controlType, const std::string& name) const;

    // ─────────────────────────────────────────────────────────
    // Constants (integers)
    // ─────────────────────────────────────────────────────────

    void SetConstant(const std::string& controlType, const std::string& name, int value);
    int GetConstant(const std::string& controlType, const std::string& name) const;
    bool HasConstant(const std::string& controlType, const std::string& name) const;

    // ─────────────────────────────────────────────────────────
    // Font Sizes
    // ─────────────────────────────────────────────────────────

    void SetFontSize(const std::string& controlType, const std::string& name, int size);
    int GetFontSize(const std::string& controlType, const std::string& name) const;
    bool HasFontSize(const std::string& controlType, const std::string& name) const;

    // ─────────────────────────────────────────────────────────
    // StyleBoxes
    // ─────────────────────────────────────────────────────────

    void SetStyleBox(const std::string& controlType, const std::string& name, UIStyleBox::Ptr style);
    UIStyleBox::Ptr GetStyleBox(const std::string& controlType, const std::string& name) const;
    bool HasStyleBox(const std::string& controlType, const std::string& name) const;

    // ─────────────────────────────────────────────────────────
    // Icons (texture IDs)
    // ─────────────────────────────────────────────────────────

    void SetIcon(const std::string& controlType, const std::string& name, uint32_t textureId);
    uint32_t GetIcon(const std::string& controlType, const std::string& name) const;
    bool HasIcon(const std::string& controlType, const std::string& name) const;

    // ─────────────────────────────────────────────────────────
    // Type Variations
    // ─────────────────────────────────────────────────────────

    /// Set that 'variation' inherits from 'base' (e.g., "ButtonRed" → "Button")
    void SetTypeVariation(const std::string& variation, const std::string& base);
    std::string GetTypeVariation(const std::string& variation) const;

    // ─────────────────────────────────────────────────────────
    // Defaults
    // ─────────────────────────────────────────────────────────

    void SetDefaultFontSize(int size) { defaultFontSize_ = size; }
    int GetDefaultFontSize() const { return defaultFontSize_; }

private:
    // Nested maps: [controlType][itemName] = value
    using ColorMap = std::unordered_map<std::string, std::unordered_map<std::string, glm::vec4>>;
    using ConstantMap = std::unordered_map<std::string, std::unordered_map<std::string, int>>;
    using StyleBoxMap = std::unordered_map<std::string, std::unordered_map<std::string, UIStyleBox::Ptr>>;
    using IconMap = std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>;
    using VariationMap = std::unordered_map<std::string, std::string>;

    ColorMap colors_;
    ConstantMap constants_;
    ConstantMap fontSizes_;
    StyleBoxMap styleBoxes_;
    IconMap icons_;
    VariationMap variations_;

    int defaultFontSize_ = 14;

    // Helper to get with variation fallback
    template<typename MapType, typename ValueType>
    ValueType GetWithVariation(const MapType& map, const std::string& type, 
                               const std::string& name, const ValueType& defaultVal) const;
};

/**
 * @class UIThemeDB
 * @brief Singleton for managing default and project themes.
 */
class UIThemeDB {
public:
    static UIThemeDB& Get();

    void SetDefaultTheme(UITheme::Ptr theme) { defaultTheme_ = std::move(theme); }
    UITheme::Ptr GetDefaultTheme() const { return defaultTheme_; }

    void SetProjectTheme(UITheme::Ptr theme) { projectTheme_ = std::move(theme); }
    UITheme::Ptr GetProjectTheme() const { return projectTheme_; }

    // Fallback values (used when no theme provides a value)
    void SetFallbackFontSize(int size) { fallbackFontSize_ = size; }
    int GetFallbackFontSize() const { return fallbackFontSize_; }

    void SetFallbackColor(const glm::vec4& color) { fallbackColor_ = color; }
    glm::vec4 GetFallbackColor() const { return fallbackColor_; }

    UIStyleBox::Ptr GetFallbackStyleBox() const { return fallbackStyleBox_; }

private:
    UIThemeDB();
    ~UIThemeDB() = default;

    UITheme::Ptr defaultTheme_;
    UITheme::Ptr projectTheme_;

    int fallbackFontSize_ = 14;
    glm::vec4 fallbackColor_{1.0f, 1.0f, 1.0f, 1.0f};
    UIStyleBox::Ptr fallbackStyleBox_;
};

}  // namespace se::ui
