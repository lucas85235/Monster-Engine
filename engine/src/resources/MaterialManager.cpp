#include "engine/resources/MaterialManager.h"

#include "engine/core/Log.h"

namespace se {
std::shared_ptr<Material>                                MaterialManager::defaultMaterial_;
std::shared_ptr<Shader>                                  MaterialManager::defaultShader_;
std::unordered_map<std::string, std::shared_ptr<Shader>> MaterialManager::shaderCache_;
bool                                                     MaterialManager::initialized_ = false;

void MaterialManager::Init() {
    if (initialized_) {
        SE_LOG_WARN("MaterialManager already initialized");
        return;
    }

    SE_LOG_INFO("Initializing MaterialManager");

    CreateDefaultShader();

    if (!defaultShader_) {
        SE_LOG_ERROR("Failed to create default shader!");
        throw std::runtime_error("Failed to create default shader");
    }

    // Create default white texture
    whiteTexture_ = Texture::CreateWhiteTexture();

    // Create default material and assign white texture
    defaultMaterial_ = std::make_shared<Material>(defaultShader_);

    // Assign White Texture to Default Material to ensure untextured objects render white (not black)
    // if default shader is updated to support textures.
    if (defaultMaterial_ && whiteTexture_) { defaultMaterial_->SetTexture("uTexture", whiteTexture_->GetHandle()); }

    SE_LOG_INFO("MaterialManager initialized successfully");
    initialized_ = true;
}

void MaterialManager::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down MaterialManager");

    ClearCache();
    defaultMaterial_.reset();
    defaultShader_.reset();

    initialized_ = false;
}

std::shared_ptr<Material> MaterialManager::GetDefaultMaterial() {
    if (!initialized_) {
        SE_LOG_ERROR("MaterialManager not initialized!");
        return nullptr;
    }

    if (!defaultMaterial_) {
        SE_LOG_ERROR("Default material is null!");
        return nullptr;
    }

    return defaultMaterial_;
}

std::shared_ptr<Material> MaterialManager::CreateMaterial(std::shared_ptr<Shader> shader) {
    if (!shader) {
        SE_LOG_WARN("Creating material with null shader, using default");
        return GetDefaultMaterial();
    }

    return std::make_shared<Material>(shader);
}

std::shared_ptr<Shader> MaterialManager::GetShader(const std::string& name, const std::filesystem::path& vertPath,
                                                   const std::filesystem::path& fragPath) {
    if (!initialized_) {
        SE_LOG_ERROR("MaterialManager not initialized!");
        return nullptr;
    }

    // Check cache
    auto it = shaderCache_.find(name);
    if (it != shaderCache_.end()) {
        // SE_LOG_INFO("Shader '{}' found in cache", name);
        return it->second;
    }

    // Load and cache shader
    try {
        auto shader        = Shader::CreateFromFiles(vertPath, fragPath);
        shaderCache_[name] = shader;
        // SE_LOG_INFO("Loaded and cached shader: {}", name);
        return shader;
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to load shader '{}': {}", name, e.what());
        return defaultShader_;
    }
}

void MaterialManager::CreateDefaultShader() {
    SE_LOG_INFO("Creating default shader (loading from SPIR-V)...");

    // Load basic shader from SPIR-V assets
    // This shader supports textures, lighting, and shadow maps
    // Path: assets/shaders/spirv/basic.vert.spv and basic.frag.spv

    // We assume the assets are in relative path "assets/shaders/spirv/"
    // But Shader::CreateFromFiles expects paths.
    // Let's use std::filesystem::path

    std::filesystem::path vertPath = "assets/shaders/basic.vert";
    std::filesystem::path fragPath = "assets/shaders/basic.frag";

    try {
        defaultShader_ = Shader::CreateFromFiles(vertPath, fragPath);
        SE_LOG_INFO("Default shader loaded successfully (ID: {})", defaultShader_->GetHandle().id);
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Failed to load default shader: {}", e.what());

        // Fallback to hardcoded simple shader if file load fails?
        // Or just re-throw. The app likely needs this shader.
        SE_LOG_WARN("Attempting fallback to hardcoded shader...");

        // Fallback: Simple Vertex/Fragment shader (Vertex Color only)
        const std::string vertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Color;
        layout(location = 2) in vec3 a_Normal;

        uniform mat4 uView;
        uniform mat4 uProj;
        uniform mat4 uModel;

        out vec3 v_Color;

        void main() {
            v_Color = a_Color;
            gl_Position = uProj * uView * uModel * vec4(a_Position, 1.0);
        }
        )";

        const std::string fragmentSrc = R"(
            #version 330 core
            layout(location = 0) out vec4 color;
            in vec3 v_Color;
            void main() {
                color = vec4(v_Color, 1.0);
            }
        )";

        defaultShader_ = std::make_shared<Shader>(vertexSrc, fragmentSrc);
    }
}

void MaterialManager::ClearCache() {
    shaderCache_.clear();
    SE_LOG_INFO("MaterialManager cache cleared");
}

std::shared_ptr<Texture> MaterialManager::GetWhiteTexture() {
    if (!whiteTexture_) { whiteTexture_ = Texture::CreateWhiteTexture(); }
    return whiteTexture_;
}

std::shared_ptr<Texture> MaterialManager::whiteTexture_ = nullptr;

}  // namespace se
