#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/renderer/Material.h"
#include "engine/renderer/Shader.h"
#include "engine/renderer/Texture.h"

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
    static std::shared_ptr<Shader> GetShader(const std::string& name, const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);

    // Clear all cached resources
    static void ClearCache();

    // Get default white texture (1x1 white pixel)
    static std::shared_ptr<Texture> GetWhiteTexture();

   private:
    MaterialManager() = delete;

    static void CreateDefaultShader();

    static std::shared_ptr<Material>                                defaultMaterial_;
    static std::shared_ptr<Shader>                                  defaultShader_;
    static std::shared_ptr<Texture>                                 whiteTexture_;
    static std::unordered_map<std::string, std::shared_ptr<Shader>> shaderCache_;
    static bool                                                     initialized_;
};

}  // namespace se