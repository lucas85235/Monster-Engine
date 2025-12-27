#include "CharacterRender.h"

#include "apps/SampleUtilities.h"
#include "engine/Application.h"
#include "engine/ecs/ModelComponent.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/resources/Model.h"
#include "engine/resources/ModelManager.h"
#include "LinearMath/btIDebugDraw.h"

namespace FirstGame {
void CharacterRender::Awake() {
    // NOTE: Do NOT load models here - OpenGL context may not be ready!
    // Model loading moved to Start() which runs in the main game loop
    SetupDebugVisualization();
    SE_LOG_INFO("CharacterRender::Awake() - Initialized");
}

void CharacterRender::Start() {
    // Load model in Start() to ensure OpenGL context is ready
    SetupMesh();
    SE_LOG_INFO("CharacterRender::Start() - Model loaded");
}

void CharacterRender::Update(float dt) {
    // Animation updates, visual effects, etc.
}

void CharacterRender::SetupMesh() {
    const std::string modelPath = "assets/models/characters/skeleton/SKM_Skeleton_Variant_1.fbx";
    auto              model     = ModelManager::Load(modelPath);

    if (!model) {
        SE_LOG_ERROR("CharacterRender: Failed to load model from '{}'", modelPath);
        return;
    }

    // Create child entity for the visual model
    // This allows separating physics (parent) from visual transform (child)
    auto visualEntity = GetScene()->CreateEntity("CharacterModel");
    visualEntity.SetParent(GetEntity());
    
    visualEntity.AddComponent<ModelComponent>(model);

    // Apply transform corrections for FBX model on the child entity
    auto& transform = visualEntity.GetComponent<TransformComponent>();
    transform.SetScale(glm::vec3(1.0f));

    // FBX models often have different forward direction - rotate to face forward (-Z in our engine)
    // Rotate -90 degrees on X axis to correct orientation (Standard Z-up to Y-up correction)
    glm::vec3 currentRotation = transform.Rotation;
    currentRotation.x         = 90.0f;
    currentRotation.y         = 90.0f;
    transform.SetPosition(glm::vec3(0.0f, -0.8f, 0.0f));
    transform.SetRotation(currentRotation);

    SE_LOG_INFO("CharacterRender: Loaded model '{}' with {} submeshes (Child Entity)",
                model->GetName(), model->GetSubMeshCount());
}

void CharacterRender::SetupDebugVisualization() {
    if (!config_.enablePhysicsDebug) return;

    Scene* scene = GetScene();
    if (!scene || !scene->GetPhysicsSystem()) {
        SE_LOG_WARN("CharacterRender: Cannot enable debug - no physics system");
        return;
    }

    auto* debugDrawer = scene->GetPhysicsSystem()->GetDebugDrawer();
    if (debugDrawer) {
        debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
        SE_LOG_INFO("CharacterRender: Physics debug enabled");
    }
}
} // namespace FirstGame