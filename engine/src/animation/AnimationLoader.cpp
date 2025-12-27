#include "engine/animation/AnimationLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

#include "engine/animation/AnimationClip.h"
#include "engine/Log.h"

namespace se {

namespace {

// Remove Assimp FBX suffixes like "_$AssimpFbx$_Translation" from bone names
std::string CleanBoneName(const std::string& name) {
    size_t pos = name.find("_$AssimpFbx$_");
    if (pos != std::string::npos) {
        return name.substr(0, pos);
    }
    return name;
}

AnimationChannel ProcessChannel(const aiNodeAnim* nodeAnim) {
    AnimationChannel channel;
    channel.BoneName = CleanBoneName(nodeAnim->mNodeName.C_Str());
    
    // Position keyframes
    channel.PositionKeys.reserve(nodeAnim->mNumPositionKeys);
    for (unsigned int i = 0; i < nodeAnim->mNumPositionKeys; ++i) {
        VectorKeyframe key;
        key.Time = static_cast<float>(nodeAnim->mPositionKeys[i].mTime);
        key.Value = glm::vec3(
            nodeAnim->mPositionKeys[i].mValue.x,
            nodeAnim->mPositionKeys[i].mValue.y,
            nodeAnim->mPositionKeys[i].mValue.z
        );
        channel.PositionKeys.push_back(key);
    }
    
    // Rotation keyframes
    channel.RotationKeys.reserve(nodeAnim->mNumRotationKeys);
    for (unsigned int i = 0; i < nodeAnim->mNumRotationKeys; ++i) {
        QuatKeyframe key;
        key.Time = static_cast<float>(nodeAnim->mRotationKeys[i].mTime);
        const auto& q = nodeAnim->mRotationKeys[i].mValue;
        key.Value = glm::quat(q.w, q.x, q.y, q.z);
        channel.RotationKeys.push_back(key);
    }
    
    // Scale keyframes
    channel.ScaleKeys.reserve(nodeAnim->mNumScalingKeys);
    for (unsigned int i = 0; i < nodeAnim->mNumScalingKeys; ++i) {
        VectorKeyframe key;
        key.Time = static_cast<float>(nodeAnim->mScalingKeys[i].mTime);
        key.Value = glm::vec3(
            nodeAnim->mScalingKeys[i].mValue.x,
            nodeAnim->mScalingKeys[i].mValue.y,
            nodeAnim->mScalingKeys[i].mValue.z
        );
        channel.ScaleKeys.push_back(key);
    }
    
    return channel;
}

std::unique_ptr<AnimationClip> ProcessAnimation(const aiAnimation* anim) {
    std::string name = anim->mName.C_Str();
    if (name.empty()) name = "Animation";
    
    float duration = static_cast<float>(anim->mDuration);
    float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);
    if (ticksPerSecond < 1.0f) ticksPerSecond = 24.0f;
    
    auto clip = std::make_unique<AnimationClip>(name, duration, ticksPerSecond);
    
    SE_LOG_INFO("AnimationLoader: Processing '{}' - duration: {:.2f} ticks, {:.2f} tps, {} channels",
                name, duration, ticksPerSecond, anim->mNumChannels);
    
    for (unsigned int i = 0; i < anim->mNumChannels; ++i) {
        const aiNodeAnim* nodeAnim = anim->mChannels[i];
        std::string rawName = nodeAnim->mNodeName.C_Str();
        
        auto channel = ProcessChannel(nodeAnim);
        
        // Debug: log first 10 channels with their keyframe counts
        if (i < 10) {
            SE_LOG_INFO("AnimationLoader: Channel[{}] raw='{}' -> cleaned='{}' pos={} rot={} scale={}",
                i, rawName, channel.BoneName, 
                channel.PositionKeys.size(), 
                channel.RotationKeys.size(),
                channel.ScaleKeys.size());
        }
        
        clip->AddChannel(std::move(channel));
    }
    
    return clip;
}
}  // namespace

std::unique_ptr<AnimationClip> AnimationLoader::Load(const std::string& path) {
    SE_LOG_INFO("AnimationLoader: Loading animation from '{}'", path);
    
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    
    if (!scene) {
        SE_LOG_ERROR("AnimationLoader: Failed to load '{}': {}", path, importer.GetErrorString());
        return nullptr;
    }
    
    if (scene->mNumAnimations == 0) {
        SE_LOG_WARN("AnimationLoader: No animations found in '{}'", path);
        return nullptr;
    }
    
    return ProcessAnimation(scene->mAnimations[0]);
}

std::vector<std::unique_ptr<AnimationClip>> AnimationLoader::LoadAll(const std::string& path) {
    SE_LOG_INFO("AnimationLoader: Loading all animations from '{}'", path);
    
    std::vector<std::unique_ptr<AnimationClip>> clips;
    
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);
    
    if (!scene) {
        SE_LOG_ERROR("AnimationLoader: Failed to load '{}': {}", path, importer.GetErrorString());
        return clips;
    }
    
    SE_LOG_INFO("AnimationLoader: Found {} animations in '{}'", scene->mNumAnimations, path);
    
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        clips.push_back(ProcessAnimation(scene->mAnimations[i]));
    }
    
    return clips;
}

std::vector<std::unique_ptr<AnimationClip>> AnimationLoader::LoadFromModel(const std::string& modelPath) {
    return LoadAll(modelPath);
}

}  // namespace se
