#pragma once

#include <cstdint>
#include <memory>
#include <string>

struct GLFWwindow;

namespace se {

class GraphicsContext;

struct ApplicationSpec {
    std::string Name = "Simple Engine";
    uint32_t WindowWidth = 1280;
    uint32_t WindowHeight = 720;
    bool VSync = true;
};

class Window {
  public:
    Window(const ApplicationSpec& spec);
    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    void OnUpdate(); // Poll events

    uint32_t GetWidth() const {
        return width_;
    }

    uint32_t GetHeight() const {
        return height_;
    }

    void SetWidth(uint32_t width) {
        width_ = width;
    }

    void SetHeight(uint32_t height) {
        height_ = height;
    }

    void SetVSync(bool enabled);
    bool IsVSync() const {
        return vsync_;
    }

    bool ShouldClose() const;
    void RequestClose();

    auto* GetNativeWindow() const {
        return handle_;
    }

    void SwapBuffers();

  private:
    void Init(uint32_t width, uint32_t height, const std::string& title);
    void Shutdown();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

  private:
    GLFWwindow* handle_ = nullptr;
    std::unique_ptr<GraphicsContext> context_;

    uint32_t width_;
    uint32_t height_;
    std::string title_;
    bool vsync_ = true;
};

} // namespace se