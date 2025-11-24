#pragma once

#include "engine/Shader.h"
#include "engine/renderer/Material.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class MaterialManager {
  public:
    static void Init();
    static void Shutdown();

    // Get default material with basic shader
    static std::shared_ptr<Material> GetDefaultMaterial();

    // Create a material with custom shader
    static std::shared_ptr<Material> CreateMaterial(std::shared_ptr<Shader> shader);

    // Get or load a shader (cached)
    static std::shared_ptr<Shader> GetShader(const std::string& name,
                                             const std::filesystem::path& vertPath,
                                             const std::filesystem::path& fragPath);

    // Clear all cached resources
    static void ClearCache();

  private:
    MaterialManager() = delete;

    static void CreateDefaultShader();

    static std::shared_ptr<Material> defaultMaterial_;
    static std::shared_ptr<Shader> defaultShader_;
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaderCache_;
    static bool initialized_;
};

} // namespace se