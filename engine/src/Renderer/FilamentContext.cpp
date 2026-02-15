#include "engine/renderer/FilamentContext.h"

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/Skybox.h>
#include <filament/IndirectLight.h>
#include <filament/Texture.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>
#include <filament-iblprefilter/IBLPrefilterContext.h>
#include <image/Ktx1Bundle.h>
#include <ktxreader/Ktx1Reader.h>

#include <math/vec3.h>

#include <utils/EntityManager.h>
#include <utils/Panic.h>

#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

// Platform-specific native window extraction
#if defined(__APPLE__)
extern "C" void* GetCocoaNativeWindow(void* glfwWindow);
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif

namespace se {

namespace {

/**
 * Convert our Backend enum to Filament's backend enum.
 */
filament::backend::Backend ToFilamentBackend(FilamentContext::Backend backend) {
    switch (backend) {
        case FilamentContext::Backend::OpenGL:
            return filament::backend::Backend::OPENGL;
        case FilamentContext::Backend::Vulkan:
            return filament::backend::Backend::VULKAN;
        case FilamentContext::Backend::Metal:
            return filament::backend::Backend::METAL;
        case FilamentContext::Backend::Default:
        default:
            return filament::backend::Backend::DEFAULT;
    }
}

struct EnvironmentPaths {
    std::filesystem::path skyboxKtx;
    std::filesystem::path iblKtx;
    std::filesystem::path hdrEquirect;

    bool HasPrecomputedKtx() const {
        return !skyboxKtx.empty() && !iblKtx.empty();
    }
};

template <size_t N>
std::filesystem::path ResolveAssetPath(const std::array<const char*, N>& relativeCandidates) {
    static constexpr std::array<const char*, 4> kSearchRoots = {
        "",
        "..",
        "../..",
        "../../..",
    };

    for (const auto* root : kSearchRoots) {
        for (const auto* relative : relativeCandidates) {
            std::filesystem::path path = std::filesystem::path(root) / relative;
            if (std::filesystem::exists(path)) {
                return path.lexically_normal();
            }
        }
    }

    return {};
}

EnvironmentPaths ResolveAltankaEnvironmentPaths() {
    static constexpr std::array<const char*, 2> kSkyboxKtxCandidates = {
        "assets/textures/ibl/altanka_4k_skybox.ktx",
        "assets/textures/ibl/altanka_4k/skybox.ktx",
    };
    static constexpr std::array<const char*, 2> kIblKtxCandidates = {
        "assets/textures/ibl/altanka_4k_ibl.ktx",
        "assets/textures/ibl/altanka_4k/ibl.ktx",
    };
    static constexpr std::array<const char*, 1> kHdrCandidates = {
        "assets/textures/ibl/altanka_4k.hdr",
    };

    EnvironmentPaths paths;
    paths.skyboxKtx  = ResolveAssetPath(kSkyboxKtxCandidates);
    paths.iblKtx     = ResolveAssetPath(kIblKtxCandidates);
    paths.hdrEquirect = ResolveAssetPath(kHdrCandidates);
    return paths;
}

filament::Texture* LoadKtxTexture(filament::Engine* engine, const std::filesystem::path& path, bool srgb) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        spdlog::warn("Failed to open KTX texture '{}'", path.string());
        return nullptr;
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        spdlog::warn("Invalid KTX texture size for '{}'", path.string());
        return nullptr;
    }
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
    if (!file.read(reinterpret_cast<char*>(bytes.data()), fileSize)) {
        spdlog::warn("Failed to read KTX texture '{}'", path.string());
        return nullptr;
    }

    auto* bundle = new image::Ktx1Bundle(bytes.data(), static_cast<uint32_t>(bytes.size()));
    auto* texture = ktxreader::Ktx1Reader::createTexture(engine, bundle, srgb);
    if (!texture) {
        delete bundle;
        spdlog::warn("Failed to decode KTX texture '{}'", path.string());
        return nullptr;
    }
    return texture;
}

