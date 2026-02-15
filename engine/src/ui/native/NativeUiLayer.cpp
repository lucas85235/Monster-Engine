#include "engine/ui/native/NativeUiLayer.h"

#include "engine/input/InputManager.h"
#include "engine/ui/native/NativeUiRenderer.h"
#include "engine/ui/native/retained/RetainedUi.h"

namespace se::ui {

NativeUiLayer::NativeUiLayer() : Layer("NativeUiLayer") {}

void NativeUiLayer::OnUpdate(float ts) {
    auto& retained = retained::RetainedUiContext::Get();
    if (!retained.IsInitialized() || !retained.HasUserContent()) {
        return;
    }

    retained.PollInput(InputManager::Get());
    retained.Tick(ts);
}

void NativeUiLayer::OnRender() {
    auto& retained = retained::RetainedUiContext::Get();
    if (!retained.IsInitialized() || !retained.HasUserContent()) {
        return;
    }

    auto& renderer = NativeUiRenderer::Get();
    if (!renderer.IsInitialized()) {
        return;
    }

    retained.Render(renderer);
}

}  // namespace se::ui
