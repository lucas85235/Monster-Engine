#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class SkinnedModel;
struct SkinnedModelData;

// Manages loading and caching of skinned models
class SkinnedModelManager {
public:
    static void Init();
    static void Shutdown();
    
    // Load a skinned model from file (cached)
    static std::shared_ptr<SkinnedModel> Load(const std::string& path);
    
    // Get cached model by name
    static std::shared_ptr<SkinnedModel> Get(const std::string& name);
    
    // Check if model is cached
    static bool Has(const std::string& name);
    
    // Remove model from cache
    static void Unload(const std::string& name);
    
    // Clear all cached models
    static void ClearCache();
    
private:
    static std::shared_ptr<SkinnedModel> CreateFromData(std::shared_ptr<SkinnedModelData> data);
    
    static std::unordered_map<std::string, std::shared_ptr<SkinnedModel>> cache_;
    static bool initialized_;
};

}  // namespace se
