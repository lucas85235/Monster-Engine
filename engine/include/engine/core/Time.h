#pragma once
/**
 * Time.h - Centralized time management for the engine.
 *
 * Provides global access to frame timing information without
 * needing to pass delta time through function parameters.
 *
 * Usage:
 *   float dt = Time::DeltaTime();
 *   float totalTime = Time::GetTime();
 */

#include <cstdint>

namespace se {

class Application;

class Time {
public:
    /// Frame delta time (affected by TimeScale)
    static float DeltaTime() { return s_deltaTime * s_timeScale; }

    /// Frame delta time (unaffected by TimeScale)
    static float UnscaledDeltaTime() { return s_deltaTime; }

    /// Time since application start (affected by TimeScale)
    static float GetTime() { return s_time; }

    /// Time since application start (unaffected by TimeScale)
    static float UnscaledTime() { return s_unscaledTime; }

    /// Fixed timestep for physics (default: 1/60)
    static float FixedDeltaTime() { return s_fixedDeltaTime; }

    /// Current time scale (1.0 = normal, 0.0 = paused, 0.5 = slow motion)
    static float TimeScale() { return s_timeScale; }

    /// Set time scale
    static void SetTimeScale(float scale) { s_timeScale = scale > 0.0f ? scale : 0.0f; }

    /// Current frame number (starts at 0)
    static uint64_t FrameCount() { return s_frameCount; }

private:
    friend class Application;

    /// Called by Application each frame
    static void Update(float rawDeltaTime);

    static inline float    s_deltaTime      = 0.0f;
    static inline float    s_time           = 0.0f;
    static inline float    s_unscaledTime   = 0.0f;
    static inline float    s_fixedDeltaTime = 1.0f / 60.0f;
    static inline float    s_timeScale      = 1.0f;
    static inline uint64_t s_frameCount     = 0;
};

} // namespace se
