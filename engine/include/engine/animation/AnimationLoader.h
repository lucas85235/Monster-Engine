#pragma once

#include <memory>
#include <string>
#include <vector>

namespace se {

class AnimationClip;
struct SkinnedModelData;

// Loads animations from FBX/GLTF files using Assimp
class AnimationLoader {
public:
    // Load the first animation from a file
    static std::unique_ptr<AnimationClip> Load(const std::string& path);
    
    // Load all animations from a file
    static std::vector<std::unique_ptr<AnimationClip>> LoadAll(const std::string& path);
    
    // Load animations embedded in a model file
    static std::vector<std::unique_ptr<AnimationClip>> LoadFromModel(const std::string& modelPath);
};

}  // namespace se
