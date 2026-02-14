#include "ThirdPersonLayer.h"

#include <btBulletDynamicsCommon.h>
#include <algorithm>
#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/ecs/SimpleComponents.h>
#include <engine/input/InputManager.h>
#include <engine/core/ServiceLocator.h>
#include <engine/renderer/MeshData.h>
#include <engine/renderer/MaterialSystem.h>
#include <engine/renderer/MeshSystem.h>
#include <engine/renderer/LightSystem.h>
#include <engine/renderer/FilamentRenderer.h>
#include <mmath/MathUtils.h>

#include "engine/physics/Collider.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

#include <GLFW/glfw3.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <imgui.h>

namespace {

// Helper: build a column-major mat4 from position, rotation (Euler degrees), and scale
void buildTransformMatrix(float out[16], const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scale) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, pos);
    m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0, 1, 0));
    m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1, 0, 0));
    m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0, 0, 1));
    m = glm::scale(m, scale);
    memcpy(out, &m[0][0], sizeof(float) * 16);
}

constexpr const char* kQualityItems[]   = {"LOW", "MEDIUM", "HIGH", "ULTRA"};
constexpr const char* kShadowTypeItems[] = {"PCF", "VSM", "DPCF", "PCSS"};
constexpr const char* kAaItems[]        = {"NONE", "FXAA"};
constexpr const char* kDitherItems[]    = {"NONE", "TEMPORAL"};
constexpr const char* kAoTypeItems[]    = {"SAO", "GTAO"};

} // anonymous namespace

ThirdPersonLayer::ThirdPersonLayer()
    : Layer("ThirdPersonLayer") {}

ThirdPersonLayer::~ThirdPersonLayer() {}

void ThirdPersonLayer::OnAttach() {
    SE_LOG_INFO("ThirdPersonLayer attached (Filament)");

    // Create PBR materials via MaterialSystem
    auto& materials = ServiceLocator::Get().GetMaterialSystem();

    MaterialConfig floorConfig;
    floorConfig.baseColor[0] = 0.4f;
    floorConfig.baseColor[1] = 0.4f;
    floorConfig.baseColor[2] = 0.4f;
    floorConfig.metallic     = 0.0f;
    floorConfig.roughness    = 0.8f;
    floorMaterial_ = materials.CreateMaterial(floorConfig);

    MaterialConfig playerConfig;
    playerConfig.baseColor[0] = 0.2f;
    playerConfig.baseColor[1] = 0.7f;
    playerConfig.baseColor[2] = 0.3f;
    playerConfig.metallic     = 0.1f;
    playerConfig.roughness    = 0.6f;
    playerMaterial_ = materials.CreateMaterial(playerConfig);

    MaterialConfig wallConfig;
    wallConfig.baseColor[0] = 0.5f;
    wallConfig.baseColor[1] = 0.5f;
    wallConfig.baseColor[2] = 0.55f;
    wallConfig.metallic     = 0.0f;
    wallConfig.roughness    = 0.7f;
    wallMaterial_ = materials.CreateMaterial(wallConfig);

    MaterialConfig cubeConfig;
    cubeConfig.baseColor[0] = 0.8f;
    cubeConfig.baseColor[1] = 0.6f;
    cubeConfig.baseColor[2] = 0.2f;
    cubeConfig.metallic     = 0.5f;
    cubeConfig.roughness    = 0.4f;
    cubeMaterial_ = materials.CreateMaterial(cubeConfig);

    MaterialConfig bulletConfig;
    bulletConfig.baseColor[0] = 0.9f;
    bulletConfig.baseColor[1] = 0.1f;
    bulletConfig.baseColor[2] = 0.1f;
    bulletConfig.metallic     = 0.7f;
    bulletConfig.roughness    = 0.3f;
    bulletMaterial_ = materials.CreateMaterial(bulletConfig);

    CreateScene();

    // Set up directional light (sun)
    auto& lights = ServiceLocator::Get().GetLightSystem();
    lights.SetDirectionalLight(
        -0.5f, -1.0f, -0.5f,  // direction
        1.0f, 0.95f, 0.9f,    // warm white
        110000.0f,             // outdoor sun intensity
        true                   // cast shadows
    );

    // Bind input axes and actions
    auto& input = InputManager::Get();
    input.BindAxis("MoveForward", Key::W, 1.0f);
    input.BindAxis("MoveForward", Key::S, -1.0f);
    input.BindAxis("MoveRight", Key::D, 1.0f);
    input.BindAxis("MoveRight", Key::A, -1.0f);
    input.BindAction("Jump", Key::Space);
    input.BindAction("Grab", Key::E);
    input.BindAction("Sprint", Key::LeftShift);
    input.BindAction("ToggleMouse", Key::Tab);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);
    input.BindAxis("ScrollWheel", Key::MouseScrollY, 1.0f);

    // Capture and hide cursor
    auto& app    = Application::Get();
    auto* window = app.GetWindow().GetNativeWindow();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    mouseCaptured_ = true;

    // Disable physics debug drawing by default.
    if (auto* physics = scene_->GetPhysicsSystem()) {
        if (auto* debugDrawer = physics->GetDebugDrawer()) {
            debugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
        }
    }

    InitializeFilamentUiState();
}

