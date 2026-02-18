#pragma once

#include "engine/Layer.h"

#if defined(SE_ENABLE_FLUTTER) && SE_ENABLE_FLUTTER

namespace se::ui::flutter {
class FlutterEmbedder;
}

namespace se {

/**
 * Test layer to verify that the Flutter embedder integration is working.
 *
 * When attached, it registers a platform channel ("monster/test") that
 * responds to method calls from the Dart side, and periodically sends
 * ticks to the Dart UI for visual feedback.
 *
 * This layer also draws an ImGui debug window (if ImGui is enabled)
 * showing the Flutter embedder status and platform channel activity.
 */
class FlutterTestLayer final : public Layer {
public:
    FlutterTestLayer();
    ~FlutterTestLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnImGuiRender() override;

private:
    ui::flutter::FlutterEmbedder* flutter_ = nullptr;
    float accumulated_time_ = 0.0f;
    int messages_sent_ = 0;
    int messages_received_ = 0;
    std::string last_dart_response_;
};

}  // namespace se

#endif  // SE_ENABLE_FLUTTER
