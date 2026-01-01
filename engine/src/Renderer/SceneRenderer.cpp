#include "engine/renderer/SceneRenderer.h"

#include <cstdio>
#include <glad/glad.h>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vector>

#include "engine/Log.h"
#include "engine/debug/FrameProfiler.h"
#include "engine/renderer/GBufferPass.h"
#include "engine/renderer/RadianceCascadesPass.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/renderer/Texture.h"
#include "engine/renderer/SSGIPass.h"
#include "engine/renderer/SSAOPass.h"
#include "engine/renderer/BloomPass.h"
#include "engine/renderer/TonemappingPass.h"
#include "engine/renderer/FXAAPass.h"
#include "engine/renderer/ColorGradingPass.h"

namespace {
constexpr const char* kShadowVertexSource = R"(#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main() {
    vec4 new_pos = uLightSpaceMatrix * uModel * vec4(a_Position, 1.0);
    gl_Position = new_pos;
}
)";

constexpr const char* kShadowFragmentSource = R"(#version 330 core
void main() {
    // depth only
}
)";

// Instanced shadow shader - reads transform from instance buffer
constexpr const char* kInstancedShadowVertexSource = R"(#version 330 core
layout(location = 0) in vec3 a_Position;

// Per-instance data (Mat4 uses locations 3-6)
layout(location = 3) in mat4 a_InstanceTransform;

uniform mat4 uLightSpaceMatrix;

void main() {
    vec4 world_pos = a_InstanceTransform * vec4(a_Position, 1.0);
    gl_Position = uLightSpaceMatrix * world_pos;
}
)";

// Skinned shadow shader - includes bone matrix transforms
// MUST match vertex attribute layout from skinned_model.vert:
// location 5 = a_BoneIds, location 6 = a_BoneWeights
constexpr int MAX_SHADOW_BONES = 128;
constexpr const char* kSkinnedShadowVertexSource = R"(#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 5) in ivec4 a_BoneIds;
layout(location = 6) in vec4 a_BoneWeights;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;
uniform mat4 uBoneMatrices[128];
uniform int uHasBones;

void main() {
    vec4 localPos = vec4(a_Position, 1.0);
    
    if (uHasBones == 1) {
        mat4 boneTransform = mat4(0.0);
        for (int i = 0; i < 4; i++) {
            int boneId = a_BoneIds[i];
            float weight = a_BoneWeights[i];
            if (boneId >= 0 && boneId < 128 && weight > 0.0) {
                boneTransform += uBoneMatrices[boneId] * weight;
            }
        }
        // Handle case where no weights were applied
        if (boneTransform[0][0] == 0.0 && boneTransform[1][1] == 0.0 && 
            boneTransform[2][2] == 0.0 && boneTransform[3][3] == 0.0) {
            boneTransform = mat4(1.0);
        }
        localPos = boneTransform * localPos;
    }
    
    gl_Position = uLightSpaceMatrix * uModel * localPos;
}
)";
}  // namespace

namespace se {

SceneRenderer::SceneRenderer() {}

SceneRenderer::~SceneRenderer() {
    if (initialized_) { Shutdown(); }
}

void SceneRenderer::Init() {
    if (initialized_) {
        SE_LOG_WARN("SceneRenderer already initialized");
        return;
    }

    SE_LOG_INFO("Initializing SceneRenderer");
    InitializeShadowResources();
    occlusionCuller_.Init();
    
    // Initialize default IBL for outdoor lighting
    iblData_.SetDefaultOutdoor();
    
    // Initialize G-Buffer (will be resized on first frame)
    gbuffer_ = std::make_unique<GBufferPass>();
    
    // Load G-Buffer shaders (may fail if files don't exist, handle gracefully)
    try {
        gbufferShader_ = Shader::CreateFromFiles("assets/shaders/deferred/gbuffer.vert", "assets/shaders/deferred/gbuffer.frag");
        SE_LOG_INFO("G-Buffer shader loaded");
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Error loading gbuffer shader: {}", e.what());
        gbufferShader_.reset();
    }
    
    try {
        gbufferInstancedShader_ = Shader::CreateFromFiles("assets/shaders/deferred/gbuffer_instanced.vert", "assets/shaders/deferred/gbuffer_instanced.frag");
        SE_LOG_INFO("G-Buffer instanced shader loaded");
    } catch (const std::exception& e) {
        SE_LOG_ERROR("Error loading gbuffer instanced shader: {}", e.what());
        gbufferInstancedShader_.reset();
    }
    
    // TODO(GI): Re-enable Radiance Cascades initialization when feature is ready
    // radianceCascades_ = std::make_unique<RadianceCascadesPass>();
    
    // Initialize SSGI pass (will be initialized on first frame with dimensions)
    ssgiPass_ = std::make_unique<SSGIPass>();
    SE_LOG_INFO("SSGI pass created (lazy init)");
    
    // Initialize Cascaded Shadow Maps
    csm_ = std::make_unique<CascadedShadowMap>();
    csm_->Init(2048);  // 2048x2048 per cascade
    
    // Initialize Post-Processing Pipeline (will add passes on first frame)
    postProcessPipeline_ = std::make_unique<PostProcessPipeline>();
    SE_LOG_INFO("Post-process pipeline created (lazy init)");
    
    initialized_ = true;
}

void SceneRenderer::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down SceneRenderer");
    
    if (gbuffer_) {
        gbuffer_->Shutdown();
        gbuffer_.reset();
    }
    
    if (radianceCascades_) {
        radianceCascades_->Shutdown();
        radianceCascades_.reset();
    }
    
    if (sparseRC_) {
        sparseRC_->Shutdown();
        sparseRC_.reset();
    }
    
    if (voxelizer_) {
        voxelizer_->Shutdown();
        voxelizer_.reset();
    }
    
    if (ssgiPass_) {
        ssgiPass_->Shutdown();
        ssgiPass_.reset();
    }
    
    if (postProcessPipeline_) {
        postProcessPipeline_->Shutdown();
        postProcessPipeline_.reset();
    }
    
    DestroyHDRFramebuffer();
    
    gbufferShader_.reset();
    occlusionCuller_.Shutdown();
    DestroyShadowResources();
    initialized_ = false;
}

void SceneRenderer::BeginFrame() {
    SE_PROFILE_SCOPE("SceneRenderer::BeginFrame");
    
    // Get viewport dimensions (needed by other code)
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int width = viewport[2];
    int height = viewport[3];
    
    // Store original FBO to restore later (used by FinishFrame)
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFBO_);
    storedViewport_[0] = viewport[0];
    storedViewport_[1] = viewport[1];
    storedViewport_[2] = viewport[2];
    storedViewport_[3] = viewport[3];
    
    // Lazy initialization of resources when viewport dimensions change
    if (width > 0 && height > 0 && (screenWidth_ != width || screenHeight_ != height)) {
        screenWidth_ = width;
        screenHeight_ = height;
        
        // Resize existing resources
        if (gbuffer_ && gbuffer_->IsInitialized()) {
            gbuffer_->Resize(width, height);
        }
        
        // Resize SSGI if already initialized
        if (ssgiPass_ && ssgiPass_->IsInitialized()) {
            ssgiPass_->Resize(width, height);
        }
        
        // Resize HDR framebuffer if it exists
        if (hdrFBO_ != 0) {
            DestroyHDRFramebuffer();
        }
        
        // Resize post-process pipeline
        if (postProcessPipeline_ && postProcessPipeline_->IsInitialized()) {
            postProcessPipeline_->Resize(width, height);
        }
        
        // Resize SSAO pass if already initialized
        if (ssaoPass_) {
            ssaoPass_->Resize(width, height);
        }
    } else if (width > 0 && height > 0) {
        // Just update screen size if no resize needed
        screenWidth_ = width;
        screenHeight_ = height;
    }
    
    // Initialize GBuffer if needed but not yet initialized
    if (width > 0 && height > 0 && gbuffer_ && !gbuffer_->IsInitialized()) {
        gbuffer_->Init(width, height);
        SE_LOG_INFO("[GBuffer] Lazy init: {}x{}", width, height);
    }
    
    // Initialize SSGI if enabled but not yet initialized
    if (width > 0 && height > 0 && ssgiPass_ && ssgiPass_->IsEnabled() && !ssgiPass_->IsInitialized()) {
        ssgiPass_->Init(width, height);
        SE_LOG_INFO("[SSGI] Lazy init: {}x{}", width, height);
    }
    
    // Initialize HDR framebuffer and post-process pipeline if enabled
    if (postProcessEnabled_ && width > 0 && height > 0) {
        // Create HDR FBO if not yet initialized
        if (hdrFBO_ == 0) {
            InitHDRFramebuffer();
            SE_LOG_INFO("[PostProcess] HDR FBO initialized: {}x{}", width, height);
        }
        
        // Initialize post-process pipeline with passes
        if (postProcessPipeline_ && !postProcessPipeline_->IsInitialized()) {
            postProcessPipeline_->AddPass<BloomPass>();  // HDR bloom effect
            postProcessPipeline_->AddPass<FXAAPass>();   // Anti-aliasing
            postProcessPipeline_->AddPass<ColorGradingPass>();  // Color adjustments
            postProcessPipeline_->AddPass<TonemappingPass>();  // Final HDR->LDR conversion
            postProcessPipeline_->Init(screenWidth_, screenHeight_);
            SE_LOG_INFO("[PostProcess] Pipeline initialized with Bloom, FXAA, ColorGrading, Tonemapping");
        }
        
        // Initialize standalone SSAO pass (generates AO texture for scene shaders)
        if (ssaoEnabled_ && !ssaoPass_) {
            ssaoPass_ = std::make_unique<SSAOPass>();
            ssaoPass_->Init(screenWidth_, screenHeight_);
            SE_LOG_INFO("[SSAO] Standalone pass initialized: {}x{}", screenWidth_, screenHeight_);
        }
        
        // Bind HDR FBO for scene rendering - all rendering until FinishFrame goes here
        if (hdrFBO_ != 0) {
            // Save the current (external) FBO and viewport before binding HDR FBO
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &originalFBO_);
            glGetIntegerv(GL_VIEWPORT, storedViewport_);
            
            glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO_);
            glViewport(0, 0, screenWidth_, screenHeight_);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
    }
}

