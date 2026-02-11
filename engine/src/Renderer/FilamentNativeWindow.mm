/**
 * Platform-specific native window handle extraction for Filament.
 *
 * On macOS with the Metal backend, Filament expects a CAMetalLayer*
 * as the native window handle for SwapChain creation.
 *
 * This Objective-C++ file extracts the NSView* from a GLFW window,
 * sets its layer to a CAMetalLayer, and returns the layer pointer.
 */

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* GetCocoaNativeWindow(void* glfwWindow) {
    if (!glfwWindow) return nullptr;

    GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
    NSWindow* nsWindow = glfwGetCocoaWindow(window);
    if (!nsWindow) return nullptr;

    NSView* view = [nsWindow contentView];
    if (!view) return nullptr;

    // Filament Metal backend requires a CAMetalLayer* as native handle.
    // We make the NSView layer-backed and replace its layer with a CAMetalLayer.
    [view setWantsLayer:YES];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.contentsScale = [nsWindow backingScaleFactor];
    [view setLayer:metalLayer];

    return (__bridge void*)metalLayer;
}
