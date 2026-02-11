#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/renderer/MeshData.h>
#include <engine/renderer/MeshSystem.h>
#include <engine/renderer/MaterialSystem.h>
#include <engine/renderer/LightSystem.h>
#include <engine/renderer/FilamentRenderer.h>
#include <engine/core/ServiceLocator.h>

using namespace se;

/**
 * Minimal Filament sandbox:
 * - Creates a colored cube with PBR material
 * - Sets up a directional light (sun)
 * - Configures camera to look at the cube
 * - Runs the render loop
 */
int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Monster Engine - Filament Test";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;

    Application application(appSpec);

    // Access rendering subsystems via ServiceLocator
    auto& materials = ServiceLocator::Get().GetMaterialSystem();
    auto& meshes    = ServiceLocator::Get().GetMeshSystem();
    auto& lights    = ServiceLocator::Get().GetLightSystem();
    auto& renderer  = ServiceLocator::Get().GetFilamentRenderer();

    // --- Create a red metallic cube ---
    MaterialConfig cubeConfig;
    cubeConfig.baseColor[0] = 0.8f;  // R
    cubeConfig.baseColor[1] = 0.2f;  // G
    cubeConfig.baseColor[2] = 0.2f;  // B
    cubeConfig.baseColor[3] = 1.0f;  // A
    cubeConfig.metallic     = 0.7f;
    cubeConfig.roughness    = 0.3f;
    cubeConfig.reflectance  = 0.5f;

    auto cubeMaterial = materials.CreateMaterial(cubeConfig);
    auto cubeMesh     = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
    auto cubeHandle   = meshes.CreateRenderable(cubeMesh, cubeMaterial);

    // --- Create a blue sphere ---
    MaterialConfig sphereConfig;
    sphereConfig.baseColor[0] = 0.2f;
    sphereConfig.baseColor[1] = 0.4f;
    sphereConfig.baseColor[2] = 0.9f;
    sphereConfig.metallic     = 0.0f;
    sphereConfig.roughness    = 0.6f;

    auto sphereMaterial = materials.CreateMaterial(sphereConfig);
    auto sphereMesh     = MeshPrimitives::CreateSphere(0.5f, 32, 16);
    auto sphereHandle   = meshes.CreateRenderable(sphereMesh, sphereMaterial);

    // Position the sphere to the right of the cube
    float sphereTransform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        2.0f, 0, 0, 1  // translate X+2
    };
    meshes.SetTransform(sphereHandle, sphereTransform);

    // --- Create a ground plane ---
    MaterialConfig groundConfig;
    groundConfig.baseColor[0] = 0.4f;
    groundConfig.baseColor[1] = 0.4f;
    groundConfig.baseColor[2] = 0.4f;
    groundConfig.metallic     = 0.0f;
    groundConfig.roughness    = 0.8f;

    auto groundMaterial = materials.CreateMaterial(groundConfig);
    auto groundMesh     = MeshPrimitives::CreatePlane(10.0f, 10.0f, 1);
    auto groundHandle   = meshes.CreateRenderable(groundMesh, groundMaterial);

    // Position the ground below the objects
    float groundTransform[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, -0.5f, 0, 1  // translate Y-0.5
    };
    meshes.SetTransform(groundHandle, groundTransform);

    // --- Set up directional light (sun) ---
    lights.SetDirectionalLight(
        -0.5f, -1.0f, -0.5f,  // direction (diagonal from top-left)
        1.0f, 0.98f, 0.95f,   // warm white color
        110000.0f,              // intensity (outdoor sun)
        true                    // cast shadows
    );

    // --- Set camera to look at the scene ---
    renderer.SetCameraProjection(60.0, 1280.0 / 720.0, 0.1, 100.0);
    renderer.SetCameraLookAt(
        {3.0f, 2.5f, 4.0f},   // eye position
        {0.5f, 0.0f, 0.0f},   // look-at target (between cube and sphere)
        {0.0f, 1.0f, 0.0f}    // up vector
    );

    SE_LOG_INFO("=== Filament Scene Ready ===");
    SE_LOG_INFO("  Red metallic cube at origin");
    SE_LOG_INFO("  Blue sphere at X=2");
    SE_LOG_INFO("  Gray ground plane");
    SE_LOG_INFO("  Directional sunlight");
    SE_LOG_INFO("Close the window to exit.");

    return application.Run();
}
