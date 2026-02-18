#include "engine/ui/flutter/FlutterPlatformChannel.h"

#import <FlutterMacOS/FlutterMacOS.h>

#include "engine/Log.h"

namespace se::ui::flutter {

// ---- Initialise / Shutdown ------------------------------------------------

void FlutterPlatformChannel::Init(void* flutterEngine) {
    engine_ = flutterEngine;
    if (!engine_) {
        SE_LOG_ERROR("FlutterPlatformChannel::Init — null engine");
        return;
    }
    initialized_ = true;
    SE_LOG_INFO("FlutterPlatformChannel initialized");
}

void FlutterPlatformChannel::Shutdown() {
    channels_.clear();
    engine_ = nullptr;
    initialized_ = false;
}

// ---- Channel Registration -------------------------------------------------

void FlutterPlatformChannel::RegisterChannel(
    const std::string& channelName,
    ChannelHandler handler) {
    if (!initialized_ || !engine_) return;

    FlutterEngine* engine = (__bridge FlutterEngine*)engine_;
    NSString* nsChannelName = [NSString stringWithUTF8String:channelName.c_str()];

    FlutterMethodChannel* channel =
        [FlutterMethodChannel methodChannelWithName:nsChannelName
                                    binaryMessenger:engine.binaryMessenger
                                              codec:[FlutterJSONMethodCodec sharedInstance]];

    // Store the C++ handler and set up the Obj-C callback.
    channels_[channelName] = {handler, (__bridge_retained void*)channel};

    [channel setMethodCallHandler:^(FlutterMethodCall* call, FlutterResult result) {
        std::string method = [call.method UTF8String];
        std::string args;
        if (call.arguments) {
            if ([call.arguments isKindOfClass:[NSString class]]) {
                args = [call.arguments UTF8String];
            } else if ([call.arguments isKindOfClass:[NSDictionary class]] ||
                       [call.arguments isKindOfClass:[NSArray class]]) {
                NSData* jsonData = [NSJSONSerialization dataWithJSONObject:call.arguments
                                                                 options:0
                                                                   error:nil];
                if (jsonData) {
                    args = std::string((const char*)jsonData.bytes, jsonData.length);
                }
            }
        }

        auto reply = [result](const std::string& response) {
            // Parse JSON response string back to Foundation object
            NSData* data = [NSData dataWithBytes:response.c_str() length:response.size()];
            id jsonObj = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
            result(jsonObj ?: [NSNull null]);
        };

        handler(method, args, reply);
    }];

    SE_LOG_INFO("FlutterPlatformChannel: registered channel '{}'", channelName);
}

void FlutterPlatformChannel::UnregisterChannel(const std::string& channelName) {
    auto it = channels_.find(channelName);
    if (it != channels_.end()) {
        // Release the bridged FlutterMethodChannel
        FlutterMethodChannel* channel = (__bridge_transfer FlutterMethodChannel*)it->second.objc_channel;
        [channel setMethodCallHandler:nil];
        channels_.erase(it);
        SE_LOG_INFO("FlutterPlatformChannel: unregistered channel '{}'", channelName);
    }
}

// ---- Send Messages --------------------------------------------------------

void FlutterPlatformChannel::SendMessage(
    const std::string& channelName,
    const std::string& method,
    const std::string& arguments) {
    auto it = channels_.find(channelName);
    if (it == channels_.end()) return;

    FlutterMethodChannel* channel = (__bridge FlutterMethodChannel*)it->second.objc_channel;
    NSString* nsMethod = [NSString stringWithUTF8String:method.c_str()];

    // Parse args JSON string to Foundation object
    id argsObj = nil;
    if (!arguments.empty()) {
        NSData* data = [NSData dataWithBytes:arguments.c_str() length:arguments.size()];
        argsObj = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    }

    [channel invokeMethod:nsMethod arguments:argsObj];
}

}  // namespace se::ui::flutter
