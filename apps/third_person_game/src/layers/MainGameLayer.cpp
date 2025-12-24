#include "MainGameLayer.h"

#include "apps/sandbox/src/SampleUtilities.h"
#include "apps/third_person_game/src/CharacterController.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/resources/MeshManager.h"

namespace FirstGame {
void ImguiDebug() {
    auto& app    = se::Application::Get();
    auto& window = app.GetWindow();

    ImGui::Begin("Main Game Debug");
    ImGui::Text("Debug Info");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Current Resolution: [%d x %d]", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    ImGui::End();
}

MainGameLayer::~MainGameLayer() = default;

// TODO(rafael): Move some of these functionalities later to inside the CharacterController
void MainGameLayer::OnAttach() {
    Layer::OnAttach();
    scene_     = CreateScope<Scene>("Main Game");
    
    // Set as active scene so Components can access it via Application::Get().GetActiveScene()
    Application::Get().SetActiveScene(scene_.get());
    
    character_ = CreateRef<Character>(scene_->CreateEntity("Character"));
    
    // AddComponent automatically detects if CharacterController inherits from Component
    // and uses the lifecycle system (Awake/Start/Update called automatically)
    auto controller = character_->GetEntity().AddComponent<CharacterController>();

    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
    material_ = Utilities::LoadMaterial();
    if (!material_) {
        SE_LOG_ERROR("Material not loaded, retrying");
        material_ = Utilities::LoadMaterial();
    }

    character_->GetEntity().AddComponent<MeshRenderComponent>(mesh, material_);

    scene_->GetPhysicsSystem()->GetDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
}

void MainGameLayer::OnDetach() {
    Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    
    scene_->OnUpdate(ts);
}

void MainGameLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();

    ImguiDebug();
}
}  // namespace FirstGame