void ThirdPersonLayer::OnDetach() {
    auto& meshes = ServiceLocator::Get().GetMeshSystem();

    // Destroy all bullet renderables
    for (auto& r : bulletRenderables_) meshes.DestroyRenderable(r);
    bulletRenderables_.clear();

    // Destroy small wall renderables
    for (auto& r : smallWallRenderables_) meshes.DestroyRenderable(r);
    smallWallRenderables_.clear();

    // Destroy wall renderables
    for (auto& r : wallRenderables_) meshes.DestroyRenderable(r);
    wallRenderables_.clear();

    meshes.DestroyRenderable(cubeRenderable_);
    meshes.DestroyRenderable(playerRenderable_);
    meshes.DestroyRenderable(floorRenderable_);

    bullets_.clear();
    smallWalls_.clear();
    walls_.clear();
    scene_.reset();
}

void ThirdPersonLayer::CreateScene() {
    scene_ = std::make_shared<Scene>("Third Person Scene");

    auto& meshes = ServiceLocator::Get().GetMeshSystem();

    // ─── Floor ──────────────────────────────────────────────────
    {
        floor_entity_ = scene_->CreateEntity("Floor");
        auto& transform = floor_entity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({50.0f, 2.0f, 50.0f});

        RigidbodyData data = RigidbodyData{.mass = 0.0f, .gravityScale = 1.0f};
        floor_entity_.AddComponent<RigidbodyComponent>(data);

        auto meshData = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
        floorRenderable_ = meshes.CreateRenderable(meshData, floorMaterial_, false);
        float mat[16];
        buildTransformMatrix(mat, transform.Position, transform.Rotation, transform.Scale);
        meshes.SetTransform(floorRenderable_, mat);
    }

    // ─── Boundary Walls ─────────────────────────────────────────
    {
        struct WallDef {
            const char* name;
            glm::vec3   position;
            glm::vec3   scale;
        };

        const WallDef wallDefinitions[] = {
            {"Wall_North", {0.0f, 25.0f, 25.0f}, {50.0f, 50.0f, 1.0f}},
            {"Wall_South", {0.0f, 25.0f, -25.0f}, {50.0f, 50.0f, 1.0f}},
            {"Wall_East", {25.0f, 25.0f, 0.0f}, {1.0f, 50.0f, 50.0f}},
            {"Wall_West", {-25.0f, 25.0f, 0.0f}, {1.0f, 50.0f, 50.0f}},
        };

        walls_.reserve(std::size(wallDefinitions));
        wallRenderables_.reserve(std::size(wallDefinitions));

        auto wallMesh = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);

        for (const auto& def : wallDefinitions) {
            auto wall = scene_->CreateEntity(def.name);
            auto& wallTransform = wall.GetComponent<TransformComponent>();
            wallTransform.SetPosition(def.position);
            wallTransform.SetScale(def.scale);

            RigidbodyData data = RigidbodyData{.mass = 0.0f, .gravityScale = 1.0f};
            wall.AddComponent<RigidbodyComponent>(data);

            auto renderable = meshes.CreateRenderable(wallMesh, wallMaterial_, false);
            wallRenderables_.push_back(renderable);
            float mat[16];
            buildTransformMatrix(mat, wallTransform.Position, wallTransform.Rotation, wallTransform.Scale);
            meshes.SetTransform(renderable, mat);
            walls_.emplace_back(wall);
        }
    }

    // ─── Player Capsule ─────────────────────────────────────────
    {
        playerEntity_ = scene_->CreateEntity("Player");

        // Use a cylinder as capsule approximation
        auto playerMesh = MeshPrimitives::CreateCylinder(0.5f, 2.0f, 16);
        playerRenderable_ = meshes.CreateRenderable(playerMesh, playerMaterial_);

        playerEntity_.AddComponent<CapsuleCollider>(0.5f, 1.0f);

        auto& transform = playerEntity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 5.0f, 0.0f});
        transform.SetScale({1.0f, 1.0f, 1.0f});

        RigidbodyData data = RigidbodyData{.mass = 10.0f, .gravityScale = 1.0f};
        auto& rb = playerEntity_.AddComponent<RigidbodyComponent>(data);
        rb.SetAngularFactor({0.0f, 1.0f, 0.0f});
    }

    // ─── Physics Cube ───────────────────────────────────────────
    {
        cube_entity_ = scene_->CreateEntity("Cube");
        cube_entity_.GetComponent<TransformComponent>().SetPosition({0.0f, 10.0f, 0.0f});

        cube_entity_.AddComponent<BoxCollider>(Vector3(1.0f, 1.0f, 1.0f));
        RigidbodyData data;
        cube_entity_.AddComponent<RigidbodyComponent>(data);

        auto cubeMesh = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
        cubeRenderable_ = meshes.CreateRenderable(cubeMesh, cubeMaterial_);
    }

    // ─── Small Walls (Occlusion Testing) ────────────────────────
    {
        struct WallConfig {
            glm::vec3 position;
            glm::vec3 scale;
            float     yRotation;
        };

        std::vector<WallConfig> smallWallDefs = {
            {{10.0f, 1.5f, 0.0f}, {5.0f, 3.0f, 0.5f}, 0.0f},
            {{-10.0f, 1.5f, 5.0f}, {5.0f, 3.0f, 0.5f}, 45.0f},
            {{0.0f, 1.5f, -12.0f}, {8.0f, 3.0f, 0.5f}, 0.0f},
            {{15.0f, 1.5f, 15.0f}, {6.0f, 3.0f, 0.5f}, 30.0f},
            {{-8.0f, 1.5f, -8.0f}, {4.0f, 3.0f, 0.5f}, -45.0f},
        };

        auto wallMesh = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);

        for (size_t i = 0; i < smallWallDefs.size(); i++) {
            const auto& config = smallWallDefs[i];

            auto wall = scene_->CreateEntity("SmallWall_" + std::to_string(i));
            auto& transform = wall.GetComponent<TransformComponent>();
            transform.SetPosition(config.position);
            transform.SetScale(config.scale);
            transform.SetRotation({0.0f, config.yRotation, 0.0f});

            wall.AddComponent<BoxCollider>(glm::vec3(1.0f));
            RigidbodyData data;
            data.mass = 0.0f;
            wall.AddComponent<RigidbodyComponent>(data);

            auto renderable = meshes.CreateRenderable(wallMesh, wallMaterial_, false);
            smallWallRenderables_.push_back(renderable);
            float mat[16];
            buildTransformMatrix(mat, transform.Position, transform.Rotation, transform.Scale);
            meshes.SetTransform(renderable, mat);
            smallWalls_.push_back(wall);
        }

        SE_LOG_INFO("Created {} small walls for occlusion testing", smallWallDefs.size());
    }
}

