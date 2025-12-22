#pragma once

#include <filesystem>
#include <fstream>

#include "engine/core/FileSystem.h"
#include "engine/core/Log.h"

namespace se {

namespace fs = std::filesystem;

class NativeFileSystem : public IFileSystem {
   public:
    bool Exists(const std::string& path) const override {
        return fs::exists(ResolvePath(path));
    }

    std::vector<uint8_t> ReadFile(const std::string& path) const override {
        std::string   fullPath = ResolvePath(path);
        std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            SE_LOG_ERROR("[VFS] Failed to open binary file: {}", fullPath);
            return {};
        }

        size_t               fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint8_t> buffer(fileSize);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        return buffer;
    }

    std::string ReadTextFile(const std::string& path) const override {
        std::string   fullPath = ResolvePath(path);
        std::ifstream file(fullPath);

        if (!file.is_open()) {
            SE_LOG_ERROR("[VFS] Failed to open text file: {}", fullPath);
            return "";
        }

        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    std::string GetAbsolutePath(const std::string& path) const override {
        return ResolvePath(path);
    }

   private:
    std::string ResolvePath(const std::string& path) const {
        fs::path p(path);

        if (p.is_absolute() && fs::exists(p)) return p.string();

        if (fs::exists(p)) return fs::absolute(p).string();

#ifdef PROJECT_SOURCE_DIR
        fs::path root(PROJECT_SOURCE_DIR);
        if (fs::exists(root / p)) return (root / p).string();
#endif

        return path;
    }
};

}  // namespace se
