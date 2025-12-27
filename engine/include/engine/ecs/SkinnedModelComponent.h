#pragma once

#include <memory>

#include "engine/animation/SkinnedModel.h"

namespace se {

// Component for entities with a skinned model (for skeletal animation)
struct SkinnedModelComponent {
    std::shared_ptr<SkinnedModel> model;
    bool IsVisible = true;
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    SkinnedModelComponent() = default;
    explicit SkinnedModelComponent(std::shared_ptr<SkinnedModel> m) : model(m) {}
};

}  // namespace se
