#include "CharacterRender.h"

#include "engine/ecs/SimpleComponents.h"
#include "engine/resources/MeshManager.h"
#include "apps/SampleUtilities.h"
#include "engine/Application.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "LinearMath/btIDebugDraw.h"

namespace FirstGame {
void CharacterRender::Awake() {
    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
    material_ = Utilities::LoadMaterial();
    if (!material_) {
        SE_LOG_ERROR("Material not loaded, retrying");
        material_ = Utilities::LoadMaterial();
    }

    GetEntity().AddComponent<MeshRenderComponent>(mesh, material_);

    // Enable physics debug wireframe - use GetScene() from Component base class
    if (GetScene() && GetScene()->GetPhysicsSystem()) {
        GetScene()->GetPhysicsSystem()->GetDebugDrawer()->setDebugMode(
            btIDebugDraw::DBG_DrawWireframe);
        SE_LOG_INFO("CharacterRender: Physics debug draw enabled");
    } else {
        SE_LOG_WARN(
            "CharacterRender: Could not enable physics debug draw - scene or physics system not available");
    }
}

void CharacterRender::Update(float dt) {
    // CharacterRender-specific update logic here
}
} // FirstGame