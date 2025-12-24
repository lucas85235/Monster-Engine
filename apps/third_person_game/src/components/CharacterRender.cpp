#include "CharacterRender.h"

#include "apps/SampleUtilities.h"
#include "engine/Application.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/resources/MeshManager.h"
#include "LinearMath/btIDebugDraw.h"

namespace FirstGame {

void CharacterRender::Awake() {
    SetupMesh();
    SetupDebugVisualization();

    SE_LOG_INFO("CharacterRender::Awake() - Visual setup complete");
}

void CharacterRender::Update(float dt) {
    // Animation updates, visual effects, etc.
}

void CharacterRender::SetupMesh() {
    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);

    material_ = Utilities::LoadMaterial();
    if (!material_) {
        SE_LOG_ERROR("CharacterRender: Failed to load material");
        return;
    }

    GetEntity().AddComponent<MeshRenderComponent>(mesh, material_);
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