void ThirdPersonLayer::InitializeFilamentUiState() {
    auto& filamentRenderer = ServiceLocator::Get().GetFilamentRenderer();
    auto* view             = filamentRenderer.GetView();
    auto* renderer         = filamentRenderer.GetRenderer();
    if (!view || !renderer) return;

    const auto& clear = renderer->getClearOptions();
    clearColor_[0]    = clear.clearColor[0];
    clearColor_[1]    = clear.clearColor[1];
    clearColor_[2]    = clear.clearColor[2];
    clearColor_[3]    = clear.clearColor[3];
    clearEnabled_     = clear.clear;
    clearDiscard_     = clear.discard;

    postProcessingEnabled_        = view->isPostProcessingEnabled();
    shadowingEnabled_             = view->isShadowingEnabled();
    screenSpaceRefractionEnabled_ = view->isScreenSpaceRefractionEnabled();
    antiAliasing_                 = static_cast<int>(view->getAntiAliasing());
    dithering_                    = static_cast<int>(view->getDithering());
    shadowType_                   = static_cast<int>(view->getShadowType());
    hdrQuality_                   = static_cast<int>(view->getRenderQuality().hdrColorBuffer);

    dynamicResOptions_ = view->getDynamicResolutionOptions();
    msaaOptions_       = view->getMultiSampleAntiAliasingOptions();
    taaOptions_        = view->getTemporalAntiAliasingOptions();
    aoOptions_         = view->getAmbientOcclusionOptions();
    ssrOptions_        = view->getScreenSpaceReflectionsOptions();
    bloomOptions_      = view->getBloomOptions();
    fogOptions_        = view->getFogOptions();
    vignetteOptions_   = view->getVignetteOptions();
    guardBandOptions_  = view->getGuardBandOptions();
    vsmShadowOptions_  = view->getVsmShadowOptions();
    softShadowOptions_ = view->getSoftShadowOptions();

    // Renderer has no getter for frame-rate options; seed with sane defaults.
    frameRateOptions_ = filament::Renderer::FrameRateOptions{};

    filamentUiInitialized_ = true;
    filamentSettingsDirty_ = false;
}

