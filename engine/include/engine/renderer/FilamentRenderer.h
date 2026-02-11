#pragma once

#include <cstdint>
#include <mmath/Luma.h>

namespace filament {
class Engine;
class Renderer;
class Scene;
class View;
class Camera;
class SwapChain;
} // namespace filament

namespace se {

class FilamentContext;

/**
 * High-level rendering interface powered by Google Filament.
 *
 * Wraps Filament's Renderer/View/Scene to provide a clean API
 * for the engine's Layer system and ECS RenderSystem.
 *
 * Replaces the existing Renderer + SceneRenderer + RenderCommand classes.
 */
class FilamentRenderer {
public:
    explicit FilamentRenderer(FilamentContext& context);
    ~FilamentRenderer();

    // Non-copyable, non-movable
    FilamentRenderer(const FilamentRenderer&) = delete;
    FilamentRenderer& operator=(const FilamentRenderer&) = delete;

    /**
     * Begin a new frame. Must be called before any rendering.
     *
     * @return true if the frame can be rendered, false if skipped (e.g., minimized window).
     */
    bool BeginFrame();

    /**
     * End the current frame and present to the swap chain.
     */
    void EndFrame();

    /**
     * Set the clear color for the view.
     */
    void SetClearColor(float r, float g, float b, float a = 1.0f);

    /**
     * Set the camera projection matrix.
     *
     * @param fovDegrees Field of view in degrees.
     * @param aspect     Aspect ratio (width / height).
     * @param near       Near clipping plane.
     * @param far        Far clipping plane.
     */
    void SetCameraProjection(double fovDegrees, double aspect, double near, double far);

    /**
     * Set the camera view (look-at) matrix.
     *
     * @param eye    Camera position.
     * @param center Look-at target position.
     * @param up     Up vector.
     */
    void SetCameraLookAt(const luma::Vector3& eye,
                         const luma::Vector3& center,
                         const luma::Vector3& up);

    /**
     * Set the viewport dimensions.
     */
    void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    // --- Direct Filament access for advanced use ---

    filament::Engine*   GetEngine()   const;
    filament::Scene*    GetScene()    const;
    filament::View*     GetView()     const;
    filament::Camera*   GetCamera()   const;
    filament::Renderer* GetRenderer() const;

private:
    FilamentContext& context_;
};

} // namespace se
