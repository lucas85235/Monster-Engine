#pragma once

#include <string>

namespace se {

class EventBus;

class Layer {
   public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer() = default;

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(float ts) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}

    const std::string& GetName() const { return debugName_; }

   protected:
    std::string debugName_;
};

}  // namespace se