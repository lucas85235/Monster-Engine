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
    
    static std::shared_ptr<Texture> CreateFromMemory(
        const uint8_t* data,
        uint32_t width,
        uint32_t height,
        uint8_t channels,
        const std::string& debugName = "embedded"
    );

    void Bind(uint32_t slot = 0) const;
    void Unbind() const;

    uint32_t GetId() const { return id_; }
    const std::string& GetPath() const { return path_; }
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    int GetChannels() const { return channels_; }
    bool IsValid() const { return id_ != 0; }

   public:
    void SetFromGLHandle(uint32_t glHandle, int width, int height, int channels, const std::string& debugName);
    
   private:
    bool LoadFromFile(const std::string& path);
    bool LoadFromMemory(const uint8_t* data, uint32_t width, uint32_t height, uint8_t channels);
    void Release();

    uint32_t id_ = 0;
    std::string path_;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
};

}  // namespace se
