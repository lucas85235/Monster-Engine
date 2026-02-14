#include "ThirdPersonLayer.h"

#include <btBulletDynamicsCommon.h>
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
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

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

    // Disable physics debug drawing by default
    scene_->GetPhysicsSystem()->GetDebugDrawer()->setDebugMode(btIDebugDraw::DBG_NoDebug);
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
        floor_entity_.AddComponent<RigidbodyComponent>(data, floor_entity_);

        auto meshData = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
        floorRenderable_ = meshes.CreateRenderable(meshData, floorMaterial_, false);
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
            wall.AddComponent<RigidbodyComponent>(data, wall);

            auto renderable = meshes.CreateRenderable(wallMesh, wallMaterial_, false);
            wallRenderables_.push_back(renderable);
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
        auto& rb = playerEntity_.AddComponent<RigidbodyComponent>(data, playerEntity_);
        rb.SetAngularFactor({0.0f, 1.0f, 0.0f});
    }

    // ─── Physics Cube ───────────────────────────────────────────
    {
        cube_entity_ = scene_->CreateEntity("Cube");
        cube_entity_.GetComponent<TransformComponent>().SetPosition({0.0f, 10.0f, 0.0f});

        cube_entity_.AddComponent<BoxCollider>(Vector3(1.0f, 1.0f, 1.0f));
        RigidbodyData data;
        cube_entity_.AddComponent<RigidbodyComponent>(data, cube_entity_);

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
            wall.AddComponent<RigidbodyComponent>(data, wall);

            auto renderable = meshes.CreateRenderable(wallMesh, wallMaterial_, false);
            smallWallRenderables_.push_back(renderable);
            smallWalls_.push_back(wall);
        }

        SE_LOG_INFO("Created {} small walls for occlusion testing", smallWallDefs.size());
    }
}

void ThirdPersonLayer::SyncPhysicsToRenderables() {
    auto& meshes = ServiceLocator::Get().GetMeshSystem();

    // Sync floor (static)
    {
        auto& t = floor_entity_.GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(floorRenderable_, mat);
    }

    // Sync walls (static)
    for (size_t i = 0; i < walls_.size(); i++) {
        auto& t = walls_[i].GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(wallRenderables_[i], mat);
    }

    // Sync small walls (static)
    for (size_t i = 0; i < smallWalls_.size(); i++) {
        auto& t = smallWalls_[i].GetComponent<TransformComponent>();
        float mat[16];
        buildTransformMatrix(mat, t.Position, t.Rotation, t.Scale);
        meshes.SetTransform(smallWallRenderables_[i], mat);
    }

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
    auto& rb  = box.AddComponent<RigidbodyComponent>(data, box);

    btVector3 impulse(forward.x, forward.y, forward.z);
    impulse *= 50.0f;
    rb.GetRigidbody()->applyCentralImpulse(impulse);

    // Create Filament renderable for the bullet
    auto& meshes = ServiceLocator::Get().GetMeshSystem();
    auto bulletMesh = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
    auto renderable = meshes.CreateRenderable(bulletMesh, bulletMaterial_);
    bulletRenderables_.push_back(renderable);

    bullets_.push_back(box);
}

void ThirdPersonLayer::UpdateCamera() {
    auto& input = InputManager::Get();

    auto& playerTrans = playerEntity_.GetComponent<TransformComponent>();

    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");

    springArmYaw_ -= mouseX * 0.1f;
    springArmPitch_ -= mouseY * 0.1f;
    springArmPitch_ = glm::clamp(springArmPitch_, -80.0f, 80.0f);

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
