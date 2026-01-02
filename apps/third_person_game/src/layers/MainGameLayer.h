#pragma once
/**
 * MainGameLayer - Primary game layer managing scene lifecycle.
 */

#include "../components/Character.h"
#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"

// Native UI System
#include "engine/ui/native/UISystem.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/ui/native/widgets/UILabel.h"
#include "engine/ui/native/widgets/UIPanel.h"
#include "engine/ui/native/widgets/UIButton.h"
#include "engine/ui/native/widgets/UIProgressBar.h"
#include "engine/ui/native/widgets/UISlider.h"
#include "engine/ui/native/widgets/UICheckBox.h"
#include "engine/ui/native/widgets/UITextureRect.h"

namespace FirstGame {
class MainGameLayer : public se::Layer {
public:
    ~MainGameLayer() override;

    void OnAttach() override;

    void OnDetach() override;

    void OnUpdate(float ts) override;

    void OnRender() override;

    void OnImGuiRender() override;

private:
    void ImguiDebug();
    void SetupNativeUI();
    void RenderNativeUI();

    se::Entity        character_entity_;
    se::Scope<se::Scene>  scene_;
    se::Ref<se::Material> material_;
    se::Entity    lightEntity_;
    
    // Native UI Demo
    se::ui::UIControl::Ptr uiRoot_;
    int buttonClickCount_ = 0;
    
    // Demo widget pointers for dynamic updates
    se::ui::UIProgressBar* progressBar_ = nullptr;
    se::ui::UILabel* sliderValueLabel_ = nullptr;
    float progressValue_ = 0.0f;
    bool animateProgress_ = true;
    
    // UI input state
    void UpdateNativeUIInput();
    glm::vec2 lastMousePos_{0.0f};
    bool lastMousePressed_ = false;
    se::ui::UIControl* lastHoveredControl_ = nullptr;
};
} // namespace FirstGame