#pragma once

#include <se_pch.h>
#include <memory>
#include <string>
#include <vector>

class Camera;

namespace se {

struct WorldSpaceRenderContext {
    const ::Camera* camera = nullptr;
    glm::vec3 worldPosition;
    glm::vec3 cameraRight;
    glm::vec3 cameraUp;
    float scale = 1.0f;
    float distanceToCamera = 0.0f;
};

class WorldSpaceUIElement {
public:
    virtual ~WorldSpaceUIElement() = default;
    virtual void Render(const WorldSpaceRenderContext& ctx) = 0;
    
    glm::vec2 localOffset{0.0f};
    bool visible = true;
};

class WorldSpaceText : public WorldSpaceUIElement {
public:
    void Render(const WorldSpaceRenderContext& ctx) override;
    
    std::string text;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float fontSize = 14.0f;
    bool centered = true;
};

class WorldSpaceProgressBar : public WorldSpaceUIElement {
public:
    void Render(const WorldSpaceRenderContext& ctx) override;
    
    float value = 1.0f;  // 0-1
    glm::vec2 size{80.0f, 8.0f};
    glm::vec4 fillColor{0.2f, 0.8f, 0.2f, 1.0f};
    glm::vec4 bgColor{0.1f, 0.1f, 0.1f, 0.8f};
    glm::vec4 borderColor{0.3f, 0.3f, 0.3f, 1.0f};
    float borderWidth = 1.0f;
};

}  // namespace se