void SceneRenderer::BeginScene(const Camera& camera, const Matrix4& projection) {
    sceneData_.ViewMatrix             = camera.getViewMatrix();
    sceneData_.ProjectionMatrix       = projection;
    sceneData_.view_projection_matrix = projection * sceneData_.ViewMatrix;
    sceneData_.CameraPosition         = glm::inverse(sceneData_.ViewMatrix)[3];  // Extract camera world position
    sceneData_.CurrentCamera          = &camera;  // Store for CSM
    sceneData_.Submissions.clear();
    instancedSubmissions_.clear();
    skinnedSubmissions_.clear();

    // Prepare directional light data and shadow matrix
    if (!sceneData_.directional_light.Active) {
        sceneData_.directional_light.Direction = Vector3(0.0f, -1.0f, 0.0f);
        sceneData_.directional_light.Intensity = 0.0f;
        sceneData_.directional_light.Color     = Vector3(1.0f);
        sceneData_.ShadowsEnabled              = false;
        sceneData_.LightSpaceMatrix            = Matrix4(1.0f);
    } else {
        Vector3 lightDir = sceneData_.directional_light.Direction;
        if (glm::length(lightDir) <= 0.0f) { lightDir = Vector3(0.0f, -1.0f, 0.0f); }
        lightDir                               = glm::normalize(lightDir);
        sceneData_.directional_light.Direction = lightDir;

        sceneData_.ShadowsEnabled = sceneData_.directional_light.CastShadows &&
                                    sceneData_.directional_light.Intensity > 0.0f;

        if (sceneData_.ShadowsEnabled) {
            // Shadow frustum follows camera position for consistent shadow coverage
            Vector3       cameraPos  = glm::inverse(sceneData_.ViewMatrix)[3];
            const Vector3 focusPoint = Vector3(cameraPos.x, 0.0f, cameraPos.z);
            const Vector3 lightPos   = focusPoint - lightDir * sceneData_.ShadowDistance;
            Vector3       up         = Vector3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(up, lightDir)) > 0.95f) { up = Vector3(0.0f, 0.0f, 1.0f); }
            Matrix4 lightView = glm::lookAt(lightPos, focusPoint, up);
            Matrix4 lightProj = glm::ortho(-sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize,
                                           -sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize,
                                           0.1f, sceneData_.ShadowDistance * 2.0f);
            sceneData_.LightSpaceMatrix = lightProj * lightView;
        } else {
            sceneData_.LightSpaceMatrix = Matrix4(1.0f);
        }
    }

    ResetStats();
}

