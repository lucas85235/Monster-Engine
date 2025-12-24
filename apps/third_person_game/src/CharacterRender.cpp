#include "apps/third_person_game/src/CharacterRender.h"

#include "engine/ecs/SimpleComponents.h"
#include "engine/resources/MeshManager.h"
#include "apps/SampleUtilities.h"

namespace FirstGame {
void CharacterRender::Awake() {
    Component::Awake();

    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
    material_ = Utilities::LoadMaterial();
    if (!material_) {
        SE_LOG_ERROR("Material not loaded, retrying");
        material_ = Utilities::LoadMaterial();
    }

    GetEntity().AddComponent<MeshRenderComponent>(mesh, material_);
}

void CharacterRender::Update(float dt) {
    Component::Update(dt);
}
} // FirstGame