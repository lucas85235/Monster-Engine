#include "engine/ecs/AnimatorComponent.h"

#include "engine/Log.h"
#include "engine/animation/AnimationManager.h"

namespace se {

bool AnimatorComponent::PlayClip(const std::string& path, bool loopAnim) {
    auto clip = AnimationManager::Load(path);
    if (!clip) {
        SE_LOG_ERROR("AnimatorComponent: Failed to load animation from '{}'", path);
        return false;
    }
    
    Play(clip, loopAnim);
    return true;
}

bool AnimatorComponent::CrossfadeToClip(const std::string& path, float duration, bool loopAnim) {
    auto clip = AnimationManager::Load(path);
    if (!clip) {
        SE_LOG_ERROR("AnimatorComponent: Failed to load animation from '{}'", path);
        return false;
    }
    
    CrossfadeTo(clip, duration, loopAnim);
    return true;
}

}  // namespace se