void SceneRenderer::EndScene() {
    SE_PROFILE_SCOPE("SceneRenderer::EndScene");
    
    if (sceneData_.ShadowsEnabled) { 
        SE_PROFILE_SCOPE("ShadowPass");
        RenderShadowPass();
        RenderCSMPass();  // Render cascaded shadow maps
    }
    
    // Step 1: Render scene to G-Buffer (for GI and SSAO)
    bool needGBuffer = (radianceCascades_ && radianceCascades_->IsEnabled()) ||
                       (sparseRC_ && sparseRC_->IsEnabled()) ||
                       (ssgiPass_ && ssgiPass_->IsReady()) ||
                       (ssaoEnabled_ && ssaoPass_);
    
    if (gbuffer_ && gbuffer_->IsInitialized() && needGBuffer) {
        SE_PROFILE_SCOPE("GBufferPass");
        RenderGBufferPass();
        
        // Voxelize from GBuffer for world-space GI
        if (sparseRC_ && sparseRC_->IsEnabled() && voxelizer_ && voxelizer_->IsInitialized()) {
            SE_PROFILE_SCOPE("Voxelize");
            voxelizer_->VoxelizeFromGBuffer(
                gbuffer_->GetPositionTexture(),
                gbuffer_->GetAlbedoTexture(),
                gbuffer_->GetEmissiveTexture(),
                screenWidth_, screenHeight_);
        }
        
        // Execute SSGI if enabled
        if (ssgiPass_ && ssgiPass_->IsReady()) {
            SE_PROFILE_SCOPE("SSGI");
            
            // Ensure G-Buffer textures are fully written before compute shader reads them
            glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
            
            // Ensure proper resolution for SSGI (in case resolution scale changed)
            ssgiPass_->Resize(screenWidth_, screenHeight_);
            
            // Update light data for sun contribution
            if (sceneData_.directional_light.Active) {
                ssgiPass_->SetLightData(
                    -sceneData_.directional_light.Direction,
                    sceneData_.directional_light.Color,
                    sceneData_.directional_light.Intensity
                );
            }
            
            ssgiPass_->Execute(
                gbuffer_->GetPositionTexture(),
                gbuffer_->GetNormalTexture(),
                gbuffer_->GetAlbedoTexture(),
                gbuffer_->GetEmissiveTexture(),
                gbuffer_->GetDepthTexture(),
                sceneData_.ProjectionMatrix,
                sceneData_.ViewMatrix,
                glm::inverse(sceneData_.ProjectionMatrix),
                glm::inverse(sceneData_.ViewMatrix),
                sceneData_.CameraPosition
            );
        }
        
        // Execute standalone SSAO pass (generates AO texture for scene shaders)
        if (ssaoEnabled_ && ssaoPass_ && ssaoPass_->IsEnabled()) {
            SE_PROFILE_SCOPE("SSAO");
            
            ssaoPass_->SetDepthTexture(gbuffer_->GetDepthTexture());
            ssaoPass_->SetNormalTexture(gbuffer_->GetNormalTexture());
            ssaoPass_->SetProjectionMatrix(sceneData_.ProjectionMatrix);
            ssaoPass_->SetViewMatrix(sceneData_.ViewMatrix);
            
            // Execute SSAO (renders to internal FBO)
            ssaoPass_->Execute(0, 0);  // Input not used, output not used (uses internal FBOs)
            
            // Cache the blurred AO texture for scene shaders
            ssaoTexture_ = ssaoPass_->GetAOTexture();
        } else {
            ssaoTexture_ = 0;  // No SSAO available
        }
    }

    // Step 2: Execute Radiance Cascades with G-Buffer data
    if (radianceCascades_ && radianceCascades_->IsEnabled()) {
        SE_PROFILE_SCOPE("RadianceCascades");
        uint32_t sceneColorTex = 0;
        uint32_t sceneDepthTex = sceneData_.ShadowDepthTexture;
        uint32_t scenePositionTex = 0;
        
        if (gbuffer_ && gbuffer_->IsInitialized()) {
            sceneColorTex = gbuffer_->GetEmissiveTexture();
            sceneDepthTex = gbuffer_->GetDepthTexture();
            scenePositionTex = gbuffer_->GetPositionTexture();
        }
        
        // Get voxel data if available
        uint32_t voxelAlbedoTex = 0;
        uint32_t voxelEmissiveTex = 0;
        glm::vec3 voxelGridCenter = glm::vec3(0.0f);
        float voxelGridSize = 50.0f;
        int voxelResolution = 128;
        
        if (voxelizer_ && voxelizer_->IsInitialized()) {
            voxelAlbedoTex = voxelizer_->GetVoxelTexture();
            voxelEmissiveTex = voxelizer_->GetVoxelEmissiveTexture();
            voxelGridCenter = voxelizer_->GetConfig().Center;
            voxelGridSize = voxelizer_->GetConfig().WorldSize;
            voxelResolution = voxelizer_->GetConfig().Resolution;
        }
        
        radianceCascades_->Execute(sceneColorTex, sceneDepthTex, scenePositionTex,
                                   sceneData_.ProjectionMatrix, sceneData_.ViewMatrix,
                                   sceneData_.CameraPosition,
                                   voxelAlbedoTex, voxelEmissiveTex,
                                   voxelGridCenter, voxelGridSize, voxelResolution);
    }
    
    // Step 2b: Execute Sparse Radiance Cascades (World-Space GI) if enabled
    static uint32_t frameNumber = 0;
    frameNumber++;
    
    if (sparseRC_ && sparseRC_->IsEnabled()) {
        SE_PROFILE_SCOPE("SparseRC");
        // Provide GBuffer access
        if (gbuffer_ && gbuffer_->IsInitialized()) {
            sparseRC_->SetGBufferPass(gbuffer_.get());
        }
        
        if (voxelizer_ && voxelizer_->IsInitialized()) {
            voxelizer_->SetCenter(glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        sparseRC_->Execute(sceneData_.ProjectionMatrix, sceneData_.ViewMatrix,
                           sceneData_.CameraPosition, frameNumber);
    }
    
    // Step 3: Render final scene with GI applied
    {
        SE_PROFILE_SCOPE("ScenePass");
        RenderScenePass();
    }
    
    // Step 3.5: Render skybox (after scene, uses depth test LEQUAL trick)
    {
        SE_PROFILE_SCOPE("Skybox");
        InitSkybox();
        RenderSkybox();
    }
    
    // Step 4: Render debug visualization for Sparse RC
    if (sparseRC_ && sparseRC_->IsEnabled()) {
        glm::mat4 viewProj = sceneData_.ProjectionMatrix * sceneData_.ViewMatrix;
        sparseRC_->RenderDebug(viewProj);
    }
    
    // Note: SkinnedModels are rendered by RenderSystem AFTER EndScene
    // Post-processing is executed in FinishFrame() after all rendering is complete
}

void SceneRenderer::FinishFrame() {
    SE_PROFILE_SCOPE("SceneRenderer::FinishFrame");
    
    bool usePostProcess = postProcessEnabled_ && postProcessPipeline_ && 
                          postProcessPipeline_->IsInitialized() && hdrFBO_ != 0;
    
    if (usePostProcess) {
        // Unbind HDR framebuffer (post-process reads from it)
        glBindFramebuffer(GL_FRAMEBUFFER, originalFBO_);
        glViewport(storedViewport_[0], storedViewport_[1], storedViewport_[2], storedViewport_[3]);
        
        // Set SSAO textures if GBuffer is available
        auto* ssaoPass = postProcessPipeline_->GetPass<SSAOPass>();
        if (ssaoPass && gbuffer_ && gbuffer_->IsInitialized()) {
            ssaoPass->SetDepthTexture(gbuffer_->GetDepthTexture());
            ssaoPass->SetNormalTexture(gbuffer_->GetNormalTexture());
            ssaoPass->SetProjectionMatrix(sceneData_.ProjectionMatrix);
            ssaoPass->SetViewMatrix(sceneData_.ViewMatrix);
        }
        
        // Temporarily disable Bloom when debug mode is active
        auto* bloomPass = postProcessPipeline_->GetPass<BloomPass>();
        bool bloomWasEnabled = bloomPass ? bloomPass->IsEnabled() : false;
        if (bloomPass && debugMode_ > 0) {
            bloomPass->SetEnabled(false);
        }
        
        // Execute all post-process passes - output to original (external) FBO
        // Pass the target viewport dimensions for correct final output size
        postProcessPipeline_->Execute(hdrColorTexture_, 0, originalFBO_, 
                                       storedViewport_[2], storedViewport_[3]);
        
        // Restore viewport for any subsequent rendering (grid, gizmos, etc.)
        
        // Restore Bloom state
        if (bloomPass) {
            bloomPass->SetEnabled(bloomWasEnabled);
        }
        
        // Clean up texture bindings to prevent interference with next frame
        for (int i = 0; i < 8; ++i) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        glActiveTexture(GL_TEXTURE0);
    }
}


void SceneRenderer::Submit(const std::shared_ptr<VertexArray>& vertexArray,
                           const std::shared_ptr<Material>& material, const Matrix4& transform,
                           bool castsShadows, bool receiveShadows, float boundingRadius,
                           const std::shared_ptr<TextureMaterial>& textureMaterial,
                           const Vector3& emissiveColor, float emissiveFactor) {
    Submission submission;
    submission.vertex_array    = vertexArray;
    submission.material        = material;
    submission.Transform       = transform;
    submission.CastsShadows    = castsShadows;
    submission.ReceiveShadows  = receiveShadows;
    submission.textureMaterial = textureMaterial;
    submission.EmissiveColor   = emissiveColor;
    submission.EmissiveFactor  = emissiveFactor;

    // Extract position from transform
    submission.Center = Vector3(transform[3]);

    // Create stable ObjectId based on position hash (for occlusion query tracking)
    // This ensures the same object gets the same ID across frames
    auto hashFloat      = [](float f) -> uint32_t { return *reinterpret_cast<uint32_t*>(&f); };
    submission.ObjectId = hashFloat(submission.Center.x) ^ (hashFloat(submission.Center.y) << 8) ^
                          (hashFloat(submission.Center.z) << 16);
    if (submission.ObjectId == 0) submission.ObjectId = 1;  // 0 is reserved

    // Calculate bounding radius considering scale
    float scaleX   = glm::length(Vector3(transform[0]));
    float scaleY   = glm::length(Vector3(transform[1]));
    float scaleZ   = glm::length(Vector3(transform[2]));
    float maxScale = glm::max(glm::max(scaleX, scaleY), scaleZ);

    // For a unit cube, bounding sphere radius is sqrt(3)/2 ≈ 0.866
    submission.BoundingRadius = boundingRadius * maxScale;

    sceneData_.Submissions.emplace_back(std::move(submission));
}

void SceneRenderer::SubmitInstanced(const std::shared_ptr<InstancedMesh>& instancedMesh,
                                    const std::shared_ptr<Material>& material, bool castsShadows,
                                    bool receiveShadows,
                                    const glm::vec3& emissiveColor, float emissiveFactor) {
    if (!instancedMesh || !material) {
        SE_LOG_WARN("SubmitInstanced called with null instancedMesh or material");
        return;
    }

    InstancedSubmission submission;
    submission.instancedMesh   = instancedMesh;
    submission.material        = material;
    submission.castsShadows    = castsShadows;
    submission.receiveShadows  = receiveShadows;
    submission.EmissiveColor   = emissiveColor;
    submission.EmissiveFactor  = emissiveFactor;
    instancedSubmissions_.emplace_back(std::move(submission));
}

void SceneRenderer::SubmitSkinnedForShadow(
    uint32_t vaoId,
    uint32_t indexCount,
    const Matrix4& transform,
    const std::vector<Matrix4>& boneMatrices,
    bool hasBones) {
    if (vaoId == 0 || indexCount == 0) return;
    
    SkinnedSubmission submission;
    submission.vaoId = vaoId;
    submission.indexCount = indexCount;
    submission.transform = transform;
    submission.boneMatrices = boneMatrices;
    submission.hasBones = hasBones;
    skinnedSubmissions_.emplace_back(std::move(submission));
}

void SceneRenderer::SetOcclusionCullingEnabled(bool enabled) {
    occlusionCullingEnabled_ = enabled;
    occlusionCuller_.SetEnabled(enabled);
}

void SceneRenderer::SubmitWithPBR(const std::shared_ptr<VertexArray>& vertexArray,
                                  const std::shared_ptr<Material>& material,
                                  const Matrix4& transform,
                                  const PBRMaterialParams& pbrParams,
                                  bool castsShadows, bool receiveShadows,
                                  const std::shared_ptr<TextureMaterial>& textureMaterial) {
    Submission submission;
    submission.vertex_array    = vertexArray;
    submission.material        = material;
    submission.Transform       = transform;
    submission.CastsShadows    = castsShadows;
    submission.ReceiveShadows  = receiveShadows;
    submission.EmissiveColor   = pbrParams.EmissiveColor;
    submission.EmissiveFactor  = pbrParams.EmissiveFactor;
    
    // Per-object PBR override
    submission.UseCustomPBR    = true;
    submission.BaseColor       = pbrParams.BaseColor;
    submission.Metallic        = pbrParams.Metallic;
    submission.Roughness       = pbrParams.Roughness;
    submission.Reflectance     = pbrParams.Reflectance;
    submission.AO              = pbrParams.AO;
    
    // Texture material for PBR textures
    submission.textureMaterial = textureMaterial;

    // Extract position from transform
    submission.Center = Vector3(transform[3]);

    // Create stable ObjectId based on position hash
    auto hashFloat      = [](float f) -> uint32_t { return *reinterpret_cast<uint32_t*>(&f); };
    submission.ObjectId = hashFloat(submission.Center.x) ^ (hashFloat(submission.Center.y) << 8) ^
                          (hashFloat(submission.Center.z) << 16);
    if (submission.ObjectId == 0) submission.ObjectId = 1;

    // Calculate bounding radius considering scale
    float scaleX   = glm::length(Vector3(transform[0]));
    float scaleY   = glm::length(Vector3(transform[1]));
    float scaleZ   = glm::length(Vector3(transform[2]));
    float maxScale = glm::max(glm::max(scaleX, scaleY), scaleZ);
    submission.BoundingRadius = 0.866f * maxScale;  // Unit cube bounding sphere

    sceneData_.Submissions.emplace_back(std::move(submission));
}

void SceneRenderer::SetDirectionalLight(const DirectionalLightData& light) {
    sceneData_.directional_light        = light;
    sceneData_.directional_light.Active = true;

    if (glm::length(sceneData_.directional_light.Direction) <= 0.0f) {
        sceneData_.directional_light.Direction = Vector3(0.0f, -1.0f, 0.0f);
    } else {
        sceneData_.directional_light.Direction =
            glm::normalize(sceneData_.directional_light.Direction);
    }

    sceneData_.directional_light.Intensity = glm::max(light.Intensity, 0.0f);
}

void SceneRenderer::ClearDirectionalLight() {
    sceneData_.directional_light = DirectionalLightData{};
    sceneData_.ShadowsEnabled    = false;
    sceneData_.LightSpaceMatrix  = Matrix4(1.0f);
}

SceneRenderer::DirectionalLightData SceneRenderer::GetDirectionalLight() const {
    return sceneData_.directional_light;
}

uint32_t SceneRenderer::GetShadowDepthTexture() const {
    return sceneData_.ShadowDepthTexture;
}

Matrix4 SceneRenderer::GetLightSpaceMatrix() const {
    return sceneData_.LightSpaceMatrix;
}

bool SceneRenderer::IsShadowsEnabled() const {
    return sceneData_.ShadowsEnabled;
}

void SceneRenderer::SetShadowMapSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        SE_LOG_WARN("Invalid shadow map size: {}x{}, using default 1024x1024", width, height);
        width  = 1024;
        height = 1024;
    }

    if (sceneData_.ShadowMapSize.x != width || sceneData_.ShadowMapSize.y != height) {
        sceneData_.ShadowMapSize = glm::ivec2(width, height);
        if (initialized_) {
            DestroyShadowResources();
            InitializeShadowResources();
        }
    }
}