void ThirdPersonLayer::ApplyFilamentUiState() {
    auto& filamentRenderer = ServiceLocator::Get().GetFilamentRenderer();
    auto* view             = filamentRenderer.GetView();
    auto* renderer         = filamentRenderer.GetRenderer();
    if (!view || !renderer) return;

    antiAliasing_ = std::clamp(antiAliasing_, 0, 1);
    dithering_    = std::clamp(dithering_, 0, 1);
    shadowType_   = std::clamp(shadowType_, 0, 3);
    hdrQuality_   = std::clamp(hdrQuality_, 0, 3);

    filament::Renderer::ClearOptions clear = renderer->getClearOptions();
    clear.clearColor = {clearColor_[0], clearColor_[1], clearColor_[2], clearColor_[3]};
    clear.clear      = clearEnabled_;
    clear.discard    = clearDiscard_;
    renderer->setClearOptions(clear);
    renderer->setFrameRateOptions(frameRateOptions_);

    view->setPostProcessingEnabled(postProcessingEnabled_);
    view->setShadowingEnabled(shadowingEnabled_);
    view->setScreenSpaceRefractionEnabled(screenSpaceRefractionEnabled_);
    view->setAntiAliasing(static_cast<filament::AntiAliasing>(antiAliasing_));
    view->setDithering(static_cast<filament::Dithering>(dithering_));
    view->setShadowType(static_cast<filament::ShadowType>(shadowType_));
    view->setDynamicResolutionOptions(dynamicResOptions_);
    view->setMultiSampleAntiAliasingOptions(msaaOptions_);
    view->setTemporalAntiAliasingOptions(taaOptions_);
    view->setAmbientOcclusionOptions(aoOptions_);
    view->setScreenSpaceReflectionsOptions(ssrOptions_);
    view->setBloomOptions(bloomOptions_);
    view->setFogOptions(fogOptions_);
    view->setVignetteOptions(vignetteOptions_);
    view->setGuardBandOptions(guardBandOptions_);
    view->setVsmShadowOptions(vsmShadowOptions_);
    view->setSoftShadowOptions(softShadowOptions_);

    auto renderQuality         = view->getRenderQuality();
    renderQuality.hdrColorBuffer = static_cast<filament::QualityLevel>(hdrQuality_);
    view->setRenderQuality(renderQuality);

    filamentSettingsDirty_ = false;
}

void ThirdPersonLayer::SyncPhysicsToRenderables() {
    auto& meshes = ServiceLocator::Get().GetMeshSystem();

    // Sync player (dynamic)
    {
        auto& t = playerEntity_.GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(playerRenderable_, mat);
    }

    // Sync cube (dynamic)
    {
        auto& t = cube_entity_.GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(cubeRenderable_, mat);
    }

    // Sync bullets (dynamic)
    for (size_t i = 0; i < bullets_.size() && i < bulletRenderables_.size(); i++) {
        if (!bullets_[i].IsValid()) continue;
        auto& t = bullets_[i].GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(bulletRenderables_[i], mat);
    }
}

void ThirdPersonLayer::OnUpdate(float ts) {
    UpdatePlayer(ts);
    UpdateGrabSystem(ts);

    // Physics + ECS update
    scene_->OnUpdate(ts);

    // Update camera after physics
    UpdateCamera();

    // Sync physics transforms to Filament renderables
    SyncPhysicsToRenderables();
}

void ThirdPersonLayer::UpdatePlayer(float ts) {
    auto& input     = InputManager::Get();
    auto& transform = playerEntity_.GetComponent<TransformComponent>();
    auto& rb        = playerEntity_.GetComponent<RigidbodyComponent>();

    // Calculate camera-relative movement direction
    float moveForward = input.GetAxis("MoveForward");
    float moveRight   = input.GetAxis("MoveRight");

    float yawRad = springArmYaw_ * 0.0174533f;
    float sinYaw = std::sin(yawRad);
    float cosYaw = std::cos(yawRad);

    glm::vec3 camForward = {-sinYaw, 0.0f, -cosYaw};
    glm::vec3 camRight   = {cosYaw, 0.0f, -sinYaw};

    glm::vec3 movement = (camForward * moveForward + camRight * moveRight);

    btVector3 currentVelocity = rb.GetLinearVelocity();
    btVector3 desiredVelocity(0, currentVelocity.y(), 0);

    if (glm::length(movement) > 0.01f) {
        movement = glm::normalize(movement);

        // Smoothly rotate player to face movement direction
        float targetYaw  = luma::YawFromDirection(movement.x, movement.z);
        float currentYaw = transform.Rotation.y;

        float diff = targetYaw - currentYaw;
        diff       = luma::NormalizeAngleDeg(diff);

        if (std::abs(diff) > 170.0f && std::abs(lastRotationDiff_) > 0.0f) {
            if ((diff > 0 && lastRotationDiff_ < 0) || (diff < 0 && lastRotationDiff_ > 0)) {
                if (diff > 0) diff -= 360.0f;
                else diff += 360.0f;
            }
        }
        lastRotationDiff_ = diff;

        float rotationSpeed = 10.0f;
        float newYaw        = currentYaw + diff * glm::clamp(rotationSpeed * ts, 0.0f, 1.0f);
        newYaw              = luma::NormalizeAngleDeg(newYaw);

        rb.SetRotation({0.0f, newYaw, 0.0f});

        debugTargetYaw_  = targetYaw;
        debugCurrentYaw_ = currentYaw;
        debugNewYaw_     = newYaw;
    }

    // Ground check via raycast
    glm::vec3 rayStart = transform.Position + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 rayEnd   = transform.Position + glm::vec3(0.0f, -1.2f, 0.0f);
    glm::vec3 hitPoint, hitNormal;

    bool hit = scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal,
                                                   rb.GetRigidbody());
    isGrounded_ = hit;

    // Jumping
    if (input.IsActionJustPressed("Jump") && isGrounded_) {
        desiredVelocity.setY(5.0f);
    }

    if (!isGrounded_) {
        desiredVelocity.setY(currentVelocity.y());
    } else {
        if (input.IsActionJustPressed("Jump")) { desiredVelocity.setY(5.0f); }
    }

    // Movement speed — Sprint with Shift
    float speed = input.IsActionPressed("Sprint") ? sprintSpeed_ : moveSpeed_;
    desiredVelocity.setX(movement.x * speed);
    desiredVelocity.setZ(movement.z * speed);

    rb.SetLinearVelocity(desiredVelocity);

    // Shooting
    if (input.IsMouseButtonDown(0)) {
        Shoot();
    }

    // Toggle mouse capture with Tab
    if (input.IsActionJustPressed("ToggleMouse")) {
        auto& app      = Application::Get();
        auto* window   = app.GetWindow().GetNativeWindow();
        mouseCaptured_ = !mouseCaptured_;
        glfwSetInputMode(window, GLFW_CURSOR,
                         mouseCaptured_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        SE_LOG_INFO("Mouse capture: {}", mouseCaptured_ ? "enabled" : "disabled");
    }

    // Grab/Release with E
    if (input.IsActionJustPressed("Grab")) { TryGrabOrRelease(); }
}

