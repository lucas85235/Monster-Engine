#include "engine/ecs/SkinnedModelComponent.h"

#include "engine/Log.h"
#include "engine/animation/SkinnedModelManager.h"

namespace se {

bool SkinnedModelComponent::LoadModel(const std::string& path) {
    model = SkinnedModelManager::Load(path);
    
    if (!model) {
        SE_LOG_ERROR("SkinnedModelComponent: Failed to load model from '{}'", path);
        return false;
    }
    
    SE_LOG_INFO("SkinnedModelComponent: Loaded '{}' with {} meshes, hasSkeleton={}",
                model->GetName(), model->GetMeshCount(), model->HasSkeleton());
    
    return true;
}

}  // namespace se