void SceneRenderer::SetShadowDistance(float distance) {
    sceneData_.ShadowDistance = glm::max(distance, 1.0f);
}

void SceneRenderer::SetShadowOrthoSize(float size) {
    sceneData_.ShadowOrthoSize = glm::max(size, 1.0f);
}

void SceneRenderer::SetAmbientStrength(float strength) {
    sceneData_.AmbientStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void SceneRenderer::SetAOStrength(float strength) {
    sceneData_.AOStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void SceneRenderer::SetAORadius(float radius) {
    sceneData_.AORadius = glm::max(radius, 0.1f);
}

void SceneRenderer::SetEnvironmentLighting(const IBLData& ibl) {
    iblData_ = ibl;
}

void SceneRenderer::InitializeShadowResources() {
    SE_LOG_INFO("Creating shadow resources ({}x{})", sceneData_.ShadowMapSize.x,
                sceneData_.ShadowMapSize.y);

    sceneData_.ShadowShader = std::make_shared<Shader>(kShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.ShadowShader || sceneData_.ShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create shadow shader");
        return;
    }

    // Create instanced shadow shader (uses same fragment, different vertex for instance buffer)
    sceneData_.InstancedShadowShader =
        std::make_shared<Shader>(kInstancedShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.InstancedShadowShader || sceneData_.InstancedShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create instanced shadow shader");
        return;
    }
    SE_LOG_INFO("Created instanced shadow shader successfully");

    // Create skinned shadow shader (includes bone matrix transforms)
    sceneData_.SkinnedShadowShader =
        std::make_shared<Shader>(kSkinnedShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.SkinnedShadowShader || sceneData_.SkinnedShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create skinned shadow shader");
        return;
    }
    SE_LOG_INFO("Created skinned shadow shader successfully");

    glGenFramebuffers(1, &sceneData_.ShadowFramebuffer);
    if (sceneData_.ShadowFramebuffer == 0) {
        SE_LOG_ERROR("Failed to generate shadow framebuffer");
        return;
    }

    glGenTextures(1, &sceneData_.ShadowDepthTexture);
    if (sceneData_.ShadowDepthTexture == 0) {
        SE_LOG_ERROR("Failed to generate shadow depth texture");
        glDeleteFramebuffers(1, &sceneData_.ShadowFramebuffer);
        sceneData_.ShadowFramebuffer = 0;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, sceneData_.ShadowDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sceneData_.ShadowMapSize.x,
                 sceneData_.ShadowMapSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    GLenum texError = glGetError();
    if (texError != GL_NO_ERROR) {
        SE_LOG_ERROR("GL error creating shadow depth texture: 0x{:X}", texError);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    const float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneData_.ShadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           sceneData_.ShadowDepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("Shadow framebuffer incomplete, status: 0x{:X}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DestroyShadowResources();
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SE_LOG_INFO("Shadow resources created successfully");
}

void SceneRenderer::DestroyShadowResources() {
    if (sceneData_.ShadowDepthTexture) {
        glDeleteTextures(1, &sceneData_.ShadowDepthTexture);
        sceneData_.ShadowDepthTexture = 0;
    }
    if (sceneData_.ShadowFramebuffer) {
        glDeleteFramebuffers(1, &sceneData_.ShadowFramebuffer);
        sceneData_.ShadowFramebuffer = 0;
    }
    sceneData_.ShadowShader.reset();
    sceneData_.InstancedShadowShader.reset();
}

void SceneRenderer::RenderShadowPass() {
    if (sceneData_.Submissions.empty() && instancedSubmissions_.empty()) return;
    if (!sceneData_.ShadowShader || !sceneData_.ShadowFramebuffer) return;

    // Save current framebuffer and viewport
    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glViewport(0, 0, sceneData_.ShadowMapSize.x, sceneData_.ShadowMapSize.y);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneData_.ShadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    GLboolean wasCullEnabled       = glIsEnabled(GL_CULL_FACE);
    GLint     previousCullFaceMode = GL_BACK;
    if (wasCullEnabled) glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    sceneData_.ShadowShader->bind();
    sceneData_.ShadowShader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);

    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.CastsShadows) continue;
        if (!submission.vertex_array) continue;

        sceneData_.ShadowShader->setMat4("uModel", submission.Transform);
        RenderCommand::DrawIndexed(submission.vertex_array.get());
    }

    // Render shadows for instanced submissions using instanced shadow shader
    if (!instancedSubmissions_.empty() && sceneData_.InstancedShadowShader) {
        sceneData_.InstancedShadowShader->bind();
        sceneData_.InstancedShadowShader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);

        for (const auto& instanced : instancedSubmissions_) {
            if (!instanced.castsShadows) continue;
            if (!instanced.instancedMesh) continue;

            auto va = instanced.instancedMesh->GetVertexArray();
            if (!va) continue;

            uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
            if (instanceCount == 0) continue;

            // Draw all instances in a single call - shader reads transform from instance buffer
            RenderCommand::DrawIndexedInstanced(va.get(), instanceCount);
        }
    }

    glCullFace(previousCullFaceMode);
    if (!wasCullEnabled) glDisable(GL_CULL_FACE);

    // Restore previous framebuffer and viewport (not just 0!)
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void SceneRenderer::RenderGBufferPass() {
    if (!gbuffer_ || !gbuffer_->IsInitialized()) return;
    if (!gbufferShader_ || !gbufferInstancedShader_) return;
    
    // Save previous state
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    
    // Bind G-Buffer and clear
    gbuffer_->Bind();
    gbuffer_->Clear();
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    
    // Render regular submissions with per-object emissive values
    gbufferShader_->bind();
    gbufferShader_->setMat4("uView", sceneData_.ViewMatrix);
    gbufferShader_->setMat4("uProj", sceneData_.ProjectionMatrix);
    
    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.vertex_array) continue;
        gbufferShader_->setMat4("uModel", submission.Transform);
        gbufferShader_->setVec3("uEmissiveColor", submission.EmissiveColor);
        gbufferShader_->setFloat("uEmissiveFactor", submission.EmissiveFactor);
        RenderCommand::DrawIndexed(submission.vertex_array.get());
    }
    
    // Render instanced submissions with per-batch emissive
    gbufferInstancedShader_->bind();
    gbufferInstancedShader_->setMat4("uView", sceneData_.ViewMatrix);
    gbufferInstancedShader_->setMat4("uProj", sceneData_.ProjectionMatrix);
    
    for (const auto& instanced : instancedSubmissions_) {
        if (!instanced.instancedMesh) continue;
        if (instanced.instancedMesh->GetInstanceCount() == 0) continue;
        
        // Use per-submission emissive values
        gbufferInstancedShader_->setVec3("uEmissiveColor", instanced.EmissiveColor);
        gbufferInstancedShader_->setFloat("uEmissiveFactor", instanced.EmissiveFactor);
        instanced.instancedMesh->DrawWithoutMaterial();
    }

    
    // Unbind FBO and restore previous state
    gbuffer_->Unbind();
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void SceneRenderer::RenderScenePass() {

    glActiveTexture(GL_TEXTURE0);
    if (sceneData_.ShadowsEnabled && sceneData_.ShadowDepthTexture) {
        glBindTexture(GL_TEXTURE_2D, sceneData_.ShadowDepthTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    occlusionCuller_.SetViewProjection(sceneData_.view_projection_matrix);
    occlusionCuller_.BeginFrame();
    occlusionCuller_.ResetStats();

    // Separate into occluders (large) and occludees (small)
    std::vector<const Submission*> occluders;
    std::vector<const Submission*> occludees;
    const float                    kOccluderThreshold = 5.0f;

    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.vertex_array || !submission.material) continue;

        stats_.TotalObjects++;

        // Frustum culling first
        if (frustumCullingEnabled_) {
            if (!occlusionCuller_.IsSphereVisible(submission.Center, submission.BoundingRadius)) {
                stats_.FrustumCulled++;
                continue;
            }
        }

        if (submission.BoundingRadius > kOccluderThreshold) {
            occluders.push_back(&submission);
        } else {
            occludees.push_back(&submission);
        }
    }

    // Lambda to render a single object
    auto renderObject = [this](const Submission& submission) {
        if (!submission.material) return;
        
        submission.material->Bind();
        auto shader = submission.material->GetShader();
        if (!shader) return;

        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setMat4("uModel", submission.Transform);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active
                                                ? sceneData_.directional_light.Intensity
                                                : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 0);
        shader->setFloat("uReceiveShadows", submission.ReceiveShadows ? 1.0f : 0.0f);
        shader->setFloat(
            "uShadowsEnabled",
            sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", sceneData_.AOStrength);
        shader->setFloat("uAORadius", sceneData_.AORadius);
        shader->setInt("uDebugMode", debugMode_);
        
        // Bind SSAO texture if available
        if (ssaoTexture_ != 0) {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, ssaoTexture_);
            shader->setInt("uSSAOTexture", 11);
            shader->setInt("uHasSSAO", 1);
            shader->setVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
        } else {
            shader->setInt("uHasSSAO", 0);
        }

        // Bind PBR texture uniforms if TextureMaterial is present
        auto texMat = submission.textureMaterial;
        if (texMat) {
            int hasAlbedo = 0, hasNormal = 0, hasSpecular = 0, hasAO = 0;
            int hasMetallic = 0, hasRoughness = 0, hasEmissive = 0;
            int hasMetallicRoughness = 0;
            
            if (texMat->HasAlbedo()) {
                texMat->Albedo->Bind(1);
                hasAlbedo = 1;
            }
            if (texMat->HasNormal()) {
                texMat->Normal->Bind(2);
                hasNormal = 1;
            }
            if (texMat->HasSpecular()) {
                texMat->Specular->Bind(3);
                hasSpecular = 1;
            }
            if (texMat->HasAO()) {
                texMat->AO->Bind(4);
                hasAO = 1;
            }
            if (texMat->HasMetallic()) {
                texMat->Metallic->Bind(5);
                hasMetallic = 1;
            }
            if (texMat->HasRoughness()) {
                texMat->Roughness->Bind(6);
                hasRoughness = 1;
            }
            if (texMat->HasEmissive()) {
                texMat->Emissive->Bind(7);
                hasEmissive = 1;
            }
            
            // Texture sampler bindings
            shader->setInt("uAlbedoMap", 1);
            shader->setInt("uNormalMap", 2);
            shader->setInt("uSpecularMap", 3);
            shader->setInt("uAOMap", 4);
            shader->setInt("uMetallicMap", 5);
            shader->setInt("uRoughnessMap", 6);
            shader->setInt("uEmissiveMap", 7);
            
            // Texture presence flags
            shader->setInt("uHasAlbedo", hasAlbedo);
            shader->setInt("uHasNormal", hasNormal);
            shader->setInt("uHasSpecular", hasSpecular);
            shader->setInt("uHasAO", hasAO);
            shader->setInt("uHasMetallic", hasMetallic);
            shader->setInt("uHasRoughness", hasRoughness);
            shader->setInt("uHasEmissive", hasEmissive);
            shader->setInt("uHasMetallicRoughness", hasMetallicRoughness);
            
            // PBR material parameters from TextureMaterial
            shader->setVec4("uBaseColor", texMat->BaseColor);
            shader->setFloat("uMetallicFactor", texMat->MetallicFactor);
            shader->setFloat("uRoughnessFactor", texMat->RoughnessFactor);
            shader->setFloat("uReflectance", 0.5f);
            shader->setFloat("uAOFactor", 1.0f);
            shader->setVec3("uEmissiveColor", texMat->EmissiveColor);
            shader->setFloat("uEmissiveFactor", 1.0f);
            shader->setFloat("uNormalScale", 1.0f);
            shader->setFloat("uShininess", texMat->Shininess);
        } else {
            // Default values when no TextureMaterial
            shader->setInt("uHasAlbedo", 0);
            shader->setInt("uHasNormal", 0);
            shader->setInt("uHasSpecular", 0);
            shader->setInt("uHasAO", 0);
            shader->setInt("uHasMetallic", 0);
            shader->setInt("uHasRoughness", 0);
            shader->setInt("uHasEmissive", 0);
            shader->setInt("uHasMetallicRoughness", 0);
            shader->setVec4("uBaseColor", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
            shader->setFloat("uMetallicFactor", 0.0f);
            shader->setFloat("uRoughnessFactor", 0.5f);
            shader->setFloat("uReflectance", 0.5f);
            shader->setFloat("uAOFactor", 1.0f);
            shader->setVec3("uEmissiveColor", glm::vec3(0.0f));
            shader->setFloat("uEmissiveFactor", 0.0f);
            shader->setFloat("uNormalScale", 1.0f);
            shader->setFloat("uShininess", 32.0f);
        }
        
        // Apply global material override if set (for PBR testing)
        if (globalMaterialOverride_) {
            // Reset texture flags to force shader to use uniform values
            shader->setInt("uHasAlbedo", 0);
            shader->setInt("uHasNormal", 0);
            shader->setInt("uHasMetallic", 0);
            shader->setInt("uHasRoughness", 0);
            shader->setInt("uHasAO", 0);
            shader->setInt("uHasEmissive", 0);
            shader->setInt("uHasMetallicRoughness", 0);
            
            // Apply override values
            shader->setVec4("uBaseColor", globalMaterialOverride_->BaseColor);
            shader->setFloat("uMetallicFactor", globalMaterialOverride_->Metallic);
            shader->setFloat("uRoughnessFactor", globalMaterialOverride_->Roughness);
            shader->setFloat("uReflectance", globalMaterialOverride_->Reflectance);
            shader->setFloat("uAOFactor", globalMaterialOverride_->AO);
            shader->setVec3("uEmissiveColor", globalMaterialOverride_->EmissiveColor);
            shader->setFloat("uEmissiveFactor", globalMaterialOverride_->EmissiveFactor);
            shader->setFloat("uNormalScale", globalMaterialOverride_->NormalScale);
            // Advanced PBR
            shader->setFloat("uClearCoat", globalMaterialOverride_->ClearCoat);
            shader->setFloat("uClearCoatRoughness", globalMaterialOverride_->ClearCoatRoughness);
            shader->setFloat("uAnisotropy", globalMaterialOverride_->Anisotropy);
            shader->setVec3("uAnisotropyDirection", globalMaterialOverride_->AnisotropyDirection);
            shader->setVec3("uSheenColor", globalMaterialOverride_->SheenColor);
            shader->setFloat("uSheenRoughness", globalMaterialOverride_->SheenRoughness);
            shader->setVec3("uSubsurfaceColor", globalMaterialOverride_->SubsurfaceColor);
            shader->setFloat("uSubsurfacePower", globalMaterialOverride_->SubsurfacePower);
            shader->setFloat("uThickness", globalMaterialOverride_->Thickness);
            shader->setFloat("uTransmission", globalMaterialOverride_->Transmission);
            shader->setFloat("uIOR", globalMaterialOverride_->IOR);
        } else if (submission.UseCustomPBR) {
            // Per-object PBR override from Material Editor assignment
            shader->setInt("uHasAlbedo", 0);
            shader->setInt("uHasNormal", 0);
            shader->setInt("uHasMetallic", 0);
            shader->setInt("uHasRoughness", 0);
            shader->setInt("uHasAO", 0);
            shader->setInt("uHasEmissive", 0);
            shader->setInt("uHasMetallicRoughness", 0);
            
            shader->setVec4("uBaseColor", submission.BaseColor);
            shader->setFloat("uMetallicFactor", submission.Metallic);
            shader->setFloat("uRoughnessFactor", submission.Roughness);
            shader->setFloat("uReflectance", submission.Reflectance);
            shader->setFloat("uAOFactor", submission.AO);
            shader->setVec3("uEmissiveColor", submission.EmissiveColor);
            shader->setFloat("uEmissiveFactor", submission.EmissiveFactor);
            shader->setFloat("uNormalScale", 1.0f);
            
            // Default advanced PBR for per-object
            shader->setFloat("uClearCoat", 0.0f);
            shader->setFloat("uClearCoatRoughness", 0.0f);
            shader->setFloat("uAnisotropy", 0.0f);
            shader->setVec3("uAnisotropyDirection", glm::vec3(1.0f, 0.0f, 0.0f));
            shader->setVec3("uSheenColor", glm::vec3(0.0f));
            shader->setFloat("uSheenRoughness", 0.0f);
            shader->setVec3("uSubsurfaceColor", glm::vec3(0.0f));
            shader->setFloat("uSubsurfacePower", 0.0f);
            shader->setFloat("uThickness", 0.0f);
            shader->setFloat("uTransmission", 0.0f);
            shader->setFloat("uIOR", 1.5f);
        } else {
            // Default advanced PBR values
            shader->setFloat("uClearCoat", 0.0f);
            shader->setFloat("uClearCoatRoughness", 0.0f);
            shader->setFloat("uAnisotropy", 0.0f);
            shader->setVec3("uAnisotropyDirection", glm::vec3(1.0f, 0.0f, 0.0f));
            shader->setVec3("uSheenColor", glm::vec3(0.0f));
            shader->setFloat("uSheenRoughness", 0.0f);
            shader->setVec3("uSubsurfaceColor", glm::vec3(0.0f));
            shader->setFloat("uSubsurfacePower", 0.0f);
            shader->setFloat("uThickness", 0.0f);
            shader->setFloat("uTransmission", 0.0f);
            shader->setFloat("uIOR", 1.5f);
        }
        
        // Bind IBL/environment lighting data
        shader->setVec3Array("uSH", iblData_.SphericalHarmonics, 9);
        shader->setFloat("uIBLIntensity", iblData_.Intensity);
        shader->setVec3("uSkyColor", iblData_.SkyColor);
        shader->setVec3("uGroundColor", iblData_.GroundColor);
        
        // Bind HDR IBL cubemaps if available
        if (iblData_.HasCubemaps()) {
            shader->setInt("uHasIBLCubemaps", 1);
            shader->setFloat("uMaxPrefilteredLod", static_cast<float>(iblData_.PrefilteredMipLevels - 1));
            
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblData_.IrradianceCubemap);
            shader->setInt("uIrradianceMap", 9);
            
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblData_.PrefilteredCubemap);
            shader->setInt("uPrefilteredMap", 10);
            
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, iblData_.DfgLut);
            shader->setInt("uDfgLut", 11);
        } else {
            shader->setInt("uHasIBLCubemaps", 0);
        }
        
        // Bind Radiance Cascades GI texture if available
        int hasGI = 0;
        float giIntensity = 1.0f;
        if (radianceCascades_ && radianceCascades_->IsEnabled()) {
            uint32_t giTex = radianceCascades_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
            }
        } else if (sparseRC_ && sparseRC_->IsEnabled()) {
            uint32_t giTex = sparseRC_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
                giIntensity = sparseRC_->GetConfig().GIIntensity;
            }
        } else if (ssgiPass_ && ssgiPass_->IsReady()) {
            uint32_t giTex = ssgiPass_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
                giIntensity = ssgiPass_->GetConfig().Intensity;
            }
        }
        shader->setInt("uHasGI", hasGI);
        shader->setFloat("uGIIntensity", giIntensity);
        
        // Bind Cascaded Shadow Maps if available
        static int csmBindLogCount = 0;
        if (csmEnabled_ && csm_ && csm_->IsInitialized()) {
            shader->setInt("uUseCSM", 1);
            shader->setInt("uCascadeCount", CASCADE_COUNT);
            
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D_ARRAY, csm_->GetTextureArray());
            shader->setInt("uShadowCascades", 12);
            
            // Cascade matrices and split depths
            for (int i = 0; i < CASCADE_COUNT; i++) {
                std::string matName = "uCascadeMatrices[" + std::to_string(i) + "]";
                std::string splitName = "uCascadeSplits[" + std::to_string(i) + "]";
                shader->setMat4(matName.c_str(), csm_->GetCascadeMatrix(i));
                shader->setFloat(splitName.c_str(), csm_->GetCascadeSplit(i));
            }
            
            shader->setInt("uVisualizeCascades", visualizeCascades_ ? 1 : 0);
        } else {
            shader->setInt("uUseCSM", 0);
            shader->setInt("uVisualizeCascades", 0);
        }
        
        // HDR exposure control
        shader->setFloat("uExposure", sceneData_.Exposure);

        if (!submission.vertex_array) return;
        RenderCommand::DrawIndexed(submission.vertex_array.get());

        stats_.VisibleObjects++;
        stats_.DrawCalls++;
        stats_.TriangleCount += submission.vertex_array->GetIndexBuffer()->GetCount() / 3;
    };

    // PHASE 1: Render all OCCLUDERS first to fill depth buffer
    for (const auto* submission : occluders) { renderObject(*submission); }

    // PHASE 2: Test occludee bounding boxes against depth buffer filled by occluders
    if (occlusionCullingEnabled_ && occlusionCuller_.IsEnabled()) {
        for (const auto* submission : occludees) {
            occlusionCuller_.BeginQuery(submission->ObjectId);
            occlusionCuller_.RenderBoundingBox(submission->Center,
                                               Vector3(submission->BoundingRadius));
            occlusionCuller_.EndQuery();
        }

        // Collect results immediately (blocking) to use this frame
        occlusionCuller_.CollectResults();
    }

    // PHASE 3: Render occludees that passed the visibility test
    for (const auto* submission : occludees) {
        if (occlusionCullingEnabled_ && occlusionCuller_.IsEnabled()) {
            if (!occlusionCuller_.WasVisibleLastFrame(submission->ObjectId)) {
                stats_.OcclusionCulled++;
                continue;  // Skip - occluded by occluders
            }
        }
        renderObject(*submission);
    }

    // PHASE 4: Render instanced batches (no culling - already handled by RenderSystem)
    for (const auto& instanced : instancedSubmissions_) {
        if (!instanced.instancedMesh || !instanced.material) continue;

        uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
        if (instanceCount == 0) continue;

        instanced.material->Bind();
        auto shader = instanced.material->GetShader();
        if (!shader) continue;

        // Set uniforms (same as normal rendering, but no uModel - that comes from instance buffer)
        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active
                                                ? sceneData_.directional_light.Intensity
                                                : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 0);
        shader->setFloat("uReceiveShadows", instanced.receiveShadows ? 1.0f : 0.0f);
        shader->setFloat(
            "uShadowsEnabled",
            sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", sceneData_.AOStrength);
        shader->setFloat("uAORadius", sceneData_.AORadius);
        shader->setInt("uDebugMode", debugMode_);
        
        // Bind SSAO texture if available
        if (ssaoTexture_ != 0) {
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, ssaoTexture_);
            shader->setInt("uSSAOTexture", 11);
            shader->setInt("uHasSSAO", 1);
            shader->setVec2("uScreenSize", glm::vec2(screenWidth_, screenHeight_));
        } else {
            shader->setInt("uHasSSAO", 0);
        }
        
        // Apply global material override if set (for PBR testing)
        if (globalMaterialOverride_) {
            shader->setInt("uUseBaseColorOverride", 1);
            shader->setVec4("uBaseColor", globalMaterialOverride_->BaseColor);
            shader->setFloat("uMetallicFactor", globalMaterialOverride_->Metallic);
            shader->setFloat("uRoughnessFactor", globalMaterialOverride_->Roughness);
            shader->setFloat("uReflectance", globalMaterialOverride_->Reflectance);
            shader->setFloat("uClearCoat", globalMaterialOverride_->ClearCoat);
            shader->setFloat("uClearCoatRoughness", globalMaterialOverride_->ClearCoatRoughness);
            shader->setFloat("uAnisotropy", globalMaterialOverride_->Anisotropy);
            shader->setVec3("uSheenColor", globalMaterialOverride_->SheenColor);
            shader->setFloat("uSheenRoughness", globalMaterialOverride_->SheenRoughness);
        } else {
            // Set default PBR values when no global override
            shader->setInt("uUseBaseColorOverride", 0);
            shader->setVec4("uBaseColor", glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
            shader->setFloat("uMetallicFactor", 0.0f);
            shader->setFloat("uRoughnessFactor", 0.9f);  // Nearly matte surfaces by default
            shader->setFloat("uReflectance", 0.5f);
            shader->setFloat("uClearCoat", 0.0f);
            shader->setFloat("uClearCoatRoughness", 0.0f);
            shader->setFloat("uAnisotropy", 0.0f);
            shader->setVec3("uSheenColor", glm::vec3(0.0f));
            shader->setFloat("uSheenRoughness", 0.0f);
        }
        
        // IBL for instanced
        shader->setVec3Array("uSH", iblData_.SphericalHarmonics, 9);
        shader->setFloat("uIBLIntensity", iblData_.Intensity);
        shader->setVec3("uSkyColor", iblData_.SkyColor);
        shader->setVec3("uGroundColor", iblData_.GroundColor);
        shader->setFloat("uExposure", sceneData_.Exposure);
        
        // Bind HDR IBL cubemaps if available
        if (iblData_.HasCubemaps()) {
            shader->setInt("uHasIBLCubemaps", 1);
            shader->setFloat("uMaxPrefilteredLod", static_cast<float>(iblData_.PrefilteredMipLevels - 1));
            
            glActiveTexture(GL_TEXTURE9);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblData_.IrradianceCubemap);
            shader->setInt("uIrradianceMap", 9);
            
            glActiveTexture(GL_TEXTURE10);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblData_.PrefilteredCubemap);
            shader->setInt("uPrefilteredMap", 10);
            
            glActiveTexture(GL_TEXTURE11);
            glBindTexture(GL_TEXTURE_2D, iblData_.DfgLut);
            shader->setInt("uDfgLut", 11);
        } else {
            shader->setInt("uHasIBLCubemaps", 0);
        }
        
        // Bind Cascaded Shadow Maps if available (instanced)
        if (csmEnabled_ && csm_ && csm_->IsInitialized()) {
            shader->setInt("uUseCSM", 1);
            shader->setInt("uCascadeCount", CASCADE_COUNT);
            
            glActiveTexture(GL_TEXTURE12);
            glBindTexture(GL_TEXTURE_2D_ARRAY, csm_->GetTextureArray());
            shader->setInt("uShadowCascades", 12);
            
            for (int i = 0; i < CASCADE_COUNT; i++) {
                std::string matName = "uCascadeMatrices[" + std::to_string(i) + "]";
                std::string splitName = "uCascadeSplits[" + std::to_string(i) + "]";
                shader->setMat4(matName.c_str(), csm_->GetCascadeMatrix(i));
                shader->setFloat(splitName.c_str(), csm_->GetCascadeSplit(i));
            }
        } else {
            shader->setInt("uUseCSM", 0);
        }
        
        // Bind Radiance Cascades GI texture if available
        int hasGI = 0;
        float giIntensity = 1.0f;
        if (radianceCascades_ && radianceCascades_->IsEnabled()) {
            uint32_t giTex = radianceCascades_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
            }
        } else if (sparseRC_ && sparseRC_->IsEnabled()) {
            uint32_t giTex = sparseRC_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
                giIntensity = sparseRC_->GetConfig().GIIntensity;
            }
        } else if (ssgiPass_ && ssgiPass_->IsReady()) {
            uint32_t giTex = ssgiPass_->GetRadianceTexture();
            if (giTex != 0) {
                glActiveTexture(GL_TEXTURE8);
                glBindTexture(GL_TEXTURE_2D, giTex);
                shader->setInt("uGIMap", 8);
                hasGI = 1;
                giIntensity = ssgiPass_->GetConfig().Intensity;
            }
        }
        shader->setInt("uHasGI", hasGI);
        shader->setFloat("uGIIntensity", giIntensity);

        // Debug log for instanced path
        static int instGiLogCount = 0;
        if (instGiLogCount++ < 10) {
            printf("[Instanced] GI Binding: hasGI=%d, giTex=%u, giIntensity=%.2f\n", 
                hasGI, ssgiPass_ ? ssgiPass_->GetRadianceTexture() : 0, giIntensity);
        }
        // Single draw call for all instances in this batch
        instanced.instancedMesh->DrawWithoutMaterial();

        stats_.InstancedBatches++;
        stats_.InstancedObjects += instanceCount;
        stats_.DrawCalls++;

        const auto& indexBuffer = instanced.instancedMesh->GetVertexArray()->GetIndexBuffer();
        if (indexBuffer) { stats_.TriangleCount += (indexBuffer->GetCount() / 3) * instanceCount; }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void SceneRenderer::SetRadianceCascadesEnabled(bool enabled) {
    if (radianceCascades_) {
        radianceCascades_->SetEnabled(enabled);
    }
}

