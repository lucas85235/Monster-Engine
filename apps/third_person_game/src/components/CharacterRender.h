#pragma once
/**
 * CharacterRender.h - Character visual representation.
 *
 * This component manages:
 * - Mesh rendering
 * - Material setup
 * - Debug visualization
 *
 * Requires: TransformComponent
 * Adds: MeshRenderComponent
 */

#include "engine/ecs/Component.h"
#include "engine/renderer/Material.h"

namespace FirstGame {
using namespace se;

struct RenderConfig {
    bool enablePhysicsDebug = true;
};

class CharacterRender : public Component {
public:
    CharacterRender() = default;
    ~CharacterRender() override = default;

    void Awake() override;
    void Update(float dt) override;

    RenderConfig& GetConfig() { return config_; }

private:
    void SetupMesh();
    void SetupDebugVisualization();

    Ref<Material> material_;
    RenderConfig  config_;
};

} // namespace FirstGame