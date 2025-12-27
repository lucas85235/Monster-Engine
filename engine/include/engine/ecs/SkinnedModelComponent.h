#pragma once

#include <memory>
#include <string>

#include <glm.hpp>

#include "engine/animation/SkinnedModel.h"

namespace se {

// Forward declarations
class SkinnedModelManager;

// Component for entities with a skinned model (for skeletal animation).
// Supports loading models directly via LoadModel() - engine handles everything automatically.
struct SkinnedModelComponent {
    std::shared_ptr<SkinnedModel> model;
    bool IsVisible = true;
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    // Transform corrections for imported models (e.g., FBX scale issues)
    glm::vec3 ModelScale{1.0f};
    glm::vec3 ModelOffset{0.0f};
    
    SkinnedModelComponent() = default;
    explicit SkinnedModelComponent(std::shared_ptr<SkinnedModel> m) : model(m) {}
    
    // High-level API: Load model from path, engine manages everything
    bool LoadModel(const std::string& path);
    
    // Configure model transform corrections
    void SetScale(float uniformScale) { ModelScale = glm::vec3(uniformScale); }
    void SetScale(const glm::vec3& scale) { ModelScale = scale; }
    void SetOffset(const glm::vec3& offset) { ModelOffset = offset; }
    
    // Get model data for animation (if skeleton exists)
    std::shared_ptr<SkinnedModelData> GetModelData() const {
        return model ? model->GetModelData() : nullptr;
    }
    
    bool HasSkeleton() const {
        return model && model->HasSkeleton();
    }
    
    bool IsValid() const {
        return model && model->IsValid();
    }
};

}  // namespace se