void ThirdPersonLayer::Shoot() {
    float time = (float)glfwGetTime();
    if (time - lastShootTime_ < 0.02f) return;

    // Hard cap active bullets to avoid unbounded CPU/GPU growth.
    if ((int)bullets_.size() >= maxBullets_) return;

    lastShootTime_ = time;

    // Get camera forward
    float     yawRad   = glm::radians(cameraYaw_);
    float     pitchRad = glm::radians(cameraPitch_);
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward   = glm::normalize(forward);

    // Spawn position in front of camera
    glm::vec3 spawnPos = cameraPosition_ + forward * 10.0f;

    // Create bullet entity
    auto box = scene_->CreateEntity("BulletBox");
    box.AddComponent<BoxCollider>(glm::vec3(1.0f));
    box.GetComponent<TransformComponent>().SetPosition(spawnPos);
    box.GetComponent<TransformComponent>().SetScale(glm::vec3(0.5f));

    RigidbodyData data;
    data.mass = 2.0f;
    auto& rb  = box.AddComponent<RigidbodyComponent>(data);

    btVector3 impulse(forward.x, forward.y, forward.z);
    impulse *= 50.0f;
    rb.GetRigidbody()->applyCentralImpulse(impulse);

    // Create Filament renderable for the bullet
    auto& meshes = ServiceLocator::Get().GetMeshSystem();
    auto bulletMesh = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
    auto renderable = meshes.CreateRenderable(bulletMesh, bulletMaterial_, false);
    bulletRenderables_.push_back(renderable);

    bullets_.push_back(box);
}

void ThirdPersonLayer::UpdateCamera() {
    auto& input = InputManager::Get();

    auto& playerTrans = playerEntity_.GetComponent<TransformComponent>();

    // Only update look yaw/pitch while mouse is locked to gameplay.
    if (mouseCaptured_) {
        float mouseX = input.GetAxis("CameraRotateX");
        float mouseY = input.GetAxis("CameraRotateY");

        springArmYaw_ -= mouseX * 0.1f;
        springArmPitch_ -= mouseY * 0.1f;
        springArmPitch_ = glm::clamp(springArmPitch_, -80.0f, 80.0f);
    }

    float yawRad   = glm::radians(springArmYaw_);
    float pitchRad = glm::radians(springArmPitch_);

    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    glm::vec3 direction;
    direction.x = cosPitch * sinYaw;
    direction.y = sinPitch;
    direction.z = cosPitch * cosYaw;

    glm::vec3 targetPos = playerTrans.Position + socketOffset_;

    float desiredArmLength = springArmLength_;

    // Camera collision test
    if (scene_->GetPhysicsSystem()) {
        btRigidBody* playerBody = nullptr;
        if (playerEntity_.HasComponent<RigidbodyComponent>()) {
            playerBody = playerEntity_.GetComponent<RigidbodyComponent>().GetRigidbody();
        }

        glm::vec3 rayStart = targetPos;
        glm::vec3 rayEnd   = targetPos + direction * (springArmLength_ + 0.5f);
        glm::vec3 hitPoint, hitNormal;

        bool hit = scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);
        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - 0.5f;
            desiredArmLength  = glm::max(hitDistance, 0.5f);
        }
    }

    float lerpSpeed    = (desiredArmLength < currentArmLength_) ? 15.0f : 5.0f;
    currentArmLength_  = glm::mix(currentArmLength_, desiredArmLength,
                                  glm::clamp(lerpSpeed * (1.0f / 60.0f), 0.0f, 1.0f));

    glm::vec3 camPos = targetPos + direction * currentArmLength_;

    // Store camera state for shooting/grab
    cameraPosition_ = camPos;
    cameraYaw_   = -springArmYaw_ - 90.0f;
    cameraPitch_ = -springArmPitch_;

    // Update Filament camera
    auto& renderer = ServiceLocator::Get().GetFilamentRenderer();
    auto& window   = Application::Get().GetWindow();
    float aspect   = (float)window.GetWidth() / (float)window.GetHeight();

    renderer.SetCameraProjection(60.0, aspect, 0.1, 500.0);
    renderer.SetCameraLookAt(
        {camPos.x, camPos.y, camPos.z},
        {targetPos.x, targetPos.y, targetPos.z},
        {0.0f, 1.0f, 0.0f}
    );
}

