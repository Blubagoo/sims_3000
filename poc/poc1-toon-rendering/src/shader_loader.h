#pragma once

#include <SDL3/SDL.h>
#include <string>

namespace sims3000 {

class ShaderLoader {
public:
    enum class Stage {
        Vertex,
        Fragment
    };

    struct ShaderResources {
        uint32_t numSamplers = 0;
        uint32_t numStorageTextures = 0;
        uint32_t numStorageBuffers = 0;
        uint32_t numUniformBuffers = 0;
    };

    // Load a pre-compiled shader from disk.
    // basePath is the path without extension, e.g. "shaders/toon.vert"
    // The loader auto-detects the right format (.spv or .dxil) based on the GPU backend.
    static SDL_GPUShader* LoadShader(
        SDL_GPUDevice* device,
        const char* basePath,
        Stage stage,
        const char* entryPoint = "main",
        const ShaderResources& resources = {}
    );

    // Create a graphics pipeline with commonly used defaults
    static SDL_GPUGraphicsPipeline* LoadGraphicsPipeline(
        SDL_GPUDevice* device,
        SDL_GPUShader* vertexShader,
        SDL_GPUShader* fragmentShader,
        const SDL_GPUVertexInputState& vertexInputState,
        const SDL_GPUColorTargetDescription& colorTargetDesc,
        SDL_GPUTextureFormat depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_BACK,
        SDL_GPUFrontFace frontFace = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
        bool enableDepthTest = true,
        bool enableDepthWrite = true
    );

private:
    ShaderLoader() = delete;

    static bool LoadBinaryFile(const char* path, std::string& outData);
};

} // namespace sims3000