filament::Texture* CreateHdrEquirectTexture(filament::Engine* engine,
                                            const std::filesystem::path& hdrPath) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(false);
    float* hdrPixels = stbi_loadf(hdrPath.string().c_str(), &width, &height, &channels, 3);
    if (!hdrPixels) {
        spdlog::warn("Failed to load HDR skybox '{}': {}", hdrPath.string(), stbi_failure_reason());
        return nullptr;
    }

    if (width <= 0 || height <= 0 || width != height * 2) {
        spdlog::warn("Invalid equirect HDR dimensions for '{}': {}x{} (expected 2:1)",
                     hdrPath.string(), width, height);
        stbi_image_free(hdrPixels);
        return nullptr;
    }

    const uint8_t mipLevels = static_cast<uint8_t>(
        std::floor(std::log2(std::max(width, height))) + 1.0
    );
    const auto usage = static_cast<filament::Texture::Usage>(
        static_cast<uint16_t>(filament::Texture::Usage::DEFAULT) |
        static_cast<uint16_t>(filament::Texture::Usage::GEN_MIPMAPPABLE)
    );

    auto* texture = filament::Texture::Builder()
        .width(static_cast<uint32_t>(width))
        .height(static_cast<uint32_t>(height))
        .levels(mipLevels)
        .sampler(filament::Texture::Sampler::SAMPLER_2D)
        .format(filament::Texture::InternalFormat::RGB16F)
        .usage(usage)
        .build(*engine);

    if (!texture) {
        spdlog::warn("Failed to create HDR equirect texture for '{}'", hdrPath.string());
        stbi_image_free(hdrPixels);
        return nullptr;
    }

    const size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 3u
        * sizeof(float);

    filament::Texture::PixelBufferDescriptor buffer(
        hdrPixels,
        dataSize,
        filament::Texture::Format::RGB,
        filament::Texture::Type::FLOAT,
        [](void* data, size_t, void*) {
            stbi_image_free(data);
        }
    );
    texture->setImage(*engine, 0, std::move(buffer));
    if (mipLevels > 1) {
        texture->generateMipmaps(*engine);
    }

    return texture;
}

} // anonymous namespace

FilamentContext::~FilamentContext() {
    Shutdown();
}

