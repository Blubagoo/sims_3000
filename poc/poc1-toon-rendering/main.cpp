// POC-1: Toon Rendering
// Demonstrates instanced toon shading with 10000 buildings across multiple models
// Uses SDL3 main callbacks pattern for clean lifecycle management

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "src/gpu_device.h"
#include "src/camera.h"
#include "src/model_loader.h"
#include "src/gpu_mesh.h"
#include "src/instance_buffer.h"
#include "src/scene.h"
#include "src/toon_pipeline.h"
#include "src/benchmark.h"

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

// Window dimensions
static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;

// Instance count for the scene
static constexpr size_t BUILDING_COUNT = 10000;

// Frames before printing benchmark report
static constexpr int BENCHMARK_FRAME_COUNT = 100;

/**
 * Creates a procedural cube mesh as a fallback when no model files are available.
 */
static MeshData CreateCube() {
    MeshData data;

    data.vertices = {
        // Front face (z = 0.5)
        {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
        // Back face (z = -0.5)
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
        // Top face (y = 0.5)
        {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
        {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
        // Bottom face (y = -0.5)
        {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
        {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
        // Right face (x = 0.5)
        {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
        // Left face (x = -0.5)
        {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
        {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
    };

    data.indices = {
         0,  1,  2,   2,  3,  0,
         4,  5,  6,   6,  7,  4,
         8,  9, 10,  10, 11,  8,
        12, 13, 14,  14, 15, 12,
        16, 17, 18,  18, 19, 16,
        20, 21, 22,  22, 23, 20,
    };

    return data;
}

/**
 * Scan a directory for all .glb files and load them.
 * Returns a vector of MeshData (one per successfully loaded model).
 */
static std::vector<MeshData> LoadAllModels(const std::string& glbDir) {
    std::vector<MeshData> models;

    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::is_directory(glbDir, ec)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Model directory not found: %s", glbDir.c_str());
        return models;
    }

    for (const auto& entry : fs::directory_iterator(glbDir, ec)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        // Case-insensitive .glb check
        if (ext != ".glb" && ext != ".GLB") continue;

        MeshData mesh = LoadModel(entry.path().string().c_str());
        if (!mesh.vertices.empty()) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Loaded model: %s (%zu verts, %zu indices)",
                        entry.path().filename().string().c_str(),
                        mesh.vertices.size(), mesh.indices.size());
            models.push_back(std::move(mesh));
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Loaded %zu models from %s", models.size(), glbDir.c_str());
    return models;
}

/**
 * Application state containing all rendering resources.
 */
struct AppState {
    SDL_Window* window = nullptr;
    std::unique_ptr<GPUDevice> gpuDevice;
    std::unique_ptr<Camera> camera;
    std::vector<std::unique_ptr<GPUMesh>> meshes;
    std::unique_ptr<poc1::InstanceBuffer> instanceBuffer;
    std::unique_ptr<poc1::Scene> scene;
    std::unique_ptr<ToonPipeline> toonPipeline;
    std::unique_ptr<poc1::Benchmark> benchmark;

    // State tracking
    bool benchmarkPrinted = false;
    bool instanceBufferNeedsUpdate = true;
    int frameCount = 0;
};

/**
 * SDL_AppInit - Initialize the application.
 */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState* state = new (std::nothrow) AppState();
    if (!state) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate AppState");
        SDL_Quit();
        return SDL_APP_FAILURE;
    }
    *appstate = state;

    // Create window
    state->window = SDL_CreateWindow(
        "POC-1: Toon Rendering - 10000 Instanced Buildings",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!state->window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Failed to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create GPU device
    state->gpuDevice = std::make_unique<GPUDevice>(state->window);
    if (!state->gpuDevice->IsValid()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create GPU device");
        return SDL_APP_FAILURE;
    }

    if (!state->gpuDevice->ClaimWindow()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to claim window");
        return SDL_APP_FAILURE;
    }

    // Create camera with isometric view
    state->camera = std::make_unique<Camera>();
    state->camera->SetAspectRatio(static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT));

    // Isometric view of the city grid
    state->camera->SetIsometricView(glm::vec3(0.0f, 0.0f, 0.0f), 60.0f);
    state->camera->SetOrthoSize(15.0f);

    // Load all GLB models from Kenney commercial kit
    const char* basePath = SDL_GetBasePath();
    std::string baseStr = basePath ? basePath : "";

    // Try multiple possible directories
    std::vector<std::string> modelDirs = {
        baseStr + "assets/models/buildings/kenney_commercial/Models/GLB format",
        "assets/models/buildings/kenney_commercial/Models/GLB format",
    };

    std::vector<MeshData> allMeshData;
    for (const auto& dir : modelDirs) {
        allMeshData = LoadAllModels(dir);
        if (!allMeshData.empty()) break;
    }

    // Fallback to cube if no models found
    if (allMeshData.empty()) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "No model files found, using procedural cube");
        allMeshData.push_back(CreateCube());
    }

    // Create GPU meshes
    uint64_t gpuMemory = 0;
    for (auto& meshData : allMeshData) {
        auto gpuMesh = std::make_unique<GPUMesh>(state->gpuDevice->GetDevice(), meshData);
        if (gpuMesh->IsValid()) {
            gpuMemory += static_cast<uint64_t>(meshData.vertices.size()) * sizeof(Vertex);
            gpuMemory += static_cast<uint64_t>(meshData.indices.size()) * sizeof(uint32_t);
            state->meshes.push_back(std::move(gpuMesh));
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Created %zu GPU meshes", state->meshes.size());

    // Create scene with buildings assigned to random models
    state->scene = std::make_unique<poc1::Scene>(BUILDING_COUNT, state->meshes.size());
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Scene created with %zu buildings across %zu model groups",
                state->scene->GetInstanceCount(),
                state->scene->GetModelGroups().size());

    // Create instance buffer
    state->instanceBuffer = std::make_unique<poc1::InstanceBuffer>(
        state->gpuDevice->GetDevice(),
        state->scene->GetInstanceCount());

    if (!state->instanceBuffer->GetBuffer()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create instance buffer");
        return SDL_APP_FAILURE;
    }

    // Create toon pipeline
    state->toonPipeline = std::make_unique<ToonPipeline>(*state->gpuDevice);

    std::string shaderDir = baseStr + "shaders/";
    if (!state->toonPipeline->Initialize(shaderDir.c_str())) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize toon pipeline");
        return SDL_APP_FAILURE;
    }

    // Create benchmark
    state->benchmark = std::make_unique<poc1::Benchmark>();

    // Estimate GPU memory
    gpuMemory += static_cast<uint64_t>(state->scene->GetInstanceCount()) * sizeof(poc1::InstanceData) * 2;
    gpuMemory += static_cast<uint64_t>(WINDOW_WIDTH) * WINDOW_HEIGHT * 4;
    state->benchmark->SetGPUMemoryBytes(gpuMemory);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Initialization complete");
    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppEvent - Handle SDL events.
 */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    AppState* state = static_cast<AppState*>(appstate);

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;

        case SDL_EVENT_KEY_DOWN:
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            if (event->window.data1 > 0 && event->window.data2 > 0) {
                float aspect = static_cast<float>(event->window.data1) /
                               static_cast<float>(event->window.data2);
                state->camera->SetAspectRatio(aspect);
            }
            break;

        default:
            break;
    }

    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppIterate - Main render loop iteration.
 */
SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* state = static_cast<AppState*>(appstate);

    state->benchmark->StartFrame();
    state->benchmark->ResetDrawCalls();
    state->benchmark->SetInstanceCount(static_cast<uint32_t>(state->scene->GetInstanceCount()));

    SDL_GPUCommandBuffer* commandBuffer = state->gpuDevice->AcquireCommandBuffer();
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to acquire command buffer");
        return SDL_APP_FAILURE;
    }

    // Upload instance data on first frame
    if (state->instanceBufferNeedsUpdate) {
        state->instanceBuffer->Update(commandBuffer, state->scene->GetInstances());
        state->instanceBufferNeedsUpdate = false;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    SDL_AcquireGPUSwapchainTexture(commandBuffer, state->window, &swapchainTexture, nullptr, nullptr);
    if (!swapchainTexture) {
        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return SDL_APP_CONTINUE;
    }

    // Render using multi-mesh toon pipeline
    state->toonPipeline->SetCamera(*state->camera);
    state->toonPipeline->Render(commandBuffer, swapchainTexture,
                                 state->meshes, *state->instanceBuffer,
                                 state->scene->GetModelGroups(),
                                 *state->benchmark);

    state->gpuDevice->Submit(commandBuffer);

    state->benchmark->EndFrame();
    state->frameCount++;

    if (state->frameCount >= BENCHMARK_FRAME_COUNT && !state->benchmarkPrinted) {
        state->benchmark->PrintReport();
        state->benchmarkPrinted = true;
    }

    return SDL_APP_CONTINUE;
}

/**
 * SDL_AppQuit - Clean up application resources.
 */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    (void)result;

    if (!appstate) {
        SDL_Quit();
        return;
    }

    AppState* state = static_cast<AppState*>(appstate);

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Shutting down...");

    state->benchmark.reset();
    state->scene.reset();
    state->instanceBuffer.reset();
    state->meshes.clear();
    state->camera.reset();
    state->toonPipeline.reset();
    state->gpuDevice.reset();

    if (state->window) {
        SDL_DestroyWindow(state->window);
        state->window = nullptr;
    }

    delete state;
    SDL_Quit();
}