void ThirdPersonLayer::TryGrabOrRelease() {
    if (grabbedBody_) {
        grabbedBody_->setGravity(btVector3(savedGravity_.x, savedGravity_.y, savedGravity_.z));
        grabbedBody_->setLinearVelocity(btVector3(0, 0, 0));
        grabbedBody_->activate();
        SE_LOG_INFO("Released grabbed object");
        grabbedBody_ = nullptr;
        return;
    }

    auto& rb = playerEntity_.GetComponent<RigidbodyComponent>();

    float     yawRad   = glm::radians(cameraYaw_);
    float     pitchRad = glm::radians(cameraPitch_);
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward   = glm::normalize(forward);

    glm::vec3 rayStart = cameraPosition_;
    glm::vec3 rayEnd   = rayStart + forward * grabMaxDistance_;
    glm::vec3 hitPoint;

    btRigidBody* hitBody =
        scene_->GetPhysicsSystem()->RaycastHitBody(rayStart, rayEnd, hitPoint, rb.GetRigidbody());

    if (hitBody && hitBody->getMass() > 0.0f) {
        if (hitBody == rb.GetRigidbody()) return;

        grabbedBody_ = hitBody;

        btVector3 grav = grabbedBody_->getGravity();
        savedGravity_  = glm::vec3(grav.x(), grav.y(), grav.z());
        grabbedBody_->setGravity(btVector3(0, 0, 0));
        grabbedBody_->setActivationState(DISABLE_DEACTIVATION);

        grabDistance_ = glm::length(hitPoint - rayStart);
        grabDistance_ = glm::clamp(grabDistance_, 2.0f, grabMaxDistance_);

        SE_LOG_INFO("Grabbed object at distance {:.2f}", grabDistance_);
    }
}

void ThirdPersonLayer::UpdateGrabSystem(float ts) {
    if (!grabbedBody_) return;

    auto& input  = InputManager::Get();
    float scroll = input.GetAxis("ScrollWheel");
    if (std::abs(scroll) > 0.01f) {
        grabDistance_ -= scroll * 2.0f;
        grabDistance_ = glm::clamp(grabDistance_, grabMinDistance_, grabMaxDistance_);
    }

    float     yawRad   = glm::radians(cameraYaw_);
    float     pitchRad = glm::radians(cameraPitch_);
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward   = glm::normalize(forward);

    glm::vec3 targetPos = cameraPosition_ + forward * grabDistance_;

    btTransform transform;
    grabbedBody_->getMotionState()->getWorldTransform(transform);
    btVector3 currentPos = transform.getOrigin();
    glm::vec3 objPos(currentPos.x(), currentPos.y(), currentPos.z());

    glm::vec3 delta    = targetPos - objPos;
    float     distance = glm::length(delta);

    if (distance > grabMaxDistance_ * 1.5f) {
        grabbedBody_->setGravity(btVector3(savedGravity_.x, savedGravity_.y, savedGravity_.z));
        grabbedBody_->activate();
        SE_LOG_INFO("Object dropped (too far)");
        grabbedBody_ = nullptr;
        return;
    }

    float     grabStrength = 15.0f;
    glm::vec3 velocity     = delta * grabStrength;

    btVector3 currentVel = grabbedBody_->getLinearVelocity();
    glm::vec3 dampedVel  = glm::vec3(currentVel.x(), currentVel.y(), currentVel.z()) * 0.5f;
    velocity             = velocity + dampedVel * 0.1f;

    grabbedBody_->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    grabbedBody_->setAngularVelocity(grabbedBody_->getAngularVelocity() * 0.9f);
}

void ThirdPersonLayer::OnRender() {
    // Filament rendering is handled by FilamentRenderer in the Application loop.
    // The camera is already configured in UpdateCamera().
    // All renderables are synced in SyncPhysicsToRenderables().
}

