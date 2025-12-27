#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class Texture;

class TextureManager {
   public:
    static void Init();
    static void Shutdown();

    static std::shared_ptr<Texture> Load(const std::string& path);
    static std::shared_ptr<Texture> Get(const std::string& name);
    static bool Has(const std::string& name);
    static bool FileExists(const std::string& path);
    static void Unload(const std::string& name);
    static void ClearCache();

   private:
    static std::unordered_map<std::string, std::shared_ptr<Texture>> cache_;
    static bool initialized_;
};

}  // namespace se