bool SceneRenderer::IsRadianceCascadesEnabled() const {
    return radianceCascades_ && radianceCascades_->IsEnabled();
}

RadianceCascadeConfig& SceneRenderer::GetRadianceCascadeConfig() {
    static RadianceCascadeConfig fallback;
    if (radianceCascades_) {
        return const_cast<RadianceCascadeConfig&>(radianceCascades_->GetConfig());
    }
    return fallback;
}

const RadianceCascadeConfig& SceneRenderer::GetRadianceCascadeConfig() const {
    static RadianceCascadeConfig fallback;
    if (radianceCascades_) {
        return radianceCascades_->GetConfig();
    }
    return fallback;
}

void SceneRenderer::SetScreenSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (screenWidth_ == width && screenHeight_ == height) return;
    
    SE_LOG_INFO("SceneRenderer: SetScreenSize({}x{})", width, height);
    
    screenWidth_ = width;
    screenHeight_ = height;
    
    // Initialize or resize G-Buffer
    if (gbuffer_) {
        if (!gbuffer_->IsInitialized()) {
            gbuffer_->Init(width, height);
        } else {
            gbuffer_->Resize(width, height);
        }
    }
    
    // TODO(GI): Re-enable Radiance Cascades initialization when feature is ready
    // if (radianceCascades_) {
    //     if (!radianceCascades_->IsEnabled() || radianceCascades_->GetRadianceTexture() == 0) {
    //         radianceCascades_->Init(width, height);
    //     } else {
    //         radianceCascades_->Resize(width, height);
    //     }
    // }
    
    // TODO(GI): Re-enable Sparse Radiance Cascades (3D) initialization when feature is ready
    // if (!voxelizer_) {
    //     voxelizer_ = std::make_shared<SceneVoxelizer>();
    //     VoxelGridConfig voxelConfig;
    //     voxelConfig.Resolution = 128;
    //     voxelConfig.WorldSize = 50.0f;
    //     voxelConfig.Center = glm::vec3(0.0f);
    //     voxelizer_->Init(voxelConfig);
    // }
    // 
    // if (!sparseRC_) {
    //     sparseRC_ = std::make_unique<SparseRadianceCascades>();
    //     sparseRC_->Init(width, height, voxelizer_);
    //     sparseRC_->SetEnabled(false);
    // } else {
    //     sparseRC_->Resize(width, height);
    // }
}

