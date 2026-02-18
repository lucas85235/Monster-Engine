#pragma once

#include <functional>
#include <string>
#include <unordered_map>

namespace se::ui::flutter {

/**
 * Bidirectional message channel between C++ and Dart.
 *
 * Uses Flutter platform channels with a JSON message codec to send
 * and receive method calls.
 *
 * On macOS, this wraps FlutterMethodChannel from FlutterMacOS.framework
 * via Objective-C++ bridging in the .mm implementation file.
 *
 * Typical usage:
 * @code
 *   // Register a handler for the "monster/scene" channel
 *   channel.RegisterChannel("monster/scene",
 *       [](const std::string& method, const std::string& args,
 *          std::function<void(const std::string&)> reply) {
 *           if (method == "getEntities") {
 *               reply("{\"entities\": [...]}");
 *           }
 *       });
 *
 *   // Send a message from C++ to Dart
 *   channel.SendMessage("monster/scene", "entityCreated", "{\"id\": 42}");
 * @endcode
 */
class FlutterPlatformChannel {
public:
    /**
     * Handler signature for incoming method calls from Dart.
     *
     * @param method  Method name invoked by the Dart side.
     * @param args    JSON-encoded arguments string.
     * @param reply   Callback to send a JSON response back to Dart.
     */
    using ChannelHandler = std::function<void(
        const std::string& method,
        const std::string& args,
        std::function<void(const std::string&)> reply)>;

    FlutterPlatformChannel() = default;
    ~FlutterPlatformChannel() = default;

    // Disallow copy/move.
    FlutterPlatformChannel(const FlutterPlatformChannel&) = delete;
    FlutterPlatformChannel& operator=(const FlutterPlatformChannel&) = delete;

    /**
     * Initialize with an opaque pointer to the native Flutter engine.
     *
     * On macOS, this is a (__bridge void*) FlutterEngine*.
     */
    void Init(void* flutterEngine);

    /** Shutdown and unregister all handlers. */
    void Shutdown();

    /**
     * Register a handler for a named platform channel.
     *
     * @param channel  Channel name (e.g. "monster/scene").
     * @param handler  Callback invoked when the Dart side sends a message.
     */
    void RegisterChannel(const std::string& channel, ChannelHandler handler);

    /**
     * Remove the handler for a named platform channel.
     */
    void UnregisterChannel(const std::string& channel);

    /**
     * Send a method call from C++ to the Dart side.
     *
     * @param channel  Target channel name.
     * @param method   Method name to invoke.
     * @param args     JSON-encoded arguments (may be empty "{}").
     */
    void SendMessage(const std::string& channel,
                     const std::string& method,
                     const std::string& args);

private:
    struct ChannelEntry {
        ChannelHandler handler;
        void*          objc_channel = nullptr;  // Bridged FlutterMethodChannel*
    };

    void*  engine_ = nullptr;
    bool   initialized_ = false;
    std::unordered_map<std::string, ChannelEntry> channels_;
};

}  // namespace se::ui::flutter
