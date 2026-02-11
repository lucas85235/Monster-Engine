#pragma once

#include "engine/ui/native/UIControl.h"

#include <memory>
#include <vector>
#include <string>

namespace se::ui {

class UICrosshair;
class UIHealthBar;
class UIAbilitySlot;

/**
 * @class HUDController
 * @brief Facade class managing all HUD components for the game.
 *
 * Provides a clean API for:
 * - Creating and positioning HUD elements
 * - Updating game state (health, abilities)
 * - Handling viewport resize
 * - Managing HUD visibility
 *
 * Following the Facade pattern for simplified HUD management.
 */
class HUDController {
public:
    static constexpr size_t MAX_ABILITY_SLOTS = 6;

    HUDController();
    ~HUDController();

    // Prevent copying
    HUDController(const HUDController&) = delete;
    HUDController& operator=(const HUDController&) = delete;

    // Initialization
    void Initialize(float viewportWidth, float viewportHeight);
    void Shutdown();

    // Returns the root container to add to UI system
    UIControl::Ptr GetRoot();

    // Viewport
    void OnViewportResize(float width, float height);

    // Visibility
    void SetVisible(bool visible);
    bool IsVisible() const;

    void SetCrosshairVisible(bool visible);
    void SetHealthBarVisible(bool visible);
    void SetAbilitySlotsVisible(bool visible);

    // Health
    void SetHealth(float current, float max);
    void SetHealth(float current);
    float GetHealth() const;
    float GetMaxHealth() const;

    // Abilities
    void SetAbilitySlotCount(size_t count);
    size_t GetAbilitySlotCount() const;

    void SetAbilityKeyLabel(size_t index, const std::string& label);
    void SetAbilityIcon(size_t index, uint32_t textureId);
    void SetAbilityCooldown(size_t index, float percent);
    void SetAbilityActive(size_t index, bool active);
    void SetAbilityEnabled(size_t index, bool enabled);

    // Crosshair customization
    void SetCrosshairColor(const glm::vec4& color);
    void SetCrosshairSize(float lineLength, float thickness, float gap);

    // Direct access (for advanced customization)
    UICrosshair* GetCrosshair() { return crosshair_; }
    UIHealthBar* GetHealthBar() { return healthBar_; }
    UIAbilitySlot* GetAbilitySlot(size_t index);

private:
    void CreateCrosshair();
    void CreateHealthBar();
    void CreateAbilitySlots();
    void UpdateLayout();

private:
    UIControl::Ptr root_;

    UICrosshair* crosshair_ = nullptr;
    UIHealthBar* healthBar_ = nullptr;
    std::vector<UIAbilitySlot*> abilitySlots_;

    float viewportWidth_ = 1920.0f;
    float viewportHeight_ = 1080.0f;

    size_t abilitySlotCount_ = 3;
    bool initialized_ = false;
};

}  // namespace se::ui