void FilamentContext::Init(GLFWwindow* window, uint32_t width, uint32_t height,
                           Backend backend) {
    if (engine_) {
        spdlog::warn("FilamentContext::Init called but engine already exists. Ignoring.");
        return;
    }

    active_backend_ = backend;

    // Create the Filament engine with the specified backend
    engine_ = filament::Engine::create(ToFilamentBackend(backend));
    if (!engine_) {
        spdlog::critical("Failed to create Filament Engine!");
        return;
    }

    spdlog::info("Filament Engine created successfully.");

    // Create SwapChain from the native window
    void* nativeWindow = GetNativeWindowHandle(window);
    if (!nativeWindow) {
        spdlog::critical("Failed to get native window handle!");
        Shutdown();
        return;
    }

    swap_chain_ = engine_->createSwapChain(nativeWindow);
    if (!swap_chain_) {
        spdlog::critical("Failed to create Filament SwapChain!");
        Shutdown();
        return;
    }

    // Create the renderer
    renderer_ = engine_->createRenderer();

    // Create the scene
    scene_ = engine_->createScene();

    // Create environment lighting from precomputed KTX first, then HDR fallback.
    auto cleanupEnvironmentResources = [this]() {
        if (indirect_light_) {
            engine_->destroy(indirect_light_);
            indirect_light_ = nullptr;
        }
        if (skybox_) {
            engine_->destroy(skybox_);
            skybox_ = nullptr;
        }
        if (ibl_texture_ == skybox_texture_ && ibl_texture_) {
            engine_->destroy(ibl_texture_);
            ibl_texture_ = nullptr;
            skybox_texture_ = nullptr;
        } else {
            if (ibl_texture_) {
                engine_->destroy(ibl_texture_);
                ibl_texture_ = nullptr;
            }
            if (skybox_texture_) {
                engine_->destroy(skybox_texture_);
                skybox_texture_ = nullptr;
            }
        }
    };

    bool environmentReady = false;
    const EnvironmentPaths environmentPaths = ResolveAltankaEnvironmentPaths();

    if (environmentPaths.HasPrecomputedKtx()) {
        skybox_texture_ = LoadKtxTexture(engine_, environmentPaths.skyboxKtx, false);
        ibl_texture_    = LoadKtxTexture(engine_, environmentPaths.iblKtx, false);

        if (skybox_texture_ && ibl_texture_) {
            skybox_ = filament::Skybox::Builder()
                .environment(skybox_texture_)
                .build(*engine_);
            indirect_light_ = filament::IndirectLight::Builder()
                .reflections(ibl_texture_)
                .intensity(30000.0f)
                .build(*engine_);

            if (skybox_ && indirect_light_) {
                scene_->setSkybox(skybox_);
                scene_->setIndirectLight(indirect_light_);
                environmentReady = true;
                spdlog::info("Using precomputed IBL assets: skybox='{}', ibl='{}'",
                             environmentPaths.skyboxKtx.string(),
                             environmentPaths.iblKtx.string());
            } else {
                cleanupEnvironmentResources();
            }
        } else {
            cleanupEnvironmentResources();
            spdlog::warn("Precomputed IBL assets were found but could not be loaded.");
        }
    } else {
        spdlog::warn("Precomputed IBL assets for altanka_4k not found. Falling back to runtime HDR prefilter.");
    }

    if (!environmentReady && !environmentPaths.hdrEquirect.empty()) {
        auto* equirectTexture = CreateHdrEquirectTexture(engine_, environmentPaths.hdrEquirect);
        if (equirectTexture) {
            try {
                IBLPrefilterContext prefilter(*engine_);
                IBLPrefilterContext::EquirectangularToCubemap equirectToCubemap(prefilter);
                IBLPrefilterContext::SpecularFilter specularFilter(prefilter);

                skybox_texture_ = equirectToCubemap(equirectTexture);
                if (skybox_texture_) {
                    ibl_texture_ = specularFilter(skybox_texture_);
                }

                if (skybox_texture_ && ibl_texture_) {
                    skybox_ = filament::Skybox::Builder()
                        .environment(skybox_texture_)
                        .build(*engine_);

                    indirect_light_ = filament::IndirectLight::Builder()
                        .reflections(ibl_texture_)
                        .intensity(30000.0f)
                        .build(*engine_);

                    if (skybox_ && indirect_light_) {
                        scene_->setSkybox(skybox_);
                        scene_->setIndirectLight(indirect_light_);
                        environmentReady = true;
                        spdlog::info("Using runtime HDR prefilter from '{}'",
                                     environmentPaths.hdrEquirect.string());
                    }
                }
            } catch (const utils::Panic& panic) {
                spdlog::warn("Failed to generate IBL from '{}': {}",
                             environmentPaths.hdrEquirect.string(), panic.what());
            } catch (const std::exception& e) {
                spdlog::warn("Failed to generate IBL from '{}': {}",
                             environmentPaths.hdrEquirect.string(), e.what());
            } catch (...) {
                spdlog::warn("Failed to generate IBL from '{}': unknown error",
                             environmentPaths.hdrEquirect.string());
            }

            engine_->destroy(equirectTexture);
        }
    }

    if (!environmentReady && environmentPaths.hdrEquirect.empty()) {
        spdlog::warn("HDR skybox not found: assets/textures/ibl/altanka_4k.hdr");
    }

    if (!environmentReady) {
        cleanupEnvironmentResources();

        skybox_ = filament::Skybox::Builder()
            .color({0.52f, 0.67f, 0.88f, 1.0f})
            .intensity(12000.0f)
            .build(*engine_);
        scene_->setSkybox(skybox_);

        filament::math::float3 sh[1] = {
            {0.35f, 0.40f, 0.48f}
        };
        indirect_light_ = filament::IndirectLight::Builder()
            .irradiance(1, sh)
            .intensity(20000.0f)
            .build(*engine_);
        scene_->setIndirectLight(indirect_light_);
    }

    // Create the view and associate it with the scene
    view_ = engine_->createView();
    view_->setScene(scene_);
    view_->setViewport({0, 0, width, height});

    // Enable post-processing features
    view_->setPostProcessingEnabled(true);
    filament::View::AmbientOcclusionOptions ao;
    ao.enabled    = true;
    ao.radius     = 0.75f;
    ao.intensity  = 1.2f;
    ao.resolution = 1.0f;
    ao.quality    = filament::QualityLevel::HIGH;
    view_->setAmbientOcclusionOptions(ao);

    // Create camera
    auto& entityManager = utils::EntityManager::get();
    auto cameraEntity = entityManager.create();
    camera_ = engine_->createCamera(cameraEntity);
    view_->setCamera(camera_);

    camera_entity_ = cameraEntity;
    has_camera_entity_ = true;

    spdlog::info("FilamentContext initialized: {}x{}, Backend={}",
                 width, height,
                 backend == Backend::Metal   ? "Metal"   :
                 backend == Backend::Vulkan  ? "Vulkan"  :
                 backend == Backend::OpenGL  ? "OpenGL"  : "Default");
}

