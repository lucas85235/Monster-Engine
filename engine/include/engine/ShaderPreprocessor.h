#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>

namespace se {

/**
 * Simple GLSL preprocessor that handles #include directives.
 * OpenGL 3.3 does not support #include natively, so we must preprocess shaders.
 */
class ShaderPreprocessor {
   public:
    /**
     * Process a shader source string, resolving all #include directives.
     * @param source The shader source code.
     * @param basePath The directory containing the shader file (for resolving relative includes).
     * @return The processed shader source with all includes expanded.
     */
    static std::string Process(const std::string& source, const std::filesystem::path& basePath);

    /**
     * Process a shader file, loading and resolving all #include directives.
     * @param filePath Path to the shader file.
     * @return The processed shader source with all includes expanded.
     */
    static std::string ProcessFile(const std::filesystem::path& filePath);

   private:
    static std::string ProcessInternal(const std::string& source, 
                                       const std::filesystem::path& basePath,
                                       std::unordered_set<std::string>& includedFiles,
                                       int depth);
    
    static constexpr int MAX_INCLUDE_DEPTH = 16;
};

}  // namespace se
