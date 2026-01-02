#include "engine/ui/native/hud/HUDController.h"
#include "engine/ui/native/widgets/hud/UICrosshair.h"
#include "engine/ui/native/widgets/hud/UIHealthBar.h"
#include "engine/ui/native/widgets/hud/UIAbilitySlot.h"
#include "engine/ui/native/widgets/UIPanel.h"
#include "engine/ui/native/UIBoxContainer.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/Log.h"

namespace se::ui {

HUDController::HUDController() = default;

HUDController::~HUDController() {
    Shutdown();
}

void HUDController::Initialize(float viewportWidth, float viewportHeight) {
    if (initialized_) {
        SE_LOG_WARN("[HUDController] Already initialized");
        return;
    }

    viewportWidth_ = viewportWidth;
    viewportHeight_ = viewportHeight;

    // Create transparent root container
    auto rootPanel = std::make_unique<UIPanel>();
    rootPanel->SetName("HUD Root");
    rootPanel->SetPosition({0.0f, 0.0f});
    rootPanel->SetSize({viewportWidth_, viewportHeight_});
    rootPanel->SetMouseFilter(MouseFilter::MOUSE_IGNORE);

    // Make root transparent
    auto transparentStyle = std::make_shared<UIStyleBoxFlat>();
    transparentStyle->SetBackgroundColor({0.0f, 0.0f, 0.0f, 0.0f});
    rootPanel->SetStyleBox(transparentStyle);

    root_ = std::move(rootPanel);

    CreateCrosshair();
    CreateHealthBar();
    CreateAbilitySlots();

    initialized_ = true;
    SE_LOG_INFO("[HUDController] Initialized with viewport {}x{}", viewportWidth_, viewportHeight_);
}

void HUDController::Shutdown() {
    if (!initialized_) return;

    crosshair_ = nullptr;
    healthBar_ = nullptr;
    abilitySlots_.clear();
    root_.reset();

    initialized_ = false;
    SE_LOG_INFO("[HUDController] Shutdown complete");
}

UIControl::Ptr HUDController::GetRoot() {
    return std::move(root_);
}

void HUDController::OnViewportResize(float width, float height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    UpdateLayout();
}

void HUDController::SetVisible(bool visible) {
    if (root_) {
        root_->SetVisible(visible);
    }
}

bool HUDController::IsVisible() const {
    return root_ ? root_->IsVisible() : false;
}

void HUDController::SetCrosshairVisible(bool visible) {
    if (crosshair_) {
        crosshair_->SetVisible(visible);
    }
}

void HUDController::SetHealthBarVisible(bool visible) {
    if (healthBar_) {
        healthBar_->SetVisible(visible);
    }
}

void HUDController::SetAbilitySlotsVisible(bool visible) {
    for (auto* slot : abilitySlots_) {
        if (slot) slot->SetVisible(visible);
    }
}

void HUDController::SetHealth(float current, float max) {
    if (healthBar_) {
        healthBar_->SetHealthRange(current, max);
    }
}

void HUDController::SetHealth(float current) {
    if (healthBar_) {
        healthBar_->SetHealth(current);
    }
}

float HUDController::GetHealth() const {
    return healthBar_ ? healthBar_->GetHealth() : 0.0f;
}

float HUDController::GetMaxHealth() const {
    return healthBar_ ? healthBar_->GetMaxHealth() : 0.0f;
}

void HUDController::SetAbilitySlotCount(size_t count) {
    abilitySlotCount_ = std::min(count, MAX_ABILITY_SLOTS);
    
    // Update visibility of existing slots
    for (size_t i = 0; i < abilitySlots_.size(); ++i) {
        if (abilitySlots_[i]) {
            abilitySlots_[i]->SetVisible(i < abilitySlotCount_);
        }
    }
    
    UpdateLayout();
}

size_t HUDController::GetAbilitySlotCount() const {
    return abilitySlotCount_;
}

void HUDController::SetAbilityKeyLabel(size_t index, const std::string& label) {
    if (index < abilitySlots_.size() && abilitySlots_[index]) {
        abilitySlots_[index]->SetKeyLabel(label);
    }
}

void HUDController::SetAbilityIcon(size_t index, uint32_t textureId) {
    if (index < abilitySlots_.size() && abilitySlots_[index]) {
        abilitySlots_[index]->SetIconTexture(textureId);
    }
}

void HUDController::SetAbilityCooldown(size_t index, float percent) {
    if (index < abilitySlots_.size() && abilitySlots_[index]) {
        abilitySlots_[index]->SetCooldownPercent(percent);
    }
}

void HUDController::SetAbilityActive(size_t index, bool active) {
    if (index < abilitySlots_.size() && abilitySlots_[index]) {
        abilitySlots_[index]->SetActive(active);
    }
}

void HUDController::SetAbilityEnabled(size_t index, bool enabled) {
    if (index < abilitySlots_.size() && abilitySlots_[index]) {
        abilitySlots_[index]->SetEnabled(enabled);
    }
}

void HUDController::SetCrosshairColor(const glm::vec4& color) {
    if (crosshair_) {
        crosshair_->SetColor(color);
    }
}

void HUDController::SetCrosshairSize(float lineLength, float thickness, float gap) {
    if (crosshair_) {
        crosshair_->SetLineLength(lineLength);
        crosshair_->SetLineThickness(thickness);
        crosshair_->SetGap(gap);
    }
}

UIAbilitySlot* HUDController::GetAbilitySlot(size_t index) {
    if (index < abilitySlots_.size()) {
        return abilitySlots_[index];
    }
    return nullptr;
}

void HUDController::CreateCrosshair() {
    auto crosshair = std::make_unique<UICrosshair>();
    crosshair->SetName("Crosshair");
    crosshair->SetLineLength(10.0f);
    crosshair->SetLineThickness(2.0f);
    crosshair->SetGap(3.0f);
    crosshair->SetColor({1.0f, 1.0f, 1.0f, 0.9f});
    crosshair->SetOutlineEnabled(true);
    crosshair->SetOutlineColor({0.0f, 0.0f, 0.0f, 0.6f});

    // Position in center
    float crosshairSize = 50.0f;
    crosshair->SetSize({crosshairSize, crosshairSize});
    crosshair->SetPosition({
        (viewportWidth_ - crosshairSize) * 0.5f,
        (viewportHeight_ - crosshairSize) * 0.5f
    });

    crosshair_ = crosshair.get();
    root_->AddChild(std::move(crosshair));
}

void HUDController::CreateHealthBar() {
    auto healthBar = std::make_unique<UIHealthBar>();
    healthBar->SetName("Health Bar");
    healthBar->SetHealthRange(100.0f, 100.0f);
    healthBar->SetShowLabel(true);
    healthBar->SetLabelFormat("HP");
    healthBar->SetFontSize(13.0f);
    healthBar->SetUseGradient(true);

    // Custom styling for a modern look
    auto bg = std::make_shared<UIStyleBoxFlat>();
    bg->SetBackgroundColor({0.08f, 0.08f, 0.12f, 0.9f});
    bg->SetCornerRadiusAll(10.0f);
    bg->SetBorderWidthAll(2.0f);
    bg->SetBorderColor({0.25f, 0.3f, 0.4f, 1.0f});
    healthBar->SetStyleBoxBackground(bg);

    // Size and position in bottom-left
    float barWidth = 220.0f;
    float barHeight = 28.0f;
    float margin = 30.0f;

    healthBar->SetSize({barWidth, barHeight});
    healthBar->SetPosition({margin, viewportHeight_ - margin - barHeight});

    healthBar_ = healthBar.get();
    root_->AddChild(std::move(healthBar));
}

void HUDController::CreateAbilitySlots() {
    abilitySlots_.clear();

    // Default key labels
    std::vector<std::string> defaultKeys = {"1", "2", "SHIFT"};

    float slotSize = 56.0f;
    float slotSpacing = 8.0f;
    float margin = 30.0f;
    float keyLabelHeight = 20.0f;
    float totalHeight = slotSize + keyLabelHeight;

    float totalWidth = abilitySlotCount_ * slotSize + (abilitySlotCount_ - 1) * slotSpacing;
    float startX = viewportWidth_ - margin - totalWidth;
    float startY = viewportHeight_ - margin - totalHeight;

    for (size_t i = 0; i < abilitySlotCount_; ++i) {
        auto slot = std::make_unique<UIAbilitySlot>();
        slot->SetName("Ability Slot " + std::to_string(i + 1));

        if (i < defaultKeys.size()) {
            slot->SetKeyLabel(defaultKeys[i]);
        } else {
            slot->SetKeyLabel(std::to_string(i + 1));
        }

        slot->SetPlaceholderChar(static_cast<char>('A' + i));
        slot->SetSize({slotSize, totalHeight});
        slot->SetPosition({startX + i * (slotSize + slotSpacing), startY});

        abilitySlots_.push_back(slot.get());
        root_->AddChild(std::move(slot));
    }
}

void HUDController::UpdateLayout() {
    if (!initialized_) return;

    // Update root size
    if (root_) {
        root_->SetSize({viewportWidth_, viewportHeight_});
    }

    // Update crosshair position (center)
    if (crosshair_) {
        glm::vec2 size = crosshair_->GetSize();
        crosshair_->SetPosition({
            (viewportWidth_ - size.x) * 0.5f,
            (viewportHeight_ - size.y) * 0.5f
        });
    }

    // Update health bar position (bottom-left)
    if (healthBar_) {
        float margin = 30.0f;
        glm::vec2 size = healthBar_->GetSize();
        healthBar_->SetPosition({margin, viewportHeight_ - margin - size.y});
    }

    // Update ability slots position (bottom-right)
    if (!abilitySlots_.empty()) {
        float slotSize = 56.0f;
        float slotSpacing = 8.0f;
        float margin = 30.0f;
        float keyLabelHeight = 20.0f;
        float totalHeight = slotSize + keyLabelHeight;

        float totalWidth = abilitySlotCount_ * slotSize + (abilitySlotCount_ - 1) * slotSpacing;
        float startX = viewportWidth_ - margin - totalWidth;
        float startY = viewportHeight_ - margin - totalHeight;

        for (size_t i = 0; i < abilitySlots_.size() && i < abilitySlotCount_; ++i) {
            if (abilitySlots_[i]) {
                abilitySlots_[i]->SetPosition({
                    startX + i * (slotSize + slotSpacing),
                    startY
                });
            }
        }
    }

    SE_LOG_DEBUG("[HUDController] Layout updated for viewport {}x{}", viewportWidth_, viewportHeight_);
}

}  // namespace se::ui