void SceneRenderer::SetSparseRCEnabled(bool enabled) {
    // Auto-initialize Sparse RC if trying to enable but not yet created
    if (enabled && !sparseRC_ && screenWidth_ > 0 && screenHeight_ > 0) {
        SE_LOG_INFO("Auto-initializing Sparse RC on enable");
        
        if (!voxelizer_) {
            voxelizer_ = std::make_shared<SceneVoxelizer>();
            VoxelGridConfig voxelConfig;
            voxelConfig.Resolution = 128;
            voxelConfig.WorldSize = 50.0f;
            voxelConfig.Center = glm::vec3(0.0f);
            voxelizer_->Init(voxelConfig);
        }
        
        sparseRC_ = std::make_unique<SparseRadianceCascades>();
        sparseRC_->Init(screenWidth_, screenHeight_, voxelizer_);
    }
    
    if (sparseRC_) {
        sparseRC_->SetEnabled(enabled);
        // When enabling Sparse RC, disable old 2D RC
        if (enabled && radianceCascades_) {
            radianceCascades_->SetEnabled(false);
        }
    }
}


bool SceneRenderer::IsSparseRCEnabled() const {
    return sparseRC_ && sparseRC_->IsEnabled();
}

SparseRCConfig& SceneRenderer::GetSparseRCConfig() {
    static SparseRCConfig fallback;
    if (sparseRC_) {
        return const_cast<SparseRCConfig&>(sparseRC_->GetConfig());
    }
    return fallback;
}

