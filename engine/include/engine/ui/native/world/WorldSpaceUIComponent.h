#pragma once

#include "engine/ui/native/world/WorldSpaceUIElement.h"
#include <memory>
#include <vector>

namespace se {

struct WorldSpaceUIComponent {
    glm::vec3 offset{0.0f, 2.0f, 0.0f};
    
    bool billboardToCamera = true;
    bool scaleByDistance = true;
    
    float baseScale = 1.0f;
    float minScale = 0.3f;
    float maxScale = 2.0f;
    float referenceDistance = 10.0f;
    float maxDistance = 100.0f;
    
    bool depthTest = false;
    
    std::vector<std::shared_ptr<WorldSpaceUIElement>> elements;
    
    template<typename T, typename... Args>
    std::shared_ptr<T> AddElement(Args&&... args) {
        auto element = std::make_shared<T>(std::forward<Args>(args)...);
        elements.push_back(element);
        return element;
    }
    
    void RemoveElement(const std::shared_ptr<WorldSpaceUIElement>& element) {
        elements.erase(
            std::remove(elements.begin(), elements.end(), element),
            elements.end()
        );
    }
    
    void ClearElements() {
        elements.clear();
    }
};

}  // namespace se
