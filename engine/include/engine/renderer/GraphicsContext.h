#pragma once

struct GLFWwindow;

#include <memory>

#include "engine/rhi/rhi_device.h"

namespace se {

class GraphicsContext {
   public:
    GraphicsContext(GLFWwindow* windowHandle, RHI::API api);
    ~GraphicsContext();

    void        Init();
    void        SwapBuffers();
    GLFWwindow* GetContext() {
        return windowHandle_;
    }

    RHI::IDevice* GetDevice() {
        return device_.get();
    }
    RHI::API GetAPI() const {
        return api_;
    }

   private:
    GLFWwindow*                   windowHandle_;
    RHI::API                      api_;
    std::unique_ptr<RHI::IDevice> device_;
};

}  // namespace se