// SSGI (Screen Space Global Illumination) - HBIL
void SceneRenderer::SetSSGIEnabled(bool enabled) {
    if (ssgiPass_) {
        ssgiPass_->SetEnabled(enabled);
        // When enabling SSGI, disable old RC systems
        if (enabled) {
            if (radianceCascades_) radianceCascades_->SetEnabled(false);
            if (sparseRC_) sparseRC_->SetEnabled(false);
        }
        SE_LOG_INFO("SSGI {}", enabled ? "enabled" : "disabled");
    }
}

bool SceneRenderer::IsSSGIEnabled() const {
    return ssgiPass_ && ssgiPass_->IsEnabled();
}

SSGIConfig& SceneRenderer::GetSSGIConfig() {
    static SSGIConfig fallback;
    if (ssgiPass_) {
        return ssgiPass_->GetConfig();
    }
    return fallback;
}

const SSGIConfig& SceneRenderer::GetSSGIConfig() const {
    static SSGIConfig fallback;
    if (ssgiPass_) {
        return ssgiPass_->GetConfig();
    }
    return fallback;
}

void SceneRenderer::InitSkybox() {
    if (skyboxInitialized_) return;
    
    namespace fs = std::filesystem;
    fs::path assetsPath = fs::current_path() / "assets";
    fs::path vertPath = assetsPath / "shaders" / "environment" / "skybox.vert";
    fs::path fragPath = assetsPath / "shaders" / "environment" / "skybox.frag";
    
    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_WARN("[Skybox] Shaders not found, skybox disabled");
        return;
    }
    
    skyboxShader_ = Shader::CreateFromFiles(vertPath, fragPath);
    if (!skyboxShader_) {
        SE_LOG_ERROR("[Skybox] Failed to create shader");
        return;
    }
    
    // Unit cube vertices
    float vertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &skyboxVAO_);
    glGenBuffers(1, &skyboxVBO_);
    glBindVertexArray(skyboxVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    
    skyboxInitialized_ = true;
    SE_LOG_INFO("[Skybox] Initialized successfully");
}

