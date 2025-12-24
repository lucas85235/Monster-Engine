#include "MainGameLayer.h"

#include "apps/sandbox/src/SampleUtilities.h"
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
    auto trans = character_->GetEntity().GetComponent<TransformComponent>();
    trans.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    trans.SetScale(glm::vec3(1.0f, 1.0f, 1.0f));

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

    auto& input = InputManager::Get();
    input.BindAction("ToggleMouse", Key::Tab);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);
}

void MainGameLayer::OnDetach() {
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);

    UpdateCamera();
    auto& input = InputManager::Get();

    if (input.IsActionJustPressed("ToggleMouse")) {
        auto& app      = Application::Get();
        auto* window   = app.GetWindow().GetNativeWindow();
        mouseCaptured_ = !mouseCaptured_;
        glfwSetInputMode(window, GLFW_CURSOR,
                         mouseCaptured_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        SE_LOG_INFO("Mouse capture: {}", mouseCaptured_ ? "enabled" : "disabled");
    }
}

void MainGameLayer::UpdateCamera() {
    if (!mouseCaptured_) return;
    auto& input = InputManager::Get();

    if (!character_->GetEntity().HasComponent<SpringArmComponent>()) return;
    auto& springArm   = character_->GetEntity().GetComponent<SpringArmComponent>();
    auto& playerTrans = character_->GetEntity().GetComponent<TransformComponent>();

    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");

    springArm.Yaw -= mouseX * 0.1f;
    springArm.Pitch -= mouseY * 0.1f;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    float yawRad   = glm::radians(springArm.Yaw);
    float pitchRad = glm::radians(springArm.Pitch);

    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    glm::vec3 direction;
    direction.x = cosPitch * sinYaw;
    direction.y = sinPitch;
    direction.z = cosPitch * cosYaw;

    glm::vec3 targetPos = playerTrans.Position + springArm.SocketOffset;

    float desiredArmLength = springArm.TargetArmLength;

    if (springArm.DoCollisionTest && scene_->GetPhysicsSystem()) {
        btRigidBody* playerBody = nullptr;
        if (character_->GetEntity().HasComponent<RigidbodyComponent>()) {
            playerBody = character_->GetEntity().GetComponent<RigidbodyComponent>().GetRigidbody();
        }

        glm::vec3 rayStart = targetPos;
        glm::vec3 rayEnd =
            targetPos + direction * (springArm.TargetArmLength + springArm.ProbeSize);
        glm::vec3 hitPoint, hitNormal;

        bool hit =
            scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);

        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - springArm.ProbeSize;
            desiredArmLength  = glm::max(hitDistance, 0.5f);
        }
    }

    float lerpSpeed            = (desiredArmLength < springArm.CurrentArmLength) ? 15.0f : 5.0f;
    springArm.CurrentArmLength = glm::mix(springArm.CurrentArmLength, desiredArmLength,
                                          glm::clamp(lerpSpeed * (1.0f / 60.0f), 0.0f, 1.0f));

    glm::vec3 camPos = targetPos + direction * springArm.CurrentArmLength;

    camera_.SetPosition(camPos);
    camera_.SetYaw(-springArm.Yaw - 90.0f);
    camera_.SetPitch(-springArm.Pitch);
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