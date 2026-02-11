#pragma once

#include <memory>
#include <entt.hpp>

class Camera;

namespace se {

class Shader;

namespace ui {
class UIFont;
}

class WorldSpaceUIRenderer {
public:
    static WorldSpaceUIRenderer& Get();
    
    void Initialize();
    void Shutdown();
    
    void Render(const ::Camera& camera, entt::registry& registry);
    
    std::shared_ptr<ui::UIFont> GetFont() const { return font_; }
    std::shared_ptr<Shader> GetTextShader() const { return textShader_; }
    std::shared_ptr<Shader> GetUIShader() const { return uiShader_; }
    
private:
    WorldSpaceUIRenderer() = default;
    ~WorldSpaceUIRenderer() = default;
    
    WorldSpaceUIRenderer(const WorldSpaceUIRenderer&) = delete;
    WorldSpaceUIRenderer& operator=(const WorldSpaceUIRenderer&) = delete;
    
    void InitializeResources();
    
    std::shared_ptr<Shader> textShader_;
    std::shared_ptr<Shader> uiShader_;
    std::shared_ptr<ui::UIFont> font_;
    
    uint32_t textVao_ = 0;
    uint32_t textVbo_ = 0;
    uint32_t quadVao_ = 0;
    uint32_t quadVbo_ = 0;
    
    bool initialized_ = false;
};

}  // namespace se
