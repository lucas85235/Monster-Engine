#pragma once

#include <engine/Camera.h>
#include <engine/renderer/CameraController.h>
#include <engine/Layer.h>
#include <engine/ecs/Scene.h>
#include <engine/resources/MaterialManager.h>
#include <engine/resources/MeshManager.h>

#include <glm.hpp>
#include <memory>
#include <string>


using namespace se;
class AppLayer : public Layer {
   public:
    AppLayer();

    ~AppLayer() override;

    void OnAttach() override;

    void OnDetach() override;


    void OnUpdate(float ts) override;

    void OnRender() override;

    void OnImGuiRender() override;

   private:
    void HandleInput(float deltaTime);

    void LoadMaterial();

    // Helper methods for creating entities
    void AddDirectionalLight();

    void CreateCubeEntity(const std::string& name, const Vector3& position, const Vector3& scale = Vector3(1.0f));

    void CreateSphereEntity(const std::string& name, const Vector3& position);

    void CreateCapsuleEntity(const std::string& name, const Vector3& position);

   private:
    Scope<Scene>  scene_;
    Ref<Material> material_;

    // Camera and input
    Camera           camera_;
    CameraController cameraController_;

    // Animation time
    float animationTime_ = 0.0f;

    float yaw_ = 0.0f;

    bool camera_active_ = true;
};
