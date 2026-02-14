#pragma once

#include <stdexcept>

namespace se {

class InputManager;
class EventBus;
class FilamentContext;
class FilamentRenderer;
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
class SceneRenderer;
#endif
class MaterialSystem;
class MeshSystem;
class LightSystem;
class TextureSystem;
class FilamentModelLoader;
class RenderSettingsSystem;

/**
 * ServiceLocator provides centralized access to engine services.
 * Uses Meyer's Singleton pattern for thread-safe lazy initialization.
 *
 * Services are non-owning pointers - the Application owns the actual instances.
 * Services must be registered before use via Provide* methods.
 */
class ServiceLocator {
   public:
    static ServiceLocator& Get() {
        static ServiceLocator instance;
        return instance;
    }

    // Service registration (non-owning pointers)
    void ProvideInputManager(InputManager* input) {
        input_manager_ = input;
    }
    void ProvideEventBus(EventBus* eventBus) {
        event_bus_ = eventBus;
    }
    void ProvideFilamentContext(FilamentContext* context) {
        filament_context_ = context;
    }
    void ProvideFilamentRenderer(FilamentRenderer* renderer) {
        filament_renderer_ = renderer;
    }
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
    void ProvideSceneRenderer(SceneRenderer* renderer) {
        scene_renderer_ = renderer;
    }
#endif
    void ProvideMaterialSystem(MaterialSystem* materials) {
        material_system_ = materials;
    }
    void ProvideMeshSystem(MeshSystem* meshes) {
        mesh_system_ = meshes;
    }
    void ProvideLightSystem(LightSystem* lights) {
        light_system_ = lights;
    }
    void ProvideTextureSystem(TextureSystem* textures) {
        texture_system_ = textures;
    }
    void ProvideModelLoader(FilamentModelLoader* loader) {
        model_loader_ = loader;
    }
    void ProvideRenderSettingsSystem(RenderSettingsSystem* settings) {
        render_settings_system_ = settings;
    }

    // Service access with validation
    InputManager& GetInputManager() const {
        if (!input_manager_)
            throw std::runtime_error("InputManager not registered with ServiceLocator");
        return *input_manager_;
    }

    EventBus& GetEventBus() const {
        if (!event_bus_) throw std::runtime_error("EventBus not registered with ServiceLocator");
        return *event_bus_;
    }

    FilamentContext& GetFilamentContext() const {
        if (!filament_context_)
            throw std::runtime_error("FilamentContext not registered with ServiceLocator");
        return *filament_context_;
    }

    FilamentRenderer& GetFilamentRenderer() const {
        if (!filament_renderer_)
            throw std::runtime_error("FilamentRenderer not registered with ServiceLocator");
        return *filament_renderer_;
    }
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
    SceneRenderer& GetSceneRenderer() const {
        if (!scene_renderer_)
            throw std::runtime_error("SceneRenderer not registered with ServiceLocator");
        return *scene_renderer_;
    }
#endif

    MaterialSystem& GetMaterialSystem() const {
        if (!material_system_)
            throw std::runtime_error("MaterialSystem not registered with ServiceLocator");
        return *material_system_;
    }

    MeshSystem& GetMeshSystem() const {
        if (!mesh_system_)
            throw std::runtime_error("MeshSystem not registered with ServiceLocator");
        return *mesh_system_;
    }

    LightSystem& GetLightSystem() const {
        if (!light_system_)
            throw std::runtime_error("LightSystem not registered with ServiceLocator");
        return *light_system_;
    }

    TextureSystem& GetTextureSystem() const {
        if (!texture_system_)
            throw std::runtime_error("TextureSystem not registered with ServiceLocator");
        return *texture_system_;
    }

    FilamentModelLoader& GetModelLoader() const {
        if (!model_loader_)
            throw std::runtime_error("FilamentModelLoader not registered with ServiceLocator");
        return *model_loader_;
    }
    RenderSettingsSystem& GetRenderSettingsSystem() const {
        if (!render_settings_system_) {
            throw std::runtime_error("RenderSettingsSystem not registered with ServiceLocator");
        }
        return *render_settings_system_;
    }

    // Raw pointer access for optional checks
    InputManager* GetInputManagerPtr() const {
        return input_manager_;
    }
    EventBus* GetEventBusPtr() const {
        return event_bus_;
    }
    FilamentContext* GetFilamentContextPtr() const {
        return filament_context_;
    }
    FilamentRenderer* GetFilamentRendererPtr() const {
        return filament_renderer_;
    }
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
    SceneRenderer* GetSceneRendererPtr() const {
        return scene_renderer_;
    }
#endif
    MaterialSystem* GetMaterialSystemPtr() const {
        return material_system_;
    }
    MeshSystem* GetMeshSystemPtr() const {
        return mesh_system_;
    }
    LightSystem* GetLightSystemPtr() const {
        return light_system_;
    }
    TextureSystem* GetTextureSystemPtr() const {
        return texture_system_;
    }
    FilamentModelLoader* GetModelLoaderPtr() const {
        return model_loader_;
    }
    RenderSettingsSystem* GetRenderSettingsSystemPtr() const {
        return render_settings_system_;
    }

    // Availability checks
    bool HasInputManager() const {
        return input_manager_ != nullptr;
    }
    bool HasEventBus() const {
        return event_bus_ != nullptr;
    }
    bool HasFilamentContext() const {
        return filament_context_ != nullptr;
    }
    bool HasFilamentRenderer() const {
        return filament_renderer_ != nullptr;
    }
    bool HasRenderSettingsSystem() const {
        return render_settings_system_ != nullptr;
    }
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
    bool HasSceneRenderer() const {
        return scene_renderer_ != nullptr;
    }
#endif

    // Reset all services (for shutdown/testing)
    void Reset() {
        input_manager_     = nullptr;
        event_bus_         = nullptr;
        filament_context_  = nullptr;
        filament_renderer_ = nullptr;
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
        scene_renderer_    = nullptr;
#endif
        material_system_   = nullptr;
        mesh_system_       = nullptr;
        light_system_      = nullptr;
        texture_system_    = nullptr;
        model_loader_      = nullptr;
        render_settings_system_ = nullptr;
    }

   private:
    ServiceLocator()  = default;
    ~ServiceLocator() = default;

    ServiceLocator(const ServiceLocator&)            = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;
    ServiceLocator(ServiceLocator&&)                 = delete;
    ServiceLocator& operator=(ServiceLocator&&)      = delete;

    InputManager*         input_manager_     = nullptr;
    EventBus*             event_bus_         = nullptr;
    FilamentContext*      filament_context_  = nullptr;
    FilamentRenderer*     filament_renderer_ = nullptr;
#if defined(SE_ENABLE_LEGACY_OPENGL_RENDERER) && SE_ENABLE_LEGACY_OPENGL_RENDERER
    SceneRenderer*        scene_renderer_    = nullptr;
#endif
    MaterialSystem*       material_system_   = nullptr;
    MeshSystem*           mesh_system_       = nullptr;
    LightSystem*          light_system_      = nullptr;
    TextureSystem*        texture_system_    = nullptr;
    FilamentModelLoader*  model_loader_      = nullptr;
    RenderSettingsSystem* render_settings_system_ = nullptr;
};

}  // namespace se
