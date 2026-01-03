#pragma once
/**
 * AdvancedAnimation.h - Master include for advanced animation system.
 * 
 * Include this header to access all advanced animation components.
 */

// Core data structures
#include "engine/animation/advanced/Pose.h"
#include "engine/animation/advanced/BoneMask.h"

// Blend spaces
#include "engine/animation/advanced/BlendSpace.h"

// Layer system
#include "engine/animation/advanced/AnimationLayer.h"

// Procedural controllers
#include "engine/animation/advanced/LookAtController.h"
#include "engine/animation/advanced/BodyRotationController.h"

// Serialization
#include "engine/animation/advanced/AnimatorAsset.h"
#include "engine/animation/advanced/AnimatorAssetLoader.h"

namespace se {
namespace anim {

// Version info
constexpr const char* ADVANCED_ANIMATION_VERSION = "1.0.0";

}  // namespace anim
}  // namespace se