void SceneRenderer::RenderSkybox() {
    if (!skyboxInitialized_ || !iblData_.HasCubemaps()) return;
    
    // Render skybox last with depth test but no depth write
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    
    skyboxShader_->bind();
    skyboxShader_->setMat4("uProjection", sceneData_.ProjectionMatrix);
    skyboxShader_->setMat4("uView", sceneData_.ViewMatrix);
    skyboxShader_->setFloat("uExposure", sceneData_.Exposure);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, iblData_.EnvironmentCubemap);  // Use original HDR env for skybox
    skyboxShader_->setInt("uSkybox", 0);
    
    glBindVertexArray(skyboxVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void SceneRenderer::RenderCSMPass() {
    static int csmLogCount = 0;
    
    if (!csm_ || !csm_->IsInitialized()) {
        if (csmLogCount++ < 5) printf("[CSM] Skipped: CSM not initialized\n");
        return;
    }
    if (!csmEnabled_) {
        if (csmLogCount++ < 5) printf("[CSM] Skipped: CSM disabled\n");
        return;
    }
    if (sceneData_.Submissions.empty() && instancedSubmissions_.empty() && skinnedSubmissions_.empty()) {
        if (csmLogCount++ < 5) printf("[CSM] Skipped: No submissions\n");
        return;
    }
    
    // Debug log for skinned submissions
    static int skinnedShadowLogCount = 0;
    if (!skinnedSubmissions_.empty() && skinnedShadowLogCount++ < 10) {
        printf("[CSM] Skinned submissions: %zu\n", skinnedSubmissions_.size());
    }
    if (!sceneData_.directional_light.Active || !sceneData_.directional_light.CastShadows) {
        if (csmLogCount++ < 5) printf("[CSM] Skipped: Light inactive or no shadows (active=%d, castShadows=%d)\n", 
            sceneData_.directional_light.Active, sceneData_.directional_light.CastShadows);
        return;
    }
    if (!sceneData_.CurrentCamera) {
        if (csmLogCount++ < 5) printf("[CSM] Skipped: No camera\n");
        return;  // Need camera for cascade calculation
    }
    
    // Calculate cascade splits and matrices
    // Light direction should be normalized and pointing FROM the light
    float aspectRatio = (float)screenWidth_ / (float)screenHeight_;
    glm::vec3 lightDir = glm::normalize(sceneData_.directional_light.Direction);
    csm_->CalculateCascades(*sceneData_.CurrentCamera, lightDir, aspectRatio, 100.0f);
    
    // Save current state
    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    
    GLboolean wasCullEnabled = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFaceMode = GL_BACK;
    if (wasCullEnabled) glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    
    // Ensure depth testing and writing are enabled for shadow pass
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    csm_->BeginShadowPass();
    
    // Render each cascade
    for (int cascade = 0; cascade < CASCADE_COUNT; cascade++) {
        csm_->BeginCascade(cascade);
        
        glm::mat4 lightSpaceMatrix = csm_->GetCascadeMatrix(cascade);
        
        // Render regular submissions
        if (sceneData_.ShadowShader) {
            sceneData_.ShadowShader->bind();
            sceneData_.ShadowShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
            
            for (const auto& submission : sceneData_.Submissions) {
                if (!submission.CastsShadows) continue;
                if (!submission.vertex_array) continue;
                
                sceneData_.ShadowShader->setMat4("uModel", submission.Transform);
                RenderCommand::DrawIndexed(submission.vertex_array.get());
            }
        }
        
        // Render instanced submissions
        if (!instancedSubmissions_.empty() && sceneData_.InstancedShadowShader) {
            sceneData_.InstancedShadowShader->bind();
            sceneData_.InstancedShadowShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
            
            for (const auto& instanced : instancedSubmissions_) {
                if (!instanced.castsShadows) continue;
                if (!instanced.instancedMesh) continue;
                
                auto va = instanced.instancedMesh->GetVertexArray();
                if (!va) continue;
                
                uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
                if (instanceCount == 0) continue;
                
            RenderCommand::DrawIndexedInstanced(va.get(), instanceCount);
            }
        }
        
        // Render skinned model submissions
        if (!skinnedSubmissions_.empty() && sceneData_.SkinnedShadowShader) {
            sceneData_.SkinnedShadowShader->bind();
            sceneData_.SkinnedShadowShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
            
            for (const auto& skinned : skinnedSubmissions_) {
                if (skinned.vaoId == 0 || skinned.indexCount == 0) continue;
                
                static int skinnedDrawLog = 0;
                if (skinnedDrawLog++ < 20) {
                    printf("[CSM] Drawing skinned mesh: vao=%u, indices=%u, bones=%zu\n", 
                        skinned.vaoId, skinned.indexCount, skinned.boneMatrices.size());
                }
                
                sceneData_.SkinnedShadowShader->setMat4("uModel", skinned.transform);
                sceneData_.SkinnedShadowShader->setInt("uHasBones", skinned.hasBones ? 1 : 0);
                
                if (skinned.hasBones && !skinned.boneMatrices.empty()) {
                    for (size_t i = 0; i < skinned.boneMatrices.size() && i < 128; ++i) {
                        char uniformName[64];
                        snprintf(uniformName, sizeof(uniformName), "uBoneMatrices[%zu]", i);
                        sceneData_.SkinnedShadowShader->setMat4(uniformName, skinned.boneMatrices[i]);
                    }
                }
                
                // Draw using raw GL since SkinnedMesh doesn't use VertexArray wrapper
                glBindVertexArray(skinned.vaoId);
                glDrawElements(GL_TRIANGLES, skinned.indexCount, GL_UNSIGNED_INT, nullptr);
            }
            glBindVertexArray(0);
        }
    }
    
    csm_->EndShadowPass();
    
    // Restore state
    glCullFace(previousCullFaceMode);
    if (!wasCullEnabled) glDisable(GL_CULL_FACE);
    
    // Restore depth state
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
}

void SceneRenderer::SetCSMSplitLambda(float lambda) {
    if (csm_) {
        csm_->SetSplitLambda(lambda);
    }
}

void SceneRenderer::InitHDRFramebuffer() {
    if (hdrFBO_ != 0) return;  // Already initialized
    
    SE_LOG_INFO("[SceneRenderer] Creating HDR framebuffer {}x{}", screenWidth_, screenHeight_);
    
    glGenFramebuffers(1, &hdrFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO_);
    
    // Color attachment (HDR)
    glGenTextures(1, &hdrColorTexture_);
    glBindTexture(GL_TEXTURE_2D, hdrColorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screenWidth_, screenHeight_, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColorTexture_, 0);
    
    // Depth attachment (renderbuffer)
    glGenRenderbuffers(1, &hdrDepthRBO_);
    glBindRenderbuffer(GL_RENDERBUFFER, hdrDepthRBO_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth_, screenHeight_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdrDepthRBO_);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("[SceneRenderer] HDR framebuffer incomplete!");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneRenderer::DestroyHDRFramebuffer() {
    if (hdrColorTexture_) {
        glDeleteTextures(1, &hdrColorTexture_);
        hdrColorTexture_ = 0;
    }
    if (hdrDepthRBO_) {
        glDeleteRenderbuffers(1, &hdrDepthRBO_);
        hdrDepthRBO_ = 0;
    }
    if (hdrFBO_) {
        glDeleteFramebuffers(1, &hdrFBO_);
        hdrFBO_ = 0;
    }
}

}  // namespace se
