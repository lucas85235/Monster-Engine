#pragma once
/**
 * EditorFramebuffer.h - Offscreen rendering target for the editor viewport.
 *
 * Creates a filament::Texture (SAMPLEABLE | COLOR_ATTACHMENT) and a
 * filament::RenderTarget so the scene can be rendered offscreen.
 * The color texture pointer is returned as ImTextureID for ImGui::Image.
 *
 * Also owns a filament::Camera to sync with the engine's Camera data.
 */

#include <cstdint>

#include <utils/Entity.h>

namespace filament {
class Engine;
class Texture;
class RenderTarget;
class View;
class Scene;
class Camera;
}  // namespace filament

namespace mst {

class EditorFramebuffer {
   public:
    EditorFramebuffer(uint32_t width, uint32_t height);
    ~EditorFramebuffer();

    // Non-copyable, non-movable
    EditorFramebuffer(const EditorFramebuffer&) = delete;
    EditorFramebuffer& operator=(const EditorFramebuffer&) = delete;

    void Resize(uint32_t width, uint32_t height);

    /**
     * Returns the filament::Texture* pointer cast to uintptr_t,
     * suitable for use as ImTextureID in ImGui::Image.
     */
    uintptr_t GetColorAttachmentAsImTextureID() const;

    /**
     * Get the offscreen Filament View.
     */
    filament::View* GetView() const { return view_; }

    /**
     * Get the owned filament::Camera for setting view/projection data.
     */
    filament::Camera* GetCamera() const { return camera_; }

    /**
     * Set the Filament Scene to render on this offscreen view.
     */
    void SetScene(filament::Scene* scene);

    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

   private:
    void CreateResources();
    void DestroyResources();

    filament::Engine*       engine_ = nullptr;
    filament::Texture*      colorTexture_ = nullptr;
    filament::Texture*      depthTexture_ = nullptr;
    filament::RenderTarget* renderTarget_ = nullptr;
    filament::View*         view_ = nullptr;
    filament::Camera*       camera_ = nullptr;
    utils::Entity           cameraEntity_{};

    uint32_t width_;
    uint32_t height_;
};

}  // namespace mst
