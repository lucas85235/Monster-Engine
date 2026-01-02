#pragma once

/**
 * @file UISystem.h
 * @brief Master include for the native UI system
 *
 * This header includes all primary UI system types for convenience.
 * Individual headers can also be included directly for finer control.
 */

// Core types and enums
#include "engine/ui/native/UITypes.h"
#include "engine/ui/native/InputEvent.h"

// Base classes
#include "engine/ui/native/UIControl.h"
#include "engine/ui/native/UIContainer.h"

// Layout containers
#include "engine/ui/native/UIBoxContainer.h"

// Theme system
#include "engine/ui/native/theme/UITheme.h"
#include "engine/ui/native/theme/UIStyleBox.h"

namespace se::ui {

/**
 * @brief Initialize the native UI system
 * @param viewportWidth Initial viewport width
 * @param viewportHeight Initial viewport height
 */
void Initialize(float viewportWidth, float viewportHeight);

/**
 * @brief Shutdown the native UI system
 */
void Shutdown();

/**
 * @brief Update the UI system (process deferred layouts)
 */
void Update();

/**
 * @brief Render all visible UI elements
 */
void Render();

/**
 * @brief Set the root control for the UI scene
 * @param root The root control (takes ownership)
 */
void SetRoot(UIControl::Ptr root);

/**
 * @brief Get the current root control
 */
UIControl* GetRoot();

/**
 * @brief Send an input event to the UI system
 * @param event The input event
 * @return true if the event was consumed
 */
bool ProcessInput(const InputEvent& inputEvent);

/**
 * @brief Poll input from InputManager and dispatch to UI controls
 * 
 * Reads mouse state from InputManager, performs hit testing,
 * tracks hover changes (fires MOUSE_ENTER/EXIT), and dispatches events.
 * Call once per frame before Update().
 */
void PollInput();

/**
 * @brief Resize the UI viewport
 * @param viewportWidth New viewport width
 * @param viewportHeight New viewport height
 */
void Resize(float viewportWidth, float viewportHeight);

/**
 * @brief Get the currently hovered control
 */
UIControl* GetHoveredControl();

/**
 * @brief Check if the UI system wants to capture mouse input
 * @return true if the last mouse event was consumed by UI
 */
bool WantsMouseCapture();

/**
 * @brief Get current viewport size
 */
glm::vec2 GetViewportSize();

/**
 * @brief Set a callback to be invoked when viewport is resized
 * @param callback Function receiving new width and height
 */
void SetOnResizeCallback(std::function<void(float, float)> callback);

}  // namespace se::ui
