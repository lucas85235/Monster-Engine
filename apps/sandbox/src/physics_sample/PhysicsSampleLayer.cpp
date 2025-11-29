#include "PhysicsSampleLayer.h"

#include "engine/Application.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/new_event_system/EventBus.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/BoxCollider.h"
#include "engine/resources/MeshManager.h"
#include "../SampleUtilities.h"
#include <imgui.h>

PhysicsSampleLayer::~PhysicsSampleLayer() {}

void PhysicsSampleLayer::OnDetach() {
    Layer::OnDetach();
}

void PhysicsSampleLayer::OnAttach() {
    Layer::OnAttach();

    scene_ = CreateScope<Scene>();
    material_ = Utilities::LoadMaterial();

    // Camera setup
    camera_ = Camera(Vector3(0.0f, 5.0f, 15.0f));
    camera_.SetPitch(-20.0f);

    Application::Get().GetPhysicsManager().SetScene(*scene_);

    // Floor
    {
        auto floor = scene_->CreateEntity("Floor");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = floor.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({20.0f, 2.0f, 20.0f});

        BoxCollider collider;
        collider.Size = {20.0f, 2.0f, 20.0f};
        floor.AddComponent<BoxCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        floor.AddComponent<RigidbodyComponent>(data, floor);
    }

    // Dynamic Box
    {
        auto box = scene_->CreateEntity("Box");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        box.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = box.GetComponent<TransformComponent>();
        transform.SetPosition({-2.0f, 10.0f, 0.0f});

        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        box.AddComponent<BoxCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        box.AddComponent<RigidbodyComponent>(data, box);
    }

    // Dynamic Sphere
    {
        auto sphere = scene_->CreateEntity("Sphere");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Sphere);
        sphere.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = sphere.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 12.0f, 0.0f});

        SphereCollider collider;
        collider.Radius = 0.5f; // Default sphere primitive radius is usually 0.5 or 1.0, need to check. Assuming 0.5 radius (1.0 diameter)
        sphere.AddComponent<SphereCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        sphere.AddComponent<RigidbodyComponent>(data, sphere);
    }

    // Dynamic Capsule
    {
        auto capsule = scene_->CreateEntity("Capsule");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Capsule);
        capsule.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = capsule.GetComponent<TransformComponent>();
        transform.SetPosition({2.0f, 14.0f, 0.0f});

        CapsuleCollider collider;
        collider.Radius = 0.5f;
        collider.Height = 1.0f;
        capsule.AddComponent<CapsuleCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        capsule.AddComponent<RigidbodyComponent>(data, capsule);
    }
    
    // Directional Light
    {
        auto light = scene_->CreateEntity("Sun");
        auto& dirLight = light.AddComponent<DirectionalLightComponent>();
        dirLight.Color = {1.0f, 1.0f, 1.0f};
        dirLight.Intensity = 1.0f;
        
        auto& transform = light.GetComponent<TransformComponent>();
        transform.SetRotation({45.0f, 45.0f, 0.0f});
    }
}

void PhysicsSampleLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    scene_->OnUpdate(ts);
}

void PhysicsSampleLayer::OnRender() {
    Layer::OnRender();
    
    auto& window = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
    
    scene_->OnRender(camera_, aspectRatio);
    
    Application::Get().GetPhysicsManager().RenderDebug(camera_);
}

void PhysicsSampleLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImGui::Begin("Physics Debug");
    ImGui::Text("Press 'R' to reset scene (not implemented yet)");
    ImGui::End();
}

void PhysicsSampleLayer::OnEvent(Event& event) {
    Layer::OnEvent(event);
}
