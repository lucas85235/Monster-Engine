#pragma once
#include <engine/utils/FilesHandler.h>

#include <glm.hpp>

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

    unsigned int getID() const {
        return program_;
    }
    unsigned int release() {
        unsigned int id = program_;
        program_        = 0;
        return id;
    }

   private:
    unsigned int program_ = 0;

    static unsigned int compileStage(unsigned int type, const char* src);
    static void         checkCompile(unsigned int id, bool isProgram);
    int                 uniformLocation(const char* name) const;
};

}  // namespace se
