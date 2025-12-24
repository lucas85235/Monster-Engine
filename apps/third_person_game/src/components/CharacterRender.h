#pragma once
#include "engine/ecs/Component.h"
#include "engine/renderer/Material.h"

namespace FirstGame {
using namespace se;

class CharacterRender : public Component {
public:
    void Awake() override;

    void Update(float dt) override;

private:
    Ref<Material> material_;
};
} // FirstGame