void ThirdPersonLayer::OnImGuiRender() {
    if (!filamentUiInitialized_) {
        InitializeFilamentUiState();
    }
    if (!filamentUiInitialized_) return;

    if (showImGuiDemo_) {
        ImGui::ShowDemoWindow(&showImGuiDemo_);
    }

    bool changed = false;

    if (ImGui::Begin("Filament Render Controls")) {
        auto* view  = ServiceLocator::Get().GetFilamentRenderer().GetView();
        auto* scene = view ? view->getScene() : nullptr;
        const bool hasIbl = scene && scene->getIndirectLight();

        ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
        ImGui::Text("AO prerequisites: post=%s, indirectLight=%s",
                    postProcessingEnabled_ ? "on" : "off",
                    hasIbl ? "on" : "off");
        ImGui::Separator();
        ImGui::Checkbox("Show ImGui Demo", &showImGuiDemo_);

        if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::ColorEdit4("Clear Color", clearColor_);
            changed |= ImGui::Checkbox("Clear Target", &clearEnabled_);
            changed |= ImGui::Checkbox("Discard Target", &clearDiscard_);
            int frameInterval = static_cast<int>(frameRateOptions_.interval);
            if (ImGui::SliderInt("Frame Interval", &frameInterval, 1, 4)) {
                frameRateOptions_.interval = static_cast<uint8_t>(frameInterval);
                changed = true;
            }
            int frameHistory = static_cast<int>(frameRateOptions_.history);
            if (ImGui::SliderInt("Frame History", &frameHistory, 1, 31)) {
                frameRateOptions_.history = static_cast<uint8_t>(frameHistory);
                changed = true;
            }
            changed |= ImGui::SliderFloat("Headroom", &frameRateOptions_.headRoomRatio, 0.0f, 0.5f);
            changed |= ImGui::SliderFloat("Scale Rate", &frameRateOptions_.scaleRate, 0.01f, 1.0f);
        }

        if (ImGui::CollapsingHeader("View Core", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::Checkbox("Post Processing", &postProcessingEnabled_);
            changed |= ImGui::Checkbox("Shadowing", &shadowingEnabled_);
            changed |= ImGui::Checkbox("Screen Space Refraction", &screenSpaceRefractionEnabled_);
            changed |= ImGui::Combo("Post AA", &antiAliasing_, kAaItems, IM_ARRAYSIZE(kAaItems));
            changed |= ImGui::Combo("Dithering", &dithering_, kDitherItems, IM_ARRAYSIZE(kDitherItems));
            changed |= ImGui::Combo("Shadow Type", &shadowType_, kShadowTypeItems, IM_ARRAYSIZE(kShadowTypeItems));
            changed |= ImGui::Combo("HDR Buffer Quality", &hdrQuality_, kQualityItems, IM_ARRAYSIZE(kQualityItems));
        }

        if (ImGui::CollapsingHeader("Dynamic Resolution")) {
            changed |= ImGui::Checkbox("Enabled##dsr", &dynamicResOptions_.enabled);
            changed |= ImGui::Checkbox("Homogeneous Scaling", &dynamicResOptions_.homogeneousScaling);
            changed |= ImGui::SliderFloat2("Min Scale", &dynamicResOptions_.minScale[0], 0.25f, 1.0f);
            changed |= ImGui::SliderFloat2("Max Scale", &dynamicResOptions_.maxScale[0], 0.5f, 2.0f);
            changed |= ImGui::SliderFloat("Sharpness##dsr", &dynamicResOptions_.sharpness, 0.0f, 1.0f);
            int quality = static_cast<int>(dynamicResOptions_.quality);
            if (ImGui::Combo("Upscale Quality", &quality, kQualityItems, IM_ARRAYSIZE(kQualityItems))) {
                dynamicResOptions_.quality = static_cast<filament::QualityLevel>(quality);
                changed = true;
            }
        }

        if (ImGui::CollapsingHeader("MSAA / TAA")) {
            changed |= ImGui::Checkbox("MSAA Enabled", &msaaOptions_.enabled);
            int msaaSamples = static_cast<int>(msaaOptions_.sampleCount);
            if (ImGui::SliderInt("MSAA Samples", &msaaSamples, 1, 8)) {
                msaaOptions_.sampleCount = static_cast<uint8_t>(msaaSamples);
                changed = true;
            }
            changed |= ImGui::Checkbox("MSAA Custom Resolve", &msaaOptions_.customResolve);

            changed |= ImGui::Checkbox("TAA Enabled", &taaOptions_.enabled);
            changed |= ImGui::SliderFloat("TAA Feedback", &taaOptions_.feedback, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("TAA Sharpness", &taaOptions_.sharpness, 0.0f, 2.0f);
            changed |= ImGui::SliderFloat("TAA Upscaling", &taaOptions_.upscaling, 1.0f, 2.0f);
            changed |= ImGui::Checkbox("TAA Prevent Flickering", &taaOptions_.preventFlickering);
        }

        if (ImGui::CollapsingHeader("Ambient Occlusion / SSR")) {
            changed |= ImGui::Checkbox("SSAO Enabled", &aoOptions_.enabled);
            int aoType = static_cast<int>(aoOptions_.aoType);
            if (ImGui::Combo("AO Type", &aoType, kAoTypeItems, IM_ARRAYSIZE(kAoTypeItems))) {
                aoOptions_.aoType = static_cast<filament::AmbientOcclusionOptions::AmbientOcclusionType>(aoType);
                changed = true;
            }
            changed |= ImGui::SliderFloat("AO Radius", &aoOptions_.radius, 0.05f, 5.0f);
            changed |= ImGui::SliderFloat("AO Power", &aoOptions_.power, 0.1f, 5.0f);
            changed |= ImGui::SliderFloat("AO Intensity", &aoOptions_.intensity, 0.0f, 5.0f);
            int aoQuality = static_cast<int>(aoOptions_.quality);
            if (ImGui::Combo("AO Quality", &aoQuality, kQualityItems, IM_ARRAYSIZE(kQualityItems))) {
                aoOptions_.quality = static_cast<filament::QualityLevel>(aoQuality);
                changed = true;
            }

            changed |= ImGui::Checkbox("SSR Enabled", &ssrOptions_.enabled);
            changed |= ImGui::SliderFloat("SSR Thickness", &ssrOptions_.thickness, 0.01f, 2.0f);
            changed |= ImGui::SliderFloat("SSR Bias", &ssrOptions_.bias, 0.0f, 0.2f);
            changed |= ImGui::SliderFloat("SSR Max Distance", &ssrOptions_.maxDistance, 0.1f, 25.0f);
            changed |= ImGui::SliderFloat("SSR Stride", &ssrOptions_.stride, 0.5f, 8.0f);
            changed |= ImGui::Checkbox("Guard Band", &guardBandOptions_.enabled);
        }

        if (ImGui::CollapsingHeader("Bloom / Fog / Vignette")) {
            changed |= ImGui::Checkbox("Bloom Enabled", &bloomOptions_.enabled);
            changed |= ImGui::SliderFloat("Bloom Strength", &bloomOptions_.strength, 0.0f, 1.0f);
            int bloomLevels = static_cast<int>(bloomOptions_.levels);
            if (ImGui::SliderInt("Bloom Levels", &bloomLevels, 1, 11)) {
                bloomOptions_.levels = static_cast<uint8_t>(bloomLevels);
                changed = true;
            }
            changed |= ImGui::SliderFloat("Bloom Highlight", &bloomOptions_.highlight, 10.0f, 3000.0f);
            int bloomQuality = static_cast<int>(bloomOptions_.quality);
            if (ImGui::Combo("Bloom Quality", &bloomQuality, kQualityItems, IM_ARRAYSIZE(kQualityItems))) {
                bloomOptions_.quality = static_cast<filament::QualityLevel>(bloomQuality);
                changed = true;
            }

            changed |= ImGui::Checkbox("Fog Enabled", &fogOptions_.enabled);
            changed |= ImGui::SliderFloat("Fog Distance", &fogOptions_.distance, 0.0f, 200.0f);
            changed |= ImGui::SliderFloat("Fog Density", &fogOptions_.density, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Fog Height", &fogOptions_.height, -50.0f, 50.0f);
            changed |= ImGui::SliderFloat("Fog Height Falloff", &fogOptions_.heightFalloff, 0.0f, 4.0f);
            changed |= ImGui::ColorEdit3("Fog Color", &fogOptions_.color[0]);

            changed |= ImGui::Checkbox("Vignette Enabled", &vignetteOptions_.enabled);
            changed |= ImGui::SliderFloat("Vignette Midpoint", &vignetteOptions_.midPoint, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Vignette Roundness", &vignetteOptions_.roundness, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Vignette Feather", &vignetteOptions_.feather, 0.0f, 1.0f);
        }

        if (ImGui::CollapsingHeader("Shadow Filters")) {
            int anisotropy = static_cast<int>(vsmShadowOptions_.anisotropy);
            if (ImGui::SliderInt("VSM Anisotropy", &anisotropy, 0, 4)) {
                vsmShadowOptions_.anisotropy = static_cast<uint8_t>(anisotropy);
                changed = true;
            }
            changed |= ImGui::Checkbox("VSM Mipmapping", &vsmShadowOptions_.mipmapping);
            int vsmMsaa = static_cast<int>(vsmShadowOptions_.msaaSamples);
            if (ImGui::SliderInt("VSM MSAA Samples", &vsmMsaa, 1, 8)) {
                vsmShadowOptions_.msaaSamples = static_cast<uint8_t>(vsmMsaa);
                changed = true;
            }
            changed |= ImGui::Checkbox("VSM High Precision", &vsmShadowOptions_.highPrecision);
            changed |= ImGui::SliderFloat("VSM Min Variance", &vsmShadowOptions_.minVarianceScale, 0.01f, 2.0f);
            changed |= ImGui::SliderFloat("VSM Light Bleed Reduction", &vsmShadowOptions_.lightBleedReduction, 0.0f, 1.0f);
            changed |= ImGui::SliderFloat("Soft Penumbra Scale", &softShadowOptions_.penumbraScale, 0.1f, 3.0f);
            changed |= ImGui::SliderFloat("Soft Penumbra Ratio", &softShadowOptions_.penumbraRatioScale, 1.0f, 4.0f);
        }
    }
    ImGui::End();

    if (changed) {
        filamentSettingsDirty_ = true;
        ApplyFilamentUiState();
    }
}
