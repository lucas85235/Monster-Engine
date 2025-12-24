#include "MainGameLayer.h"

#include "apps/sandbox/src/SampleUtilities.h"
#include "apps/third_person_game/src/CharacterController.h"
#include "engine/Application.h"
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
    camera_    = Camera(glm::vec3(0.0f, 5.0f, 10.0f));
    scene_     = CreateScope<Scene>("Main Game");
    character_ = CreateRef<Character>(scene_->CreateEntity("Character"));
    character_->GetEntity().AddComponent<CharacterController>(character_.get(), scene_.get());

    auto& springArm           = character_->GetEntity().AddComponent<SpringArmComponent>();
    springArm.TargetArmLength = 8.0f;
    springArm.SocketOffset    = {0.0f, 1.5f, 0.0f};
    springArm.Pitch           = -30.0f;

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
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
}

void MainGameLayer::OnRender() {
    Layer::OnRender();

    auto& window      = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();

    // Render scene (entities with built-in frustum and occlusion culling)
    scene_->OnRender(camera_, aspectRatio);
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();

    ImguiDebug();
}
}  // namespace FirstGame