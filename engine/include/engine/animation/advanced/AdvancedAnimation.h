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

// Animation graph system
#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/graph/AnimationGraph.h"
#include "engine/animation/graph/ClipNode.h"
#include "engine/animation/graph/BlendNode.h"
#include "engine/animation/graph/BlendSpaceNode.h"
#include "engine/animation/graph/StateMachineNode.h"
#include "engine/animation/graph/LayerNode.h"

// Locomotion system
#include "engine/animation/locomotion/LocomotionConfig.h"
#include "engine/animation/locomotion/LocomotionController.h"

// IK system
#include "engine/animation/ik/IIKSolver.h"
#include "engine/animation/ik/TwoBoneIKSolver.h"

// Bone attachment system
#include "engine/animation/BoneAttachment.h"

// Orchestrating component
#include "engine/animation/AdvancedAnimatorComponent.h"

// Serialization
#include "engine/animation/advanced/AnimatorAsset.h"
#include "engine/animation/advanced/AnimatorAssetLoader.h"

namespace se {
namespace anim {

// Version info
constexpr const char* ADVANCED_ANIMATION_VERSION = "2.0.0";

}  // namespace anim
}  // namespace se
