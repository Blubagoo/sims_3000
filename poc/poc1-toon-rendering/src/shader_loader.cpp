#include "shader_loader.h"

namespace sims3000 {

bool ShaderLoader::LoadBinaryFile(const char* path, std::string& outData)
{
    if (!path) return false;

    SDL_IOStream* file = SDL_IOFromFile(path, "rb");
    if (!file) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Cannot open file %s: %s", path, SDL_GetError());
        return false;
    }

    Sint64 size = SDL_GetIOSize(file);
    if (size <= 0) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: File %s is empty or unreadable", path);
        SDL_CloseIO(file);
        return false;
    }

    outData.resize(static_cast<size_t>(size));
    size_t bytesRead = SDL_ReadIO(file, outData.data(), static_cast<size_t>(size));
    SDL_CloseIO(file);

    if (bytesRead != static_cast<size_t>(size)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Failed to read file %s", path);
        outData.clear();
        return false;
    }

    return true;
}

SDL_GPUShader* ShaderLoader::LoadShader(
    SDL_GPUDevice* device,
    const char* basePath,
    Stage stage,
    const char* entryPoint,
    const ShaderResources& resources)
{
    if (!device || !basePath) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Invalid device or path");
        return nullptr;
    }

    // Query which shader formats the device supports
    SDL_GPUShaderFormat supportedFormats = SDL_GetGPUShaderFormats(device);

    // Try to load the best format for this backend
    std::string bytecode;
    SDL_GPUShaderFormat chosenFormat = SDL_GPU_SHADERFORMAT_INVALID;
    std::string chosenPath;

    // Prefer DXIL (D3D12), fall back to SPIRV (Vulkan)
    if (supportedFormats & SDL_GPU_SHADERFORMAT_DXIL) {
        chosenPath = std::string(basePath) + ".dxil";
        if (LoadBinaryFile(chosenPath.c_str(), bytecode)) {
            chosenFormat = SDL_GPU_SHADERFORMAT_DXIL;
        }
    }

    if (chosenFormat == SDL_GPU_SHADERFORMAT_INVALID && (supportedFormats & SDL_GPU_SHADERFORMAT_SPIRV)) {
        chosenPath = std::string(basePath) + ".spv";
        if (LoadBinaryFile(chosenPath.c_str(), bytecode)) {
            chosenFormat = SDL_GPU_SHADERFORMAT_SPIRV;
        }
    }

    if (chosenFormat == SDL_GPU_SHADERFORMAT_INVALID || bytecode.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "ShaderLoader: No compatible shader bytecode found for %s (supported formats: 0x%x)",
                     basePath, supportedFormats);
        return nullptr;
    }

    // Map stage
    SDL_GPUShaderStage gpuStage;
    switch (stage) {
        case Stage::Vertex:   gpuStage = SDL_GPU_SHADERSTAGE_VERTEX; break;
        case Stage::Fragment: gpuStage = SDL_GPU_SHADERSTAGE_FRAGMENT; break;
        default:
            SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Unknown shader stage");
            return nullptr;
    }

    // Create the shader
    SDL_GPUShaderCreateInfo shaderInfo = {};
    shaderInfo.code = reinterpret_cast<const Uint8*>(bytecode.data());
    shaderInfo.code_size = bytecode.size();
    shaderInfo.entrypoint = entryPoint;
    shaderInfo.format = chosenFormat;
    shaderInfo.stage = gpuStage;
    shaderInfo.num_samplers = resources.numSamplers;
    shaderInfo.num_storage_textures = resources.numStorageTextures;
    shaderInfo.num_storage_buffers = resources.numStorageBuffers;
    shaderInfo.num_uniform_buffers = resources.numUniformBuffers;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderInfo);
    if (!shader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Failed to create GPU shader from %s: %s",
                     chosenPath.c_str(), SDL_GetError());
        return nullptr;
    }

    const char* formatName = (chosenFormat == SDL_GPU_SHADERFORMAT_SPIRV) ? "SPIRV" : "DXIL";
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Loaded %s shader from %s", formatName, chosenPath.c_str());
    return shader;
}

SDL_GPUGraphicsPipeline* ShaderLoader::LoadGraphicsPipeline(
    SDL_GPUDevice* device,
    SDL_GPUShader* vertexShader,
    SDL_GPUShader* fragmentShader,
    const SDL_GPUVertexInputState& vertexInputState,
    const SDL_GPUColorTargetDescription& colorTargetDesc,
    SDL_GPUTextureFormat depthStencilFormat,
    SDL_GPUCullMode cullMode,
    SDL_GPUFrontFace frontFace,
    bool enableDepthTest,
    bool enableDepthWrite)
{
    if (!device || !vertexShader || !fragmentShader) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Invalid parameters for pipeline creation");
        return nullptr;
    }

    SDL_GPURasterizerState rasterizerState = {};
    rasterizerState.fill_mode = SDL_GPU_FILLMODE_FILL;
    rasterizerState.cull_mode = cullMode;
    rasterizerState.front_face = frontFace;
    rasterizerState.depth_bias_constant_factor = 0.0f;
    rasterizerState.depth_bias_clamp = 0.0f;
    rasterizerState.depth_bias_slope_factor = 0.0f;
    rasterizerState.enable_depth_bias = false;
    rasterizerState.enable_depth_clip = true;

    SDL_GPUMultisampleState multisampleState = {};
    multisampleState.sample_count = SDL_GPU_SAMPLECOUNT_1;
    multisampleState.sample_mask = 0;
    multisampleState.enable_mask = false;

    SDL_GPUDepthStencilState depthStencilState = {};
    depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS;
    depthStencilState.back_stencil_state = {};
    depthStencilState.front_stencil_state = {};
    depthStencilState.compare_mask = 0xFF;
    depthStencilState.write_mask = 0xFF;
    depthStencilState.enable_depth_test = enableDepthTest;
    depthStencilState.enable_depth_write = enableDepthWrite;
    depthStencilState.enable_stencil_test = false;

    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
    targetInfo.color_target_descriptions = &colorTargetDesc;
    targetInfo.num_color_targets = 1;
    targetInfo.depth_stencil_format = depthStencilFormat;
    targetInfo.has_depth_stencil_target = (depthStencilFormat != SDL_GPU_TEXTUREFORMAT_INVALID);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state = rasterizerState;
    pipelineInfo.multisample_state = multisampleState;
    pipelineInfo.depth_stencil_state = depthStencilState;
    pipelineInfo.target_info = targetInfo;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!pipeline) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Failed to create graphics pipeline: %s",
                     SDL_GetError());
        return nullptr;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "ShaderLoader: Successfully created graphics pipeline");
    return pipeline;
}

} // namespace sims3000
