#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class AnimationClip;

// Caches and manages animation clips
class AnimationManager {
public:
    static void Init();
    static void Shutdown();
    
    // Load animation from file (cached)
    static std::shared_ptr<AnimationClip> Load(const std::string& path);
    
    // Get cached animation by name
    static std::shared_ptr<AnimationClip> Get(const std::string& name);
    
    // Check if animation is cached
    static bool Has(const std::string& name);
    
    // Remove animation from cache
    static void Unload(const std::string& name);
    
    // Clear all cached animations
    static void ClearCache();
    
private:
    static std::unordered_map<std::string, std::shared_ptr<AnimationClip>> cache_;
    static bool initialized_;
};

}  // namespace se
