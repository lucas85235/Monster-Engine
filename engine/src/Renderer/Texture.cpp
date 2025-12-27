#include "engine/renderer/Texture.h"

#include <glad/glad.h>

#include <algorithm>

#include "engine/Log.h"
#include <stb_image.h>

namespace se {

Texture::Texture(const std::string& path) {
    LoadFromFile(path);
}

Texture::~Texture() {
    Release();
}

Texture::Texture(Texture&& other) noexcept
    : id_(other.id_),
      path_(std::move(other.path_)),
      width_(other.width_),
      height_(other.height_),
      channels_(other.channels_) {
    other.id_ = 0;
    other.width_ = 0;
    other.height_ = 0;
    other.channels_ = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        Release();
        id_ = other.id_;
        path_ = std::move(other.path_);
        width_ = other.width_;
        height_ = other.height_;
        channels_ = other.channels_;
        other.id_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.channels_ = 0;
    }
    return *this;
}

std::shared_ptr<Texture> Texture::Create(const std::string& path) {
    auto texture = std::make_shared<Texture>(path);
    if (!texture->IsValid()) {
        return nullptr;
    }
    return texture;
}

void Texture::Bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture::Unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Texture::LoadFromFile(const std::string& path) {
    path_ = path;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width_, &height_, &channels_, 0);

    if (!data) {
        SE_LOG_ERROR("Texture: Failed to load image '{}'", path);
        return false;
    }

    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat = GL_RGB;

    if (channels_ == 4) {
        internalFormat = GL_RGBA8;
        dataFormat = GL_RGBA;
    } else if (channels_ == 3) {
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    } else if (channels_ == 1) {
        internalFormat = GL_R8;
        dataFormat = GL_RED;
    } else {
        SE_LOG_WARN("Texture: Unsupported channel count {} for '{}', treating as RGB", channels_, path);
        internalFormat = GL_RGB8;
        dataFormat = GL_RGB;
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width_, height_, 0, dataFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    float maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    if (maxAniso > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(maxAniso, 8.0f));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    SE_LOG_INFO("Texture: Loaded '{}' ({}x{}, {} channels)", path, width_, height_, channels_);
    return true;
}

void Texture::Release() {
    if (id_ != 0) {
        glDeleteTextures(1, &id_);
        id_ = 0;
    }
}

}  // namespace se
