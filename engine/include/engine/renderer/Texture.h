#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace se {

enum class TextureType {
    Albedo,
    Normal,
    Specular,
    AO,
    Emissive,
    Roughness,
    Metallic,
    Unknown
};

class Texture {
   public:
    Texture() = default;
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    static std::shared_ptr<Texture> Create(const std::string& path);

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetId() const { return id_; }
    const std::string& GetPath() const { return path_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetChannels() const { return channels_; }
    bool IsValid() const { return id_ != 0; }

   private:
    bool LoadFromFile(const std::string& path);
    void Release();

    uint32_t id_ = 0;
    std::string path_;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
};

}  // namespace se
