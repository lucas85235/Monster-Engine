#include "PhysicsSampleLayer.h"

#include <imgui.h>

#include "../SampleUtilities.h"
#include "engine/Application.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/events/EventBus.h"
#include "engine/physics/Collider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/resources/MeshManager.h"

PhysicsSampleLayer::~PhysicsSampleLayer() {}

void PhysicsSampleLayer::OnDetach() {}

void PhysicsSampleLayer::OnAttach() {
    scene_    = CreateScope<Scene>();
    material_ = Utilities::LoadMaterial();

    // Camera setup
    camera_ = Camera(Vector3(0.0f, 5.0f, 15.0f));
    camera_.SetPitch(-20.0f);

    // Floor
    {
        auto floor = scene_->CreateEntity("Floor");
        auto mesh  = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = floor.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({20.0f, 2.0f, 20.0f});

        BoxCollider collider;
        collider.Size = {20.0f, 2.0f, 20.0f};
        floor.AddComponent<BoxCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        floor.AddComponent<RigidbodyComponent>(data);
    }

    // Dynamic Box
    {
        auto box  = scene_->CreateEntity("Box");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        box.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = box.GetComponent<TransformComponent>();
        transform.SetPosition({-2.0f, 10.0f, 0.0f});

        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        box.AddComponent<BoxCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        box.AddComponent<RigidbodyComponent>(data);
    }

    // Dynamic Sphere
    {
        auto sphere = scene_->CreateEntity("Sphere");
        auto mesh   = MeshManager::GetPrimitive(PrimitiveMeshType::Sphere);
        sphere.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = sphere.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 12.0f, 0.0f});

        SphereCollider collider;
        collider.Radius = 0.5f;
        sphere.AddComponent<SphereCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        sphere.AddComponent<RigidbodyComponent>(data);
    }

    // Dynamic Capsule
    {
        auto capsule = scene_->CreateEntity("Capsule");
        auto mesh    = MeshManager::GetPrimitive(PrimitiveMeshType::Capsule);
        capsule.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = capsule.GetComponent<TransformComponent>();
        transform.SetPosition({2.0f, 14.0f, 0.0f});

        CapsuleCollider collider;
        collider.Radius = 0.5f;
        collider.Height = 1.0f;
        capsule.AddComponent<CapsuleCollider>(collider);

        RigidbodyData data = RigidbodyData{.mass = 1.0f};
        capsule.AddComponent<RigidbodyComponent>(data);
    }

    // Directional Light
    {
        auto  light        = scene_->CreateEntity("Sun");
        auto& dirLight     = light.AddComponent<DirectionalLightComponent>();
        dirLight.Color     = {1.0f, 1.0f, 1.0f};
        dirLight.Intensity = 1.0f;

        auto& transform = light.GetComponent<TransformComponent>();
        transform.SetRotation({45.0f, 45.0f, 0.0f});
    }
}

void PhysicsSampleLayer::OnUpdate(float ts) {
    scene_->OnUpdate(ts);
}

void PhysicsSampleLayer::OnRender() {
    auto& window      = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();

    scene_->OnRender(camera_, aspectRatio);
}

void PhysicsSampleLayer::OnImGuiRender() {
    ImGui::Begin("Physics Debug");
    ImGui::Text("Press 'R' to reset scene (not implemented yet)");
    ImGui::End();
}
