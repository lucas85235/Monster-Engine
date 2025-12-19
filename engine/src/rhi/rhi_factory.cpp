#include <stdexcept>

#include "engine/rhi/opengl/opengl_device.h"
#include "engine/rhi/rhi_device.h"
#include "engine/rhi/vulkan/vulkan_device.h"

namespace RHI {

std::unique_ptr<IDevice> DeviceFactory::Create(API api) {
    switch (api) {
        case API::OpenGL:
            return std::make_unique<OpenGLDevice>();
        case API::Vulkan:
            return std::make_unique<VulkanDevice>();
        default:
            throw std::runtime_error("Unknown Graphics API requested");
    }
}

}  // namespace RHI
