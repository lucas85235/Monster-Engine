#include "engine/ui/native/theme/UITheme.h"
#include "engine/Log.h"

namespace se::ui {

// ─────────────────────────────────────────────────────────
// UITheme
// ─────────────────────────────────────────────────────────

UITheme::UITheme() {
    SE_LOG_DEBUG("UITheme created");
}

UITheme::~UITheme() {
    SE_LOG_DEBUG("UITheme destroyed");
}

// Helper template implementation
template<typename MapType, typename ValueType>
ValueType UITheme::GetWithVariation(const MapType& map, const std::string& type, 
                                    const std::string& name, const ValueType& defaultVal) const {
    // Try exact type first
    auto typeIt = map.find(type);
    if (typeIt != map.end()) {
        auto itemIt = typeIt->second.find(name);
        if (itemIt != typeIt->second.end()) {
            return itemIt->second;
        }
    }
    
    // Try variation base type(s)
    std::string currentType = type;
    int maxDepth = 10;  // Prevent infinite loops
    while (maxDepth-- > 0) {
        auto varIt = variations_.find(currentType);
        if (varIt == variations_.end()) break;
        
        currentType = varIt->second;
        typeIt = map.find(currentType);
        if (typeIt != map.end()) {
            auto itemIt = typeIt->second.find(name);
            if (itemIt != typeIt->second.end()) {
                return itemIt->second;
            }
        }
    }
    
    return defaultVal;
}

// ─────────────────────────────────────────────────────────
// Colors
// ─────────────────────────────────────────────────────────

void UITheme::SetColor(const std::string& controlType, const std::string& name, const glm::vec4& color) {
    colors_[controlType][name] = color;
}

glm::vec4 UITheme::GetColor(const std::string& controlType, const std::string& name) const {
    return GetWithVariation(colors_, controlType, name, glm::vec4(1.0f));
}

bool UITheme::HasColor(const std::string& controlType, const std::string& name) const {
    auto typeIt = colors_.find(controlType);
    if (typeIt != colors_.end()) {
        return typeIt->second.find(name) != typeIt->second.end();
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────

void UITheme::SetConstant(const std::string& controlType, const std::string& name, int value) {
    constants_[controlType][name] = value;
}

int UITheme::GetConstant(const std::string& controlType, const std::string& name) const {
    return GetWithVariation(constants_, controlType, name, 0);
}

bool UITheme::HasConstant(const std::string& controlType, const std::string& name) const {
    auto typeIt = constants_.find(controlType);
    if (typeIt != constants_.end()) {
        return typeIt->second.find(name) != typeIt->second.end();
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// Font Sizes
// ─────────────────────────────────────────────────────────

void UITheme::SetFontSize(const std::string& controlType, const std::string& name, int size) {
    fontSizes_[controlType][name] = size;
}

int UITheme::GetFontSize(const std::string& controlType, const std::string& name) const {
    int result = GetWithVariation(fontSizes_, controlType, name, -1);
    return result >= 0 ? result : defaultFontSize_;
}

bool UITheme::HasFontSize(const std::string& controlType, const std::string& name) const {
    auto typeIt = fontSizes_.find(controlType);
    if (typeIt != fontSizes_.end()) {
        return typeIt->second.find(name) != typeIt->second.end();
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// StyleBoxes
// ─────────────────────────────────────────────────────────

void UITheme::SetStyleBox(const std::string& controlType, const std::string& name, UIStyleBox::Ptr style) {
    styleBoxes_[controlType][name] = std::move(style);
}

UIStyleBox::Ptr UITheme::GetStyleBox(const std::string& controlType, const std::string& name) const {
    return GetWithVariation(styleBoxes_, controlType, name, UIStyleBox::Ptr{});
}

bool UITheme::HasStyleBox(const std::string& controlType, const std::string& name) const {
    auto typeIt = styleBoxes_.find(controlType);
    if (typeIt != styleBoxes_.end()) {
        return typeIt->second.find(name) != typeIt->second.end();
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// Icons
// ─────────────────────────────────────────────────────────

void UITheme::SetIcon(const std::string& controlType, const std::string& name, uint32_t textureId) {
    icons_[controlType][name] = textureId;
}

uint32_t UITheme::GetIcon(const std::string& controlType, const std::string& name) const {
    return GetWithVariation(icons_, controlType, name, 0u);
}

bool UITheme::HasIcon(const std::string& controlType, const std::string& name) const {
    auto typeIt = icons_.find(controlType);
    if (typeIt != icons_.end()) {
        return typeIt->second.find(name) != typeIt->second.end();
    }
    return false;
}

// ─────────────────────────────────────────────────────────
// Type Variations
// ─────────────────────────────────────────────────────────

void UITheme::SetTypeVariation(const std::string& variation, const std::string& base) {
    variations_[variation] = base;
}

std::string UITheme::GetTypeVariation(const std::string& variation) const {
    auto it = variations_.find(variation);
    return it != variations_.end() ? it->second : "";
}

// ─────────────────────────────────────────────────────────
// UIThemeDB Singleton
// ─────────────────────────────────────────────────────────

UIThemeDB& UIThemeDB::Get() {
    static UIThemeDB instance;
    return instance;
}

UIThemeDB::UIThemeDB() {
    // Create a minimal fallback stylebox
    fallbackStyleBox_ = std::make_shared<UIStyleBoxFlat>();
    
    SE_LOG_INFO("UIThemeDB initialized");
}

}  // namespace se::ui
