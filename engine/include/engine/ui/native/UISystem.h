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

}  // namespace se::ui
