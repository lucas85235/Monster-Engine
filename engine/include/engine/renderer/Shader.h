#pragma once
#include <engine/utils/FilesHandler.h>

#include <glm.hpp>

#include "engine/rhi/rhi_types.h"
#include "se_pch.h"

namespace se {

class Shader {
   public:
    Shader(const std::string& vertSrc, const std::string& fragSrc);

    static std::shared_ptr<Shader> CreateFromFiles(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath);

    static Shader fromFiles(const std::filesystem::path& vertPath, const std::filesystem::path& fragPath) {
        return Shader(readFileToString(vertPath), readFileToString(fragPath));
    }

    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    void bind() const;
    void unbind() const;

    // Minimal uniform helper (float)
    void setFloat(const char* name, float value) const;
    void setInt(const char* name, int value) const;
    void setVec3(const char* name, const Vector3& value) const;
    void setVec4(const char* name, const Vector4& value) const;
    void setMat4(const char* name, const Matrix4& value) const;

    RHI::ShaderHandle GetHandle() const {
        return handle_;
    }

   private:
    RHI::ShaderHandle handle_ = {0};
};

}  // namespace se