void FilamentContext::Shutdown() {
    if (!engine_) return;

    if (scene_) {
        scene_->setIndirectLight(nullptr);
        scene_->setSkybox(nullptr);
    }

    if (indirect_light_) {
        engine_->destroy(indirect_light_);
        indirect_light_ = nullptr;
    }

    if (skybox_) {
        engine_->destroy(skybox_);
        skybox_ = nullptr;
    }

    if (ibl_texture_ == skybox_texture_ && ibl_texture_) {
        engine_->destroy(ibl_texture_);
        ibl_texture_ = nullptr;
        skybox_texture_ = nullptr;
    } else {
        if (ibl_texture_) {
            engine_->destroy(ibl_texture_);
            ibl_texture_ = nullptr;
        }

        if (skybox_texture_) {
            engine_->destroy(skybox_texture_);
            skybox_texture_ = nullptr;
        }
    }

    if (has_camera_entity_) {
        engine_->destroyCameraComponent(camera_entity_);
        utils::EntityManager::get().destroy(camera_entity_);
        has_camera_entity_ = false;
        camera_ = nullptr;
    }

    if (view_) {
        engine_->destroy(view_);
        view_ = nullptr;
    }

    if (scene_) {
        engine_->destroy(scene_);
        scene_ = nullptr;
    }

    if (renderer_) {
        engine_->destroy(renderer_);
        renderer_ = nullptr;
    }

    if (swap_chain_) {
        engine_->destroy(swap_chain_);
        swap_chain_ = nullptr;
    }

    filament::Engine::destroy(&engine_);
    engine_ = nullptr;

    spdlog::info("FilamentContext shut down.");
}

void FilamentContext::OnResize(uint32_t width, uint32_t height) {
    if (!view_) return;
    view_->setViewport({0, 0, width, height});
}

void* FilamentContext::GetNativeWindowHandle(GLFWwindow* window) const {
#if defined(__APPLE__)
    // Implemented in FilamentNativeWindow.mm (Objective-C++)
    return GetCocoaNativeWindow(static_cast<void*>(window));
#elif defined(_WIN32)
    return static_cast<void*>(glfwGetWin32Window(window));
#elif defined(__linux__)
    return reinterpret_cast<void*>(glfwGetX11Window(window));
#else
    spdlog::error("Unsupported platform for Filament native window!");
    return nullptr;
#endif
}

} // namespace se
