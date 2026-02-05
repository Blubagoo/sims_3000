#ifndef TOON_PIPELINE_H
#define TOON_PIPELINE_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

// Forward declarations
class GPUDevice;
class GPUMesh;
class Camera;

namespace poc1 {
class InstanceBuffer;
class Benchmark;
struct ModelGroup;
}

/**
 * ToonPipeline - Two-pass toon rendering pipeline.
 *
 * Implements a classic cel-shading effect with:
 * - Pass 1: Outline rendering using inverted hull technique (front-face culling)
 * - Pass 2: Toon shading with stepped lighting (back-face culling)
 *
 * Uses instanced rendering with storage buffers for efficient batching.
 */
class ToonPipeline {
public:
    /**
     * Construct a ToonPipeline with an associated GPU device.
     * @param device Reference to the GPU device for resource creation
     */
    explicit ToonPipeline(GPUDevice& device);

    /**
     * Destructor - Releases all GPU resources.
     */
    ~ToonPipeline();

    // Non-copyable
    ToonPipeline(const ToonPipeline&) = delete;
    ToonPipeline& operator=(const ToonPipeline&) = delete;

    // Movable
    ToonPipeline(ToonPipeline&& other) noexcept;
    ToonPipeline& operator=(ToonPipeline&& other) noexcept;

    /**
     * Initialize the pipeline by loading shaders and creating GPU resources.
     *
     * @param shaderPath Base path to shader directory (e.g., "shaders/")
     * @return true if initialization succeeded, false otherwise
     */
    bool Initialize(const char* shaderPath);

    /**
     * Set the camera for view/projection transformations.
     *
     * @param camera The camera to use for rendering
     */
    void SetCamera(const Camera& camera);

    /**
     * Render multiple meshes with instanced toon shading.
     *
     * Executes two passes per model group:
     * 1. Outline pass with front-face culling (inverted hull)
     * 2. Toon pass with back-face culling (normal rendering)
     *
     * @param commandBuffer Active command buffer for recording
     * @param swapchain Swapchain texture to render to
     * @param meshes Vector of meshes, indexed by ModelGroup::modelIndex
     * @param instances Instance buffer with all instances (sorted by model)
     * @param modelGroups Contiguous instance ranges per model
     * @param benchmark Benchmark object for tracking draw calls
     */
    void Render(
        SDL_GPUCommandBuffer* commandBuffer,
        SDL_GPUTexture* swapchain,
        const std::vector<std::unique_ptr<GPUMesh>>& meshes,
        poc1::InstanceBuffer& instances,
        const std::vector<poc1::ModelGroup>& modelGroups,
        poc1::Benchmark& benchmark
    );

    /**
     * Clean up all GPU resources.
     * Called automatically by destructor.
     */
    void Cleanup();

    /**
     * Check if the pipeline was initialized successfully.
     * @return true if ready for rendering
     */
    bool IsValid() const;

private:
    // Uniform buffer data structures
    struct ViewProjectionUniforms {
        glm::mat4 viewProjection;
    };

    struct OutlineUniforms {
        float outlineThickness;
        float padding[3]; // Align to 16 bytes
    };

    // Helper methods
    bool CreateShaders(const char* shaderPath);
    bool CreatePipelines(SDL_GPUTextureFormat swapchainFormat);
    bool CreateDepthTexture(uint32_t width, uint32_t height);
    void ReleaseDepthTexture();

    // Device reference
    GPUDevice* m_device = nullptr;

    // Shaders
    SDL_GPUShader* m_toonVertexShader = nullptr;
    SDL_GPUShader* m_toonFragmentShader = nullptr;
    SDL_GPUShader* m_outlineVertexShader = nullptr;
    SDL_GPUShader* m_outlineFragmentShader = nullptr;

    // Pipelines
    SDL_GPUGraphicsPipeline* m_toonPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_outlinePipeline = nullptr;

    // Depth buffer
    SDL_GPUTexture* m_depthTexture = nullptr;
    uint32_t m_depthWidth = 0;
    uint32_t m_depthHeight = 0;

    // Uniform data
    ViewProjectionUniforms m_vpUniforms = {};
    OutlineUniforms m_outlineUniforms = {};

    // State
    bool m_initialized = false;
};

#endif // TOON_PIPELINE_H
