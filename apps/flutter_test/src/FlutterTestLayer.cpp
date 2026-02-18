#include "FlutterTestLayer.h"

#if defined(SE_ENABLE_FLUTTER) && SE_ENABLE_FLUTTER

#include <engine/Log.h>
#include <engine/core/ServiceLocator.h>
#include <engine/ui/flutter/FlutterEmbedder.h>
#include <engine/ui/flutter/FlutterPlatformChannel.h>

namespace se {

FlutterTestLayer::FlutterTestLayer()
    : Layer("FlutterTestLayer") {}

void FlutterTestLayer::OnAttach() {
    flutter_ = ServiceLocator::Get().GetFlutterEmbedderPtr();
    if (!flutter_) {
        SE_LOG_WARN("FlutterTestLayer: Flutter embedder not available");
        return;
    }

    SE_LOG_INFO("FlutterTestLayer: Registering 'monster/test' platform channel");

    // Register a test platform channel that the Dart side can call.
    flutter_->GetPlatformChannel().RegisterChannel("monster/test",
        [this](const std::string& method, const std::string& args,
               std::function<void(const std::string&)> reply) {
            messages_received_++;
            SE_LOG_INFO("FlutterTestLayer: Received from Dart — method='{}', args='{}'",
                        method, args);

            if (method == "ping") {
                reply("{\"status\": \"pong\", \"engineName\": \"Monster Engine\"}");
            } else if (method == "getEngineInfo") {
                reply("{\"name\": \"Monster Engine\","
                      " \"renderer\": \"Filament\","
                      " \"flutter\": true,"
                      " \"version\": \"0.1.0\"}");
            } else {
                reply("{\"error\": \"unknown method\", \"method\": \"" + method + "\"}");
            }
        });

    SE_LOG_INFO("FlutterTestLayer attached — Flutter integration active");
}

void FlutterTestLayer::OnDetach() {
    if (flutter_) {
        flutter_->GetPlatformChannel().UnregisterChannel("monster/test");
    }
    flutter_ = nullptr;
    SE_LOG_INFO("FlutterTestLayer detached");
}

void FlutterTestLayer::OnUpdate(float ts) {
    if (!flutter_ || !flutter_->IsInitialized()) return;

    accumulated_time_ += ts;

    // Send a tick message to Dart every 2 seconds for visual feedback.
    if (accumulated_time_ >= 2.0f) {
        accumulated_time_ -= 2.0f;
        messages_sent_++;

        std::string tickArgs = "{\"tick\": " + std::to_string(messages_sent_) +
                               ", \"fps\": " + std::to_string(static_cast<int>(1.0f / ts)) + "}";
        flutter_->GetPlatformChannel().SendMessage("monster/test", "engineTick", tickArgs);
    }
}

void FlutterTestLayer::OnImGuiRender() {
#ifdef SE_ENABLE_IMGUI
    // This requires ImGui to be enabled. If not, it's a no-op.
    // The actual ImGui calls would go here, but we keep it simple
    // to avoid pulling imgui.h into this translation unit unnecessarily.
#endif

    // Even without ImGui, we log status periodically (handled in OnUpdate).
}

}  // namespace se

#endif  // SE_ENABLE_FLUTTER
