#pragma once

/**
 * @file PerfOverlay.h
 * @brief Lightweight performance overlay using the native UI system
 * 
 * This overlay works in both Debug and Release builds, providing
 * minimal FPS and frame time display without ImGui overhead.
 * 
 * Usage:
 *   // In game layer initialization:
 *   se::perf::PerfOverlay::Initialize();
 *   
 *   // In game layer update (each frame):
 *   se::perf::PerfOverlay::Update(deltaTime);
 *   
 *   // Cleanup:
 *   se::perf::PerfOverlay::Shutdown();
 */

#include <cstdint>
#include <memory>

namespace se {
namespace ui { class UILabel; }
}

namespace se::perf {

/**
 * @class PerfOverlay
 * @brief Static class providing a lightweight FPS/frame time overlay
 * 
 * Uses the engine's native UI system (UILabel) for rendering,
 * avoiding ImGui overhead in Release builds.
 */
class PerfOverlay {
public:
    /**
     * @brief Initialize the performance overlay
     * @param fontSize Font size for the overlay text (default: 14)
     * 
     * Creates the UI label and positions it in the top-right corner.
     * Safe to call multiple times (subsequent calls are no-ops).
     */
    static void Initialize(float fontSize = 14.0f);
    
    /**
     * @brief Shutdown and cleanup the overlay
     */
    static void Shutdown();
    
    /**
     * @brief Update the overlay with current frame data
     * @param deltaTime Time since last frame in seconds
     * 
     * Call this each frame to update the displayed statistics.
     * Automatically calculates FPS from deltaTime.
     */
    static void Update(float deltaTime);
    
    /**
     * @brief Render the overlay
     * 
     * Call this after Update() to draw the overlay.
     * In Release builds, this is the only way the FPS is displayed.
     */
    static void Render();
    
    /**
     * @brief Check if the overlay is currently visible
     */
    static bool IsVisible();
    
    /**
     * @brief Show or hide the overlay
     */
    static void SetVisible(bool visible);
    
    /**
     * @brief Toggle overlay visibility
     */
    static void ToggleVisible();
    
    /**
     * @brief Set the update interval for smoothed FPS
     * @param interval Time in seconds between FPS updates (default: 0.5)
     */
    static void SetUpdateInterval(float interval);
    
    /**
     * @brief Set position offset from top-right corner
     * @param x Horizontal offset (positive = towards center)
     * @param y Vertical offset (positive = towards bottom)
     */
    static void SetPositionOffset(float x, float y);

private:
    PerfOverlay() = delete;
    
    static void UpdateLabelPosition();
    
    static std::unique_ptr<ui::UILabel> label_;
    static bool initialized_;
    static bool visible_;
    static float updateInterval_;
    static float accumulatedTime_;
    static float accumulatedFrames_;
    static float lastFps_;
    static float lastFrameTime_;
    static float offsetX_;
    static float offsetY_;
    static float fontSize_;
};

}  // namespace se::perf
