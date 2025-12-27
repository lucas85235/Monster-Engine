#pragma once

#include <memory>

namespace se {

class Model;

struct ModelComponent {
    std::shared_ptr<Model> model;
    bool IsVisible = true;
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    ModelComponent() = default;
    ModelComponent(const ModelComponent&) = default;
    ModelComponent(std::shared_ptr<Model> m) : model(std::move(m)) {}
};

}  // namespace se
