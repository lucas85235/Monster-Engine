/**
 * Platform-specific native window handle extraction for Filament.
 *
 * On macOS, Filament needs either:
 * - NSView* for OpenGL backend
 * - CAMetalLayer* for Metal backend (Filament handles this internally from NSView*)
 *
 * This Objective-C++ file extracts the NSView* from a GLFW window.
 */

#import <Cocoa/Cocoa.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

extern "C" void* GetCocoaNativeWindow(void* glfwWindow) {
    if (!glfwWindow) return nullptr;

    GLFWwindow* window = static_cast<GLFWwindow*>(glfwWindow);
    NSWindow* nsWindow = glfwGetCocoaWindow(window);
    if (!nsWindow) return nullptr;

    NSView* view = [nsWindow contentView];
    return (__bridge void*)view;
}
