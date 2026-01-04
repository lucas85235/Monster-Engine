#pragma once

#include <glm.hpp>
#include <string>

namespace se {
namespace anim {

struct LocomotionAnimPaths {
    std::string idle;
    std::string walk;
    std::string run;
    std::string walkBack;
    
    // Strafe animations (for aim mode)
    std::string strafeForward;
    std::string strafeBack;
    std::string strafeLeft;
    std::string strafeRight;
    std::string strafeIdle;  // Idle while in strafe mode
};

struct LocomotionConfig {
    LocomotionAnimPaths animations;
    
    // Blend space thresholds
    float idleThreshold = 0.1f;   // Below this = idle
    float walkThreshold = 2.0f;   // Walk speed
    float runThreshold = 5.0f;    // Run speed
    float maxSpeed = 8.0f;        // Max blend space bound
    
    // Transition durations
    float crossfadeDuration = 0.15f;
    float modeTransitionDuration = 0.3f;
    
    // Velocity smoothing
    float velocityLerpSpeed = 10.0f;
    float strafeLerpSpeed = 8.0f;
    
    static LocomotionConfig Default() {
        return LocomotionConfig{};
    }
};

}  // namespace anim
}  // namespace se
