/**
 * @file Application.cpp
 * @brief Application implementation.
 */

#include "sims3000/app/Application.h"
#include "sims3000/net/ENetTransport.h"
#include "sims3000/render/ViewMatrix.h"
#include "sims3000/render/ProjectionMatrix.h"
#include "sims3000/terrain/ElevationGenerator.h"
#include "sims3000/terrain/BiomeGenerator.h"
#include "sims3000/terrain/WaterDistanceField.h"
#include "sims3000/terrain/TerrainTypeInfo.h"
#include "sims3000/terrain/TerrainChunk.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <glm/gtc/type_ptr.hpp>
#include <chrono>
#include <thread>
#include <cmath>

namespace sims3000 {

Application::Application(const ApplicationConfig& config)
    : m_appConfig(config)
    , m_serverMode(config.serverMode)
{
    // Load user configuration
    m_config.load();

    // Initialize SDL
    Uint32 sdlFlags = SDL_INIT_EVENTS;
    if (!m_serverMode) {
        sdlFlags |= SDL_INIT_VIDEO;
    }

    if (!SDL_Init(sdlFlags)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return;
    }

    if (m_serverMode) {
        SDL_Log("Starting in SERVER mode on port %d", m_appConfig.serverPort);
    } else {
        // Client mode - create GPU device first
        m_gpuDevice = std::make_unique<GPUDevice>();

        if (!m_gpuDevice->isValid()) {
            SDL_Log("Failed to create GPU device: %s", m_gpuDevice->getLastError().c_str());
            return;
        }

        m_gpuDevice->logCapabilities();

        // Create window
        m_window = std::make_unique<Window>(
            m_appConfig.title,
            m_appConfig.windowWidth,
            m_appConfig.windowHeight
        );

        if (!m_window->isValid()) {
            SDL_Log("Failed to create window: %s", m_window->getLastError().c_str());
            return;
        }

        // Claim window for GPU device
        if (!m_window->claimForDevice(*m_gpuDevice)) {
            SDL_Log("Failed to claim window for GPU: %s", m_window->getLastError().c_str());
            return;
        }

        // Apply vsync setting from config
        PresentMode presentMode = m_config.graphics().vsync ?
            PresentMode::VSync : PresentMode::Immediate;
        m_window->setPresentMode(presentMode);

        if (m_appConfig.startFullscreen) {
            m_window->toggleFullscreen();
        }

        // Create input system (client only)
        m_input = std::make_unique<InputSystem>();

        // Create asset manager (client only)
        // Window is now claimed for GPU, so Window.getDevice() returns the GPU device
        m_assets = std::make_unique<AssetManager>(*m_window);

#ifdef SIMS3000_DEBUG
        m_assets->setHotReloadEnabled(true);
#endif

        // Create ToonPipeline for rendering (client only)
        // Note: Pipeline creation requires shaders - will be fully initialized
        // when MainRenderPass and shader loading are connected
        m_toonPipeline = std::make_unique<ToonPipeline>(*m_gpuDevice);

        // Initialize terrain rendering (Epic 3)
        if (!initTerrain()) {
            SDL_Log("Warning: Terrain rendering failed to initialize, falling back to demo");
            // Fallback to demo rendering if terrain fails
            if (!initDemo()) {
                SDL_Log("Warning: Demo rendering also failed to initialize");
            }
        }

        // Initialize zone/building demo (Epic 4)
        if (!initZoneBuilding()) {
            SDL_Log("Warning: Zone/building demo failed to initialize");
        }
    }

    // Create ECS (both client and server)
    m_registry = std::make_unique<Registry>();
    m_systems = std::make_unique<SystemManager>();

    // Create SyncSystem (both client and server)
    m_syncSystem = std::make_unique<SyncSystem>(*m_registry);
    m_syncSystem->subscribeAll();

#ifdef SIMS3000_DEBUG
    m_systems->setProfilingEnabled(true);
#endif

    // Initialize networking
    initializeNetworking();

    m_valid = true;
    SDL_Log("Application initialized (%s mode)", m_serverMode ? "server" : "client");
}

Application::~Application() {
    shutdown();
}

bool Application::isValid() const {
    return m_valid;
}

bool Application::isServerMode() const {
    return m_serverMode;
}

AppState Application::getState() const {
    return m_currentState;
}

void Application::requestStateChange(AppState newState) {
    if (newState != m_currentState) {
        m_pendingState = newState;
        m_stateChangeRequested = true;
    }
}

int Application::run() {
    if (!m_valid) {
        SDL_Log("Application not valid, cannot run");
        return 1;
    }

    m_running = true;
    m_lastFrameTime = SDL_GetPerformanceCounter();

    // Enter initial state
    transitionState(m_serverMode ? AppState::Playing : AppState::Menu);

    SDL_Log("Entering main loop");

    while (m_running) {
        // Calculate delta time
        std::uint64_t currentTime = SDL_GetPerformanceCounter();
        float deltaTime = static_cast<float>(currentTime - m_lastFrameTime) /
                          static_cast<float>(SDL_GetPerformanceFrequency());
        m_lastFrameTime = currentTime;

        // Cap delta time to prevent spiral of death
        if (deltaTime > 0.25f) {
            deltaTime = 0.25f;
        }

        // Handle state transitions
        if (m_stateChangeRequested) {
            transitionState(m_pendingState);
            m_stateChangeRequested = false;
        }

        // 1. Poll network and process incoming messages FIRST
        //    Data flow: receive -> process -> tick -> generate -> send
        processNetworkMessages();

        // Client-specific frame setup
        if (!m_serverMode) {
            m_input->beginFrame();
            m_frameStats.update(deltaTime);
            processEvents();
        }

        // 2. Apply pending state updates (client) before simulation
        if (!m_serverMode && m_currentState == AppState::Playing) {
            applyPendingStateUpdates();
        }

        // 3. Update simulation (both modes)
        updateSimulation();

        // 4. Generate and send deltas (server) after simulation
        if (m_serverMode && m_currentState == AppState::Playing) {
            generateAndSendDeltas();
        }

        // 5. Update demo camera and render (client only)
        if (!m_serverMode) {
            updateDemoCamera(deltaTime);
            render();
            m_assets->checkHotReload();
        } else {
            // Server mode - sleep to avoid CPU spin
            // Target ~60 ticks of headroom checking per second
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    SDL_Log("Exiting main loop");

    // Save config on exit
    m_config.save();

    return 0;
}

void Application::requestShutdown() {
    requestStateChange(AppState::Exiting);
    m_running = false;
}

const ISimulationTime& Application::getSimulationTime() const {
    return m_clock;
}

InputSystem& Application::getInput() {
    return *m_input;
}

Registry& Application::getRegistry() {
    return *m_registry;
}

SystemManager& Application::getSystems() {
    return *m_systems;
}

AssetManager& Application::getAssets() {
    return *m_assets;
}

const FrameStats& Application::getFrameStats() const {
    return m_frameStats;
}

Window& Application::getWindow() {
    return *m_window;
}

GPUDevice& Application::getGPUDevice() {
    return *m_gpuDevice;
}

ToonPipeline& Application::getToonPipeline() {
    return *m_toonPipeline;
}

bool Application::isWireframeEnabled() const {
    return m_toonPipeline && m_toonPipeline->isWireframeEnabled();
}

bool Application::toggleWireframe() {
    if (m_toonPipeline) {
        return m_toonPipeline->toggleWireframe();
    }
    return false;
}

Config& Application::getConfig() {
    return m_config;
}

const Config& Application::getConfig() const {
    return m_config;
}

void Application::transitionState(AppState newState) {
    if (newState == m_currentState) return;

    SDL_Log("State transition: %s -> %s",
            getAppStateName(m_currentState),
            getAppStateName(newState));

    onStateExit(m_currentState);
    m_currentState = newState;
    onStateEnter(newState);
}

void Application::onStateEnter(AppState state) {
    switch (state) {
        case AppState::Menu:
            // Pause simulation when in menu
            m_clock.setPaused(true);
            break;

        case AppState::Connecting:
            // Connection initiated via connectToServer() or auto-connect
            m_clock.setPaused(true);
            break;

        case AppState::Loading:
            // Loading assets or waiting for server to be ready
            m_clock.setPaused(true);
            break;

        case AppState::Playing:
            m_clock.setPaused(false);
            // Sync the clock to server tick if we have one
            if (!m_serverMode && m_networkClient) {
                const auto& serverStatus = m_networkClient->getServerStatus();
                // Reset sync system for fresh delta tracking
                m_syncSystem->resetLastProcessedTick(serverStatus.currentTick);
            }
            break;

        case AppState::Paused:
            m_clock.setPaused(true);
            break;

        case AppState::Exiting:
            m_running = false;
            break;
    }
}

void Application::onStateExit(AppState state) {
    switch (state) {
        case AppState::Connecting:
            // Connection completed or cancelled
            break;

        case AppState::Loading:
            // Finalize loaded assets
            break;

        case AppState::Playing:
            // Flush any pending sync data
            m_syncSystem->flush();
            break;

        default:
            break;
    }
}

void Application::processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Let input system handle first
        if (m_input->processEvent(event)) {
            continue;
        }

        switch (event.type) {
            case SDL_EVENT_QUIT:
                requestShutdown();
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                m_window->onResize(event.window.data1, event.window.data2);
                break;

            case SDL_EVENT_KEY_DOWN:
                // Handle global keys
                if (m_input->isActionPressed(Action::QUIT)) {
                    requestShutdown();
                }
                if (m_input->isActionPressed(Action::FULLSCREEN)) {
                    m_window->toggleFullscreen();
                }
                if (m_input->isActionPressed(Action::PAUSE)) {
                    if (m_currentState == AppState::Playing) {
                        requestStateChange(AppState::Paused);
                    } else if (m_currentState == AppState::Paused) {
                        requestStateChange(AppState::Playing);
                    }
                }
                // Debug: Toggle wireframe mode (F key)
                if (m_input->isActionPressed(Action::DEBUG_WIREFRAME)) {
                    toggleWireframe();
                }
                // Zone/Building demo input (Epic 4)
                handleZoneBuildingInput();
                break;

            default:
                break;
        }
    }
}

void Application::updateSimulation() {
    // Only run simulation when in Playing state
    if (m_currentState != AppState::Playing) {
        return;
    }

    // Calculate frame delta for this update
    std::uint64_t currentTime = SDL_GetPerformanceCounter();
    float deltaTime = static_cast<float>(currentTime - m_lastFrameTime) /
                      static_cast<float>(SDL_GetPerformanceFrequency());

    // Accumulate time and process ticks (fixed 50ms intervals, 20 ticks/sec)
    int tickCount = m_clock.accumulate(deltaTime);

    auto tickStart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < tickCount; ++i) {
        // Run all registered systems
        m_systems->tick(m_clock);

        // Tick zone/building systems (Epic 4 demo)
        tickZoneBuilding();

        // SyncSystem tick (captures dirty entities via signals, no extra work needed)
        m_syncSystem->tick(m_clock);

        // Advance the tick counter
        m_clock.advanceTick();

        // Server: Update network server's tick number for sync
        if (m_serverMode && m_networkServer) {
            m_networkServer->setCurrentTick(m_clock.getCurrentTick());
        }

        // Server: Generate and broadcast deltas after each tick
        if (m_serverMode && m_networkServer) {
            auto delta = m_syncSystem->generateDelta(m_clock.getCurrentTick());
            if (delta && delta->hasDeltas()) {
                m_networkServer->broadcastStateUpdate(*delta);
            }
            // Clear dirty set after sending
            m_syncSystem->flush();
        }
    }

    auto tickEnd = std::chrono::high_resolution_clock::now();
    float tickTimeMs = std::chrono::duration<float, std::milli>(tickEnd - tickStart).count();

    if (tickCount > 0) {
        m_frameStats.recordTickTime(tickTimeMs / static_cast<float>(tickCount));
    }
}

void Application::render() {
    if (!m_window || !m_gpuDevice) return;

    SDL_GPUCommandBuffer* cmdBuffer = m_gpuDevice->acquireCommandBuffer();
    if (!cmdBuffer) {
        return;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!m_window->acquireSwapchainTexture(cmdBuffer, &swapchainTexture)) {
        // Still need to submit the command buffer even if acquire fails
        m_gpuDevice->submit(cmdBuffer);
        return;
    }

    if (swapchainTexture) {
        // Render terrain if available, otherwise fall back to demo
        if (m_terrainInitialized) {
            renderTerrain(cmdBuffer, swapchainTexture);
        } else {
            renderDemo(cmdBuffer, swapchainTexture);
        }
        // Render zone/building overlay on top (Epic 4)
        if (m_zoneBuildingInitialized) {
            renderZoneBuildingOverlay(cmdBuffer, swapchainTexture);
        }
    } else {
        static int nullCount = 0;
        if (++nullCount % 60 == 1) {
            SDL_Log("render: null swapchain texture (count: %d)", nullCount);
        }
    }

    m_gpuDevice->submit(cmdBuffer);
}

void Application::shutdown() {
    if (!m_valid) return;

    SDL_Log("Shutting down...");

    // Shutdown networking first
    shutdownNetworking();

    // Clear systems first (they may reference assets)
    if (m_systems) {
        m_systems->clear();
    }

    // Clear SyncSystem
    m_syncSystem.reset();

    // Clear ECS
    if (m_registry) {
        m_registry->clear();
    }

    // Clear assets (client only)
    if (m_assets) {
        m_assets->clearAll();
    }

    // Cleanup zone/building resources
    cleanupZoneBuilding();

    // Cleanup terrain resources
    cleanupTerrain();

    // Cleanup demo resources
    cleanupDemo();

    // Destroy systems in reverse order of creation
    m_systems.reset();
    m_registry.reset();
    m_toonPipeline.reset();  // Pipeline before GPU device
    m_assets.reset();
    m_input.reset();
    m_window.reset();      // Window releases from GPU device on destruction
    m_gpuDevice.reset();   // GPU device destroyed after window

    SDL_Quit();

    m_valid = false;
    SDL_Log("Shutdown complete");
}

// =============================================================================
// Networking
// =============================================================================

void Application::initializeNetworking() {
    if (m_serverMode) {
        // Create server
        ServerConfig serverConfig;
        serverConfig.port = static_cast<std::uint16_t>(m_appConfig.serverPort);
        serverConfig.mapSize = m_appConfig.mapSize;
        serverConfig.maxPlayers = 4;
        serverConfig.tickRate = 20;
        serverConfig.serverName = "Sims 3000 Server";

        auto transport = std::make_unique<ENetTransport>();
        m_networkServer = std::make_unique<NetworkServer>(std::move(transport), serverConfig);

        if (m_networkServer->start()) {
            SDL_Log("Server started on port %d", m_appConfig.serverPort);
        } else {
            SDL_Log("Failed to start server on port %d", m_appConfig.serverPort);
        }
    } else {
        // Create client
        ConnectionConfig clientConfig;
        clientConfig.heartbeatIntervalMs = 1000;
        clientConfig.timeoutIndicatorMs = 2000;
        clientConfig.timeoutBannerMs = 5000;
        clientConfig.timeoutFullUIMs = 15000;

        auto transport = std::make_unique<ENetTransport>();
        m_networkClient = std::make_unique<NetworkClient>(std::move(transport), clientConfig);

        // Set up state change callback
        m_networkClient->setStateChangeCallback(
            [this](ConnectionState oldState, ConnectionState newState) {
                onClientStateChange(oldState, newState);
            }
        );

        // Auto-connect if address specified
        if (!m_appConfig.connectAddress.empty() && m_appConfig.connectPort > 0) {
            connectToServer(m_appConfig.connectAddress, m_appConfig.connectPort);
        }
    }
}

void Application::shutdownNetworking() {
    if (m_networkServer) {
        m_networkServer->stop();
        m_networkServer.reset();
        SDL_Log("Server stopped");
    }

    if (m_networkClient) {
        m_networkClient->disconnect();
        m_networkClient.reset();
        SDL_Log("Client disconnected");
    }
}

void Application::processNetworkMessages() {
    // Calculate delta time for network update
    std::uint64_t currentTime = SDL_GetPerformanceCounter();
    float deltaTime = static_cast<float>(currentTime - m_lastFrameTime) /
                      static_cast<float>(SDL_GetPerformanceFrequency());

    if (m_serverMode && m_networkServer) {
        // Server: poll network and process messages
        m_networkServer->update(deltaTime);
    } else if (m_networkClient) {
        // Client: poll network and process messages
        m_networkClient->update(deltaTime);
    }
}

void Application::applyPendingStateUpdates() {
    if (!m_networkClient || !m_syncSystem) {
        return;
    }

    // Apply all pending state updates before render
    StateUpdateMessage update;
    while (m_networkClient->pollStateUpdate(update)) {
        DeltaApplicationResult result = m_syncSystem->applyDelta(update);

        switch (result) {
            case DeltaApplicationResult::Applied:
                // Success - state is now synchronized with server tick
                break;

            case DeltaApplicationResult::Duplicate:
                // Already processed this tick - no action needed
                SDL_Log("Duplicate state update for tick %llu (ignored)",
                        static_cast<unsigned long long>(update.tick));
                break;

            case DeltaApplicationResult::OutOfOrder:
                // Received older tick - could happen during reconnection
                SDL_Log("Out-of-order state update for tick %llu (ignored)",
                        static_cast<unsigned long long>(update.tick));
                break;

            case DeltaApplicationResult::Error:
                SDL_Log("Error applying state update for tick %llu",
                        static_cast<unsigned long long>(update.tick));
                break;
        }
    }
}

void Application::generateAndSendDeltas() {
    // Note: Delta generation is now done inside updateSimulation() after each tick
    // This method is kept for potential future use (e.g., batching multiple ticks)
}

void Application::onClientStateChange(ConnectionState oldState, ConnectionState newState) {
    (void)oldState;

    switch (newState) {
        case ConnectionState::Disconnected:
            // Return to menu on disconnect
            requestStateChange(AppState::Menu);
            break;

        case ConnectionState::Connecting:
            // Show connecting state
            requestStateChange(AppState::Connecting);
            break;

        case ConnectionState::Connected:
            // Connected but waiting for join accept - still connecting
            break;

        case ConnectionState::Playing:
            // Fully connected and joined - transition to playing
            requestStateChange(AppState::Playing);
            break;

        case ConnectionState::Reconnecting:
            // Show connecting state during reconnection
            requestStateChange(AppState::Connecting);
            break;
    }
}

bool Application::connectToServer(const std::string& address, std::uint16_t port) {
    if (m_serverMode) {
        SDL_Log("Cannot connect to server in server mode");
        return false;
    }

    if (!m_networkClient) {
        SDL_Log("Network client not initialized");
        return false;
    }

    SDL_Log("Connecting to %s:%u...", address.c_str(), port);
    requestStateChange(AppState::Connecting);

    return m_networkClient->connect(address, port, m_appConfig.playerName);
}

void Application::disconnectFromServer() {
    if (m_networkClient) {
        m_networkClient->disconnect();
    }
    requestStateChange(AppState::Menu);
}

NetworkServer* Application::getNetworkServer() {
    return m_networkServer.get();
}

NetworkClient* Application::getNetworkClient() {
    return m_networkClient.get();
}

SyncSystem& Application::getSyncSystem() {
    return *m_syncSystem;
}

SimulationTick Application::getCurrentTick() const {
    return m_clock.getCurrentTick();
}

// =============================================================================
// Demo Rendering (for manual testing Epic 2)
// =============================================================================

bool Application::initDemo() {
    if (!m_gpuDevice || !m_gpuDevice->isValid()) {
        return false;
    }

    SDL_Log("Initializing demo rendering...");

    // Create shader compiler
    m_shaderCompiler = std::make_unique<ShaderCompiler>(*m_gpuDevice);
    m_shaderCompiler->setAssetPath(".");  // Load shaders relative to exe

    // Load debug_bbox shaders
    ShaderResources vertResources{};
    vertResources.numUniformBuffers = 1;  // viewProjection

    ShaderResources fragResources{};

    auto vertResult = m_shaderCompiler->loadShader(
        "shaders/debug_bbox.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        SDL_Log("Failed to load demo vertex shader: %s", vertResult.error.message.c_str());
        return false;
    }
    m_demoVertShader = vertResult.shader;

    auto fragResult = m_shaderCompiler->loadShader(
        "shaders/debug_bbox.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        SDL_Log("Failed to load demo fragment shader: %s", fragResult.error.message.c_str());
        return false;
    }
    m_demoFragShader = fragResult.shader;

    SDL_Log("Demo shaders loaded successfully");

    // Create graphics pipeline
    SDL_GPUDevice* device = m_gpuDevice->getHandle();

    // Vertex attributes: position (vec3) + color (vec4)
    SDL_GPUVertexBufferDescription vertexBufferDesc{};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(float) * 7;  // 3 floats pos + 4 floats color
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    // Position
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0;
    // Color
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].offset = sizeof(float) * 3;

    SDL_GPUVertexInputState vertexInputState{};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = m_demoVertShader;
    pipelineInfo.fragment_shader = m_demoFragShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;

    // Rasterizer state
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    m_demoPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!m_demoPipeline) {
        SDL_Log("Failed to create demo pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_Log("Demo pipeline created");

    // Create cube vertices (8 corners with colors)
    // Cube centered at origin, size 2x2x2
    struct DemoVertex {
        float x, y, z;      // position
        float r, g, b, a;   // color
    };

    // 8 corners of cube with distinct colors
    DemoVertex cubeVertices[8] = {
        // Front face (z = 1)
        {-1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f},  // 0: red
        { 1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f},  // 1: green
        { 1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 1.0f, 1.0f},  // 2: blue
        {-1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 1.0f},  // 3: yellow
        // Back face (z = -1)
        {-1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 1.0f, 1.0f},  // 4: magenta
        { 1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 1.0f, 1.0f},  // 5: cyan
        { 1.0f,  1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 1.0f},  // 6: white
        {-1.0f,  1.0f, -1.0f,  0.5f, 0.5f, 0.5f, 1.0f},  // 7: gray
    };

    // 12 edges (24 indices for LINE_LIST)
    uint16_t cubeIndices[24] = {
        // Front face
        0, 1,  1, 2,  2, 3,  3, 0,
        // Back face
        4, 5,  5, 6,  6, 7,  7, 4,
        // Connecting edges
        0, 4,  1, 5,  2, 6,  3, 7
    };

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vbInfo{};
    vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size = sizeof(cubeVertices);

    m_demoVertexBuffer = SDL_CreateGPUBuffer(device, &vbInfo);
    if (!m_demoVertexBuffer) {
        SDL_Log("Failed to create demo vertex buffer");
        return false;
    }

    // Create index buffer
    SDL_GPUBufferCreateInfo ibInfo{};
    ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size = sizeof(cubeIndices);

    m_demoIndexBuffer = SDL_CreateGPUBuffer(device, &ibInfo);
    if (!m_demoIndexBuffer) {
        SDL_Log("Failed to create demo index buffer");
        return false;
    }

    // Create uniform buffer for viewProjection matrix
    SDL_GPUBufferCreateInfo ubInfo{};
    ubInfo.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
    ubInfo.size = sizeof(glm::mat4);

    m_demoUniformBuffer = SDL_CreateGPUBuffer(device, &ubInfo);
    if (!m_demoUniformBuffer) {
        SDL_Log("Failed to create demo uniform buffer");
        return false;
    }

    // Upload vertex and index data via transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(cubeVertices) + sizeof(cubeIndices);

    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transferBuffer) {
        SDL_Log("Failed to create transfer buffer");
        return false;
    }

    // Map and copy data
    void* mapped = SDL_MapGPUTransferBuffer(device, transferBuffer, false);
    std::memcpy(mapped, cubeVertices, sizeof(cubeVertices));
    std::memcpy(static_cast<char*>(mapped) + sizeof(cubeVertices), cubeIndices, sizeof(cubeIndices));
    SDL_UnmapGPUTransferBuffer(device, transferBuffer);

    // Upload via copy pass
    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmd);

    SDL_GPUTransferBufferLocation srcVertex{};
    srcVertex.transfer_buffer = transferBuffer;
    srcVertex.offset = 0;

    SDL_GPUBufferRegion dstVertex{};
    dstVertex.buffer = m_demoVertexBuffer;
    dstVertex.offset = 0;
    dstVertex.size = sizeof(cubeVertices);

    SDL_UploadToGPUBuffer(copyPass, &srcVertex, &dstVertex, false);

    SDL_GPUTransferBufferLocation srcIndex{};
    srcIndex.transfer_buffer = transferBuffer;
    srcIndex.offset = sizeof(cubeVertices);

    SDL_GPUBufferRegion dstIndex{};
    dstIndex.buffer = m_demoIndexBuffer;
    dstIndex.offset = 0;
    dstIndex.size = sizeof(cubeIndices);

    SDL_UploadToGPUBuffer(copyPass, &srcIndex, &dstIndex, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCmd);

    SDL_ReleaseGPUTransferBuffer(device, transferBuffer);

    // Initialize camera state
    m_demoCamera.focus_point = glm::vec3(0.0f, 0.0f, 0.0f);
    m_demoCamera.distance = 8.0f;
    m_demoCamera.pitch = 35.0f;
    m_demoCamera.yaw = 45.0f;
    m_demoCamera.mode = CameraMode::Free;

    m_demoInitialized = true;
    SDL_Log("Demo rendering initialized - use WASD to pan, QE to rotate, +/- to zoom");

    return true;
}

void Application::updateDemoCamera(float deltaTime) {
    if ((!m_demoInitialized && !m_terrainInitialized) || !m_input) return;

    const float panSpeed = 100.0f * deltaTime;
    const float rotateSpeed = 180.0f * deltaTime;
    const float zoomSpeed = 100.0f * deltaTime;

    // Calculate camera forward and right vectors in world space
    float yawRad = m_demoCamera.yaw * 3.14159f / 180.0f;
    glm::vec3 forward(-sinf(yawRad), 0.0f, -cosf(yawRad));
    glm::vec3 right(cosf(yawRad), 0.0f, -sinf(yawRad));

    // Pan with WASD
    if (m_input->isActionDown(Action::PAN_UP)) {
        m_demoCamera.focus_point += forward * panSpeed;
    }
    if (m_input->isActionDown(Action::PAN_DOWN)) {
        m_demoCamera.focus_point -= forward * panSpeed;
    }
    if (m_input->isActionDown(Action::PAN_LEFT)) {
        m_demoCamera.focus_point -= right * panSpeed;
    }
    if (m_input->isActionDown(Action::PAN_RIGHT)) {
        m_demoCamera.focus_point += right * panSpeed;
    }

    // Rotate with Q/E
    if (m_input->isActionDown(Action::ROTATE_CCW)) {
        m_demoCamera.yaw -= rotateSpeed;
    }
    if (m_input->isActionDown(Action::ROTATE_CW)) {
        m_demoCamera.yaw += rotateSpeed;
    }

    // Zoom with +/- keys
    if (m_input->isActionDown(Action::ZOOM_IN)) {
        m_demoCamera.distance -= zoomSpeed;
    }
    if (m_input->isActionDown(Action::ZOOM_OUT)) {
        m_demoCamera.distance += zoomSpeed;
    }

    // Zoom with mouse scroll wheel
    const auto& mouse = m_input->getMouse();
    if (std::fabs(mouse.wheelY) > 0.001f) {
        m_demoCamera.distance -= mouse.wheelY * 10.0f;  // Scroll up = zoom in
    }

    // Apply constraints
    m_demoCamera.applyConstraints();
}

void Application::renderDemo(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain) {
    if (!m_demoInitialized) {
        // Just clear if demo not initialized
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchain;
        colorTarget.clear_color = {0.1f, 0.1f, 0.15f, 1.0f};
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);
        SDL_EndGPURenderPass(renderPass);
        return;
    }

    // Calculate camera position from CameraState (orbital camera)
    float pitchRad = glm::radians(m_demoCamera.pitch);
    float yawRad = glm::radians(m_demoCamera.yaw);

    // Camera orbits around focus_point at distance
    glm::vec3 offset;
    offset.x = m_demoCamera.distance * cos(pitchRad) * sin(yawRad);
    offset.y = m_demoCamera.distance * sin(pitchRad);
    offset.z = m_demoCamera.distance * cos(pitchRad) * cos(yawRad);

    glm::vec3 cameraPos = m_demoCamera.focus_point + offset;
    glm::vec3 target = m_demoCamera.focus_point;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPos, target, up);

    float aspectRatio = static_cast<float>(m_window->getWidth()) /
                        static_cast<float>(m_window->getHeight());
    glm::mat4 projection = glm::perspectiveRH_ZO(
        glm::radians(45.0f),
        aspectRatio,
        0.1f,
        100.0f
    );

    glm::mat4 viewProjection = projection * view;

    // Begin render pass with clear
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = swapchain;
    colorTarget.clear_color = {0.02f, 0.02f, 0.05f, 1.0f};  // Dark bioluminescent
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, nullptr);

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_demoPipeline);

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBinding{};
    vertexBinding.buffer = m_demoVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    // Bind index buffer
    SDL_GPUBufferBinding indexBinding{};
    indexBinding.buffer = m_demoIndexBuffer;
    indexBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Push uniform data (viewProjection matrix)
    SDL_PushGPUVertexUniformData(cmdBuffer, 0, glm::value_ptr(viewProjection), sizeof(glm::mat4));

    // Draw the cube (24 indices = 12 edges)
    SDL_DrawGPUIndexedPrimitives(renderPass, 24, 1, 0, 0, 0);

    SDL_EndGPURenderPass(renderPass);
}

void Application::cleanupDemo() {
    if (!m_gpuDevice) return;

    SDL_GPUDevice* device = m_gpuDevice->getHandle();
    if (!device) return;

    // Wait for GPU to finish before releasing resources
    SDL_WaitForGPUIdle(device);

    if (m_demoPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_demoPipeline);
        m_demoPipeline = nullptr;
    }

    if (m_demoVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, m_demoVertexBuffer);
        m_demoVertexBuffer = nullptr;
    }

    if (m_demoIndexBuffer) {
        SDL_ReleaseGPUBuffer(device, m_demoIndexBuffer);
        m_demoIndexBuffer = nullptr;
    }

    if (m_demoUniformBuffer) {
        SDL_ReleaseGPUBuffer(device, m_demoUniformBuffer);
        m_demoUniformBuffer = nullptr;
    }

    if (m_demoVertShader) {
        SDL_ReleaseGPUShader(device, m_demoVertShader);
        m_demoVertShader = nullptr;
    }

    if (m_demoFragShader) {
        SDL_ReleaseGPUShader(device, m_demoFragShader);
        m_demoFragShader = nullptr;
    }

    m_shaderCompiler.reset();
    m_demoInitialized = false;
}

// =============================================================================
// Terrain Rendering (Epic 3)
// =============================================================================

bool Application::initTerrain() {
    if (!m_gpuDevice || !m_gpuDevice->isValid()) {
        return false;
    }

    SDL_Log("Initializing terrain rendering...");

    // Create shader compiler if not already created
    if (!m_shaderCompiler) {
        m_shaderCompiler = std::make_unique<ShaderCompiler>(*m_gpuDevice);
        m_shaderCompiler->setAssetPath(".");
    }

    // Load terrain shaders
    ShaderResources vertResources{};
    vertResources.numUniformBuffers = 1;  // ViewProjection + LightViewProjection

    ShaderResources fragResources{};
    fragResources.numUniformBuffers = 2;  // Lighting + TerrainVisuals
    fragResources.numSamplers = 1;        // Shadow map + sampler (combined in SDL_GPU)

    auto vertResult = m_shaderCompiler->loadShader(
        "shaders/terrain.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        SDL_Log("Failed to load terrain vertex shader: %s", vertResult.error.message.c_str());
        return false;
    }
    m_terrainVertShader = vertResult.shader;

    auto fragResult = m_shaderCompiler->loadShader(
        "shaders/terrain.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        SDL_Log("Failed to load terrain fragment shader: %s", fragResult.error.message.c_str());
        return false;
    }
    m_terrainFragShader = fragResult.shader;

    SDL_Log("Terrain shaders loaded successfully");

    // Create terrain graphics pipeline
    SDL_GPUDevice* device = m_gpuDevice->getHandle();

    // Get terrain vertex attributes
    SDL_GPUVertexBufferDescription vertexBufferDesc = terrain::getTerrainVertexBufferDescription(0);

    SDL_GPUVertexAttribute vertexAttributes[terrain::TERRAIN_VERTEX_ATTRIBUTE_COUNT];
    std::uint32_t attrCount = 0;
    terrain::getTerrainVertexAttributes(0, vertexAttributes, &attrCount);

    SDL_GPUVertexInputState vertexInputState{};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = attrCount;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;

    // Depth-stencil state
    SDL_GPUDepthStencilState depthState{};
    depthState.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    depthState.enable_depth_test = true;
    depthState.enable_depth_write = true;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = m_terrainVertShader;
    pipelineInfo.fragment_shader = m_terrainFragShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    pipelineInfo.target_info.has_depth_stencil_target = true;
    pipelineInfo.depth_stencil_state = depthState;

    // Rasterizer state
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;  // DEBUG: disabled to test winding order
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    m_terrainPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!m_terrainPipeline) {
        SDL_Log("Failed to create terrain pipeline: %s", SDL_GetError());
        return false;
    }

    SDL_Log("Terrain pipeline created");

    // Initialize terrain grid with procedural generation
    using namespace terrain;

    // Use medium map size (256x256)
    m_terrainGrid.initialize(MapSize::Medium);

    // Generate elevation using a random seed based on time
    std::uint64_t seed = static_cast<std::uint64_t>(SDL_GetTicks());
    SDL_Log("Generating terrain with seed: %llu", static_cast<unsigned long long>(seed));

    ElevationConfig elevConfig = ElevationConfig::defaultConfig();
    ElevationResult elevResult = ElevationGenerator::generate(m_terrainGrid, seed, elevConfig);

    SDL_Log("Elevation generated: min=%u, max=%u, ridges=%u tiles, time=%.2fms",
            elevResult.minElevation, elevResult.maxElevation,
            elevResult.ridgeTileCount, elevResult.generationTimeMs);

    // Compute water distance field (required for biome generation)
    WaterDistanceField waterDist(MapSize::Medium);
    waterDist.compute(m_terrainGrid);

    // Generate biomes
    BiomeConfig biomeConfig;
    BiomeResult biomeResult = BiomeGenerator::generate(m_terrainGrid, waterDist, seed + 1, biomeConfig);

    SDL_Log("Biomes generated: grove=%u, prisma=%u, spore=%u, mire=%u, ember=%u, time=%.2fms",
            biomeResult.groveCount, biomeResult.prismaCount,
            biomeResult.sporeCount, biomeResult.mireCount,
            biomeResult.emberCount, biomeResult.generationTimeMs);

    // Initialize mesh generator
    m_terrainMeshGenerator.initialize(m_terrainGrid.width, m_terrainGrid.height);

    // Calculate number of chunks
    std::uint16_t chunksX = m_terrainGrid.width / CHUNK_SIZE;
    std::uint16_t chunksY = m_terrainGrid.height / CHUNK_SIZE;
    std::uint32_t totalChunks = static_cast<std::uint32_t>(chunksX) * chunksY;

    SDL_Log("Creating %u terrain chunks (%ux%u)", totalChunks, chunksX, chunksY);

    // Create chunks
    m_terrainChunks.clear();
    m_terrainChunks.reserve(totalChunks);
    for (std::uint16_t cy = 0; cy < chunksY; ++cy) {
        for (std::uint16_t cx = 0; cx < chunksX; ++cx) {
            m_terrainChunks.emplace_back(cx, cy);
        }
    }

    // Build all chunk meshes
    if (!m_terrainMeshGenerator.buildAllChunks(device, m_terrainGrid, m_terrainChunks)) {
        SDL_Log("Failed to build terrain chunk meshes");
        return false;
    }

    // Wait for GPU uploads to complete before rendering
    // Fixes race condition where first frames render before chunk data is uploaded
    SDL_WaitForGPUIdle(device);

    // Initialize camera to look at terrain center
    float centerX = static_cast<float>(m_terrainGrid.width) / 2.0f;
    float centerZ = static_cast<float>(m_terrainGrid.height) / 2.0f;
    m_demoCamera.focus_point = glm::vec3(centerX, 0.0f, centerZ);
    m_demoCamera.distance = 100.0f;  // Zoom out to see more terrain
    m_demoCamera.pitch = 45.0f;      // Look down at terrain
    m_demoCamera.yaw = 45.0f;
    m_demoCamera.mode = CameraMode::Free;

    m_terrainInitialized = true;
    SDL_Log("Terrain rendering initialized - %u chunks, %zu tiles",
            totalChunks, m_terrainGrid.tile_count());
    SDL_Log("Use WASD to pan, QE to rotate, +/- to zoom");

    return true;
}

void Application::renderTerrain(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain) {
    if (!m_terrainInitialized) {
        SDL_Log("renderTerrain: not initialized");
        return;
    }
    if (!swapchain) {
        SDL_Log("renderTerrain: null swapchain");
        return;
    }
    if (!m_terrainPipeline) {
        SDL_Log("renderTerrain: null pipeline");
        return;
    }

    SDL_GPUDevice* device = m_gpuDevice->getHandle();

    // Calculate camera matrices (same as demo camera)
    float pitchRad = glm::radians(m_demoCamera.pitch);
    float yawRad = glm::radians(m_demoCamera.yaw);

    glm::vec3 offset;
    offset.x = m_demoCamera.distance * cos(pitchRad) * sin(yawRad);
    offset.y = m_demoCamera.distance * sin(pitchRad);
    offset.z = m_demoCamera.distance * cos(pitchRad) * cos(yawRad);

    glm::vec3 cameraPos = m_demoCamera.focus_point + offset;
    glm::vec3 target = m_demoCamera.focus_point;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPos, target, up);


    float aspectRatio = static_cast<float>(m_window->getWidth()) /
                        static_cast<float>(m_window->getHeight());
    glm::mat4 projection = glm::perspectiveRH_ZO(
        glm::radians(45.0f),
        aspectRatio,
        0.1f,
        1000.0f  // Larger far plane for terrain
    );

    // ViewProjection uniform data
    struct ViewProjectionUBO {
        glm::mat4 viewProjection;
        glm::mat4 lightViewProjection;
    };

    ViewProjectionUBO vpUbo;
    vpUbo.viewProjection = projection * view;
    // Light from above-front for good terrain lighting
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.5f));
    glm::mat4 lightView = glm::lookAt(
        target + lightDir * 200.0f,
        target,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    float mapSize = static_cast<float>(m_terrainGrid.width);
    glm::mat4 lightProj = glm::orthoRH_ZO(-mapSize/2, mapSize/2, -mapSize/2, mapSize/2, 0.1f, 500.0f);
    vpUbo.lightViewProjection = lightProj * lightView;

    // Lighting uniform data
    struct LightingUBO {
        glm::vec3 sunDirection;
        float globalAmbient;
        glm::vec3 ambientColor;
        float shadowEnabled;
        glm::vec3 deepShadowColor;
        float shadowIntensity;
        glm::vec3 shadowTintColor;
        float shadowBias;
        glm::vec3 shadowColor;
        float shadowSoftness;
    };

    LightingUBO lightUbo;
    lightUbo.sunDirection = lightDir;
    lightUbo.globalAmbient = 0.3f;
    lightUbo.ambientColor = glm::vec3(0.2f, 0.15f, 0.25f);  // Purple-ish ambient
    lightUbo.shadowEnabled = 0.0f;  // No shadow map yet
    lightUbo.deepShadowColor = glm::vec3(0.05f, 0.02f, 0.1f);
    lightUbo.shadowIntensity = 0.5f;
    lightUbo.shadowTintColor = glm::vec3(0.1f, 0.15f, 0.2f);
    lightUbo.shadowBias = 0.001f;
    lightUbo.shadowColor = glm::vec3(0.1f, 0.1f, 0.15f);
    lightUbo.shadowSoftness = 0.5f;

    // Terrain visuals uniform data (palette)
    struct TerrainVisualsUBO {
        glm::vec4 base_colors[10];
        glm::vec4 emissive_colors[10];
        float glow_time;
        float sea_level;
        float _padding[2];
    };

    TerrainVisualsUBO terrainUbo;

    // Fill palette from TerrainTypeInfo
    for (int i = 0; i < 10; ++i) {
        const auto& info = terrain::getTerrainInfo(static_cast<terrain::TerrainType>(i));
        // Use emissive_color as base color (terrain has no separate base color in this version)
        terrainUbo.base_colors[i] = glm::vec4(
            info.emissive_color.x,
            info.emissive_color.y,
            info.emissive_color.z,
            1.0f
        );
        terrainUbo.emissive_colors[i] = glm::vec4(
            info.emissive_color.x,
            info.emissive_color.y,
            info.emissive_color.z,
            info.emissive_intensity
        );
    }

    // Animate glow
    static float glowTime = 0.0f;
    glowTime += 0.016f;  // ~60fps
    terrainUbo.glow_time = glowTime;
    terrainUbo.sea_level = static_cast<float>(m_terrainGrid.sea_level);
    terrainUbo._padding[0] = 0.0f;
    terrainUbo._padding[1] = 0.0f;

    // Create depth texture for this frame
    SDL_GPUTextureCreateInfo depthTextureInfo{};
    depthTextureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureInfo.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    depthTextureInfo.width = m_window->getWidth();
    depthTextureInfo.height = m_window->getHeight();
    depthTextureInfo.layer_count_or_depth = 1;
    depthTextureInfo.num_levels = 1;
    depthTextureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;  // Required for valid texture
    depthTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    SDL_GPUTexture* depthTexture = SDL_CreateGPUTexture(device, &depthTextureInfo);
    if (!depthTexture) {
        SDL_Log("Failed to create depth texture: %s", SDL_GetError());
        return;
    }

    // Begin render pass with depth buffer
    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchain;
    colorTarget.clear_color = {0.02f, 0.02f, 0.05f, 1.0f};  // Dark bioluminescent
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget{};
    depthTarget.texture = depthTexture;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTarget, 1, &depthTarget);

    // Bind terrain pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, m_terrainPipeline);

    // Create dummy shadow map texture (1x1 depth texture)
    SDL_GPUTextureCreateInfo dummyShadowInfo{};
    dummyShadowInfo.type = SDL_GPU_TEXTURETYPE_2D;
    dummyShadowInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    dummyShadowInfo.width = 1;
    dummyShadowInfo.height = 1;
    dummyShadowInfo.layer_count_or_depth = 1;
    dummyShadowInfo.num_levels = 1;
    dummyShadowInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    dummyShadowInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* dummyShadowMap = SDL_CreateGPUTexture(device, &dummyShadowInfo);

    // Create shadow sampler (comparison sampler for shadow mapping)
    SDL_GPUSamplerCreateInfo shadowSamplerInfo{};
    shadowSamplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    shadowSamplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    shadowSamplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    shadowSamplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    shadowSamplerInfo.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    shadowSamplerInfo.enable_compare = true;

    SDL_GPUSampler* shadowSampler = SDL_CreateGPUSampler(device, &shadowSamplerInfo);

    // Bind shadow map texture and sampler (slot 0)
    SDL_GPUTextureSamplerBinding shadowBinding{};
    shadowBinding.texture = dummyShadowMap;
    shadowBinding.sampler = shadowSampler;
    SDL_BindGPUFragmentSamplers(renderPass, 0, &shadowBinding, 1);

    // Push uniform data
    SDL_PushGPUVertexUniformData(cmdBuffer, 0, &vpUbo, sizeof(ViewProjectionUBO));
    SDL_PushGPUFragmentUniformData(cmdBuffer, 0, &lightUbo, sizeof(LightingUBO));
    SDL_PushGPUFragmentUniformData(cmdBuffer, 1, &terrainUbo, sizeof(TerrainVisualsUBO));

    // Render each chunk
    static bool loggedOnce = false;
    int renderedChunks = 0;
    int skippedChunks = 0;
    for (const auto& chunk : m_terrainChunks) {
        if (!chunk.hasGPUResources() || chunk.index_count == 0) {
            skippedChunks++;
            continue;
        }
        renderedChunks++;

        // Bind vertex buffer
        SDL_GPUBufferBinding vertexBinding{};
        vertexBinding.buffer = chunk.vertex_buffer;
        vertexBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

        // Bind index buffer
        SDL_GPUBufferBinding indexBinding{};
        indexBinding.buffer = chunk.index_buffer;
        indexBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // Draw chunk
        SDL_DrawGPUIndexedPrimitives(renderPass, chunk.index_count, 1, 0, 0, 0);
    }

    if (!loggedOnce) {
        SDL_Log("Terrain render: %d chunks rendered, %d skipped", renderedChunks, skippedChunks);
        loggedOnce = true;
    }

    SDL_EndGPURenderPass(renderPass);

    // Release depth texture
    SDL_ReleaseGPUTexture(device, depthTexture);

    // Release dummy shadow resources
    SDL_ReleaseGPUSampler(device, shadowSampler);
    SDL_ReleaseGPUTexture(device, dummyShadowMap);
}

void Application::cleanupTerrain() {
    if (!m_gpuDevice) return;

    SDL_GPUDevice* device = m_gpuDevice->getHandle();
    if (!device) return;

    // Wait for GPU to finish
    SDL_WaitForGPUIdle(device);

    // Release chunk GPU resources
    for (auto& chunk : m_terrainChunks) {
        chunk.releaseGPUResources(device);
    }
    m_terrainChunks.clear();

    // Release pipeline
    if (m_terrainPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_terrainPipeline);
        m_terrainPipeline = nullptr;
    }

    // Release shaders
    if (m_terrainVertShader) {
        SDL_ReleaseGPUShader(device, m_terrainVertShader);
        m_terrainVertShader = nullptr;
    }

    if (m_terrainFragShader) {
        SDL_ReleaseGPUShader(device, m_terrainFragShader);
        m_terrainFragShader = nullptr;
    }

    m_terrainInitialized = false;
}

// =============================================================================
// Zone/Building Demo Integration (Epic 4)
// =============================================================================

bool Application::initZoneBuilding() {
    if (!m_gpuDevice || !m_gpuDevice->isValid()) {
        return false;
    }

    SDL_Log("Initializing zone/building demo...");

    // Create ZoneSystem (nullptr terrain is OK - validation is permissive)
    m_zoneSystem = std::make_unique<zone::ZoneSystem>(
        nullptr,           // no terrain queryable
        &m_stubTransport,  // stub transport (always accessible)
        256                // match terrain grid size
    );

    // Create BuildingSystem
    m_buildingSystem = std::make_unique<building::BuildingSystem>(
        m_zoneSystem.get(),
        nullptr,  // no terrain queryable
        256       // match terrain grid size
    );

    // Wire stub providers
    m_buildingSystem->set_energy_provider(&m_stubEnergy);
    m_buildingSystem->set_fluid_provider(&m_stubFluid);
    m_buildingSystem->set_transport_provider(&m_stubTransport);
    m_buildingSystem->set_land_value_provider(&m_stubLandValue);
    m_buildingSystem->set_demand_provider(&m_stubDemand);
    m_buildingSystem->set_credit_provider(&m_stubCredits);

    // Create shader compiler if not already created
    if (!m_shaderCompiler) {
        m_shaderCompiler = std::make_unique<ShaderCompiler>(*m_gpuDevice);
        m_shaderCompiler->setAssetPath(".");
    }

    // Load debug_bbox shaders for overlay rendering
    ShaderResources vertResources{};
    vertResources.numUniformBuffers = 1;  // viewProjection

    ShaderResources fragResources{};

    auto vertResult = m_shaderCompiler->loadShader(
        "shaders/debug_bbox.vert",
        ShaderStage::Vertex,
        "main",
        vertResources
    );

    if (!vertResult.isValid()) {
        SDL_Log("Failed to load overlay vertex shader: %s", vertResult.error.message.c_str());
        return false;
    }
    m_overlayVertShader = vertResult.shader;

    auto fragResult = m_shaderCompiler->loadShader(
        "shaders/debug_bbox.frag",
        ShaderStage::Fragment,
        "main",
        fragResources
    );

    if (!fragResult.isValid()) {
        SDL_Log("Failed to load overlay fragment shader: %s", fragResult.error.message.c_str());
        return false;
    }
    m_overlayFragShader = fragResult.shader;

    // Create overlay pipeline (LINELIST, no depth test, draws on top)
    SDL_GPUDevice* device = m_gpuDevice->getHandle();

    // Vertex attributes: position (vec3) + color (vec4)
    SDL_GPUVertexBufferDescription vertexBufferDesc{};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.pitch = sizeof(float) * 7;
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;

    SDL_GPUVertexAttribute vertexAttributes[2] = {};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].offset = sizeof(float) * 3;

    SDL_GPUVertexInputState vertexInputState{};
    vertexInputState.vertex_buffer_descriptions = &vertexBufferDesc;
    vertexInputState.num_vertex_buffers = 1;
    vertexInputState.vertex_attributes = vertexAttributes;
    vertexInputState.num_vertex_attributes = 2;

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    // Enable alpha blending for overlay
    colorTarget.blend_state.enable_blend = true;
    colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = m_overlayVertShader;
    pipelineInfo.fragment_shader = m_overlayFragShader;
    pipelineInfo.vertex_input_state = vertexInputState;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;

    // No depth testing - overlay draws on top
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    m_overlayPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineInfo);
    if (!m_overlayPipeline) {
        SDL_Log("Failed to create overlay pipeline: %s", SDL_GetError());
        return false;
    }

    // Create GPU vertex buffer (pre-allocated)
    SDL_GPUBufferCreateInfo vbInfo{};
    vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size = MAX_OVERLAY_VERTICES * sizeof(float) * 7;

    m_overlayVertexBuffer = SDL_CreateGPUBuffer(device, &vbInfo);
    if (!m_overlayVertexBuffer) {
        SDL_Log("Failed to create overlay vertex buffer");
        return false;
    }

    // Create transfer buffer for per-frame updates
    SDL_GPUTransferBufferCreateInfo tbInfo{};
    tbInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size = MAX_OVERLAY_VERTICES * sizeof(float) * 7;

    m_overlayTransferBuffer = SDL_CreateGPUTransferBuffer(device, &tbInfo);
    if (!m_overlayTransferBuffer) {
        SDL_Log("Failed to create overlay transfer buffer");
        return false;
    }

    m_zoneBuildingInitialized = true;
    SDL_Log("Zone/building demo initialized");
    SDL_Log("  4=Habitation  5=Exchange  6=Fabrication  Z=Place  X=Demolish");

    return true;
}

void Application::tickZoneBuilding() {
    if (!m_zoneBuildingInitialized || !m_zoneSystem || !m_buildingSystem) return;

    m_zoneBuildingTickCounter++;
    m_zoneSystem->tick(0.05f);        // 50ms per tick at 20Hz
    m_buildingSystem->tick(0.05f);
}

void Application::handleZoneBuildingInput() {
    if (!m_zoneBuildingInitialized || !m_input || !m_zoneSystem) return;

    // Zone mode selection
    if (m_input->isActionPressed(Action::ZONE_HABITATION)) {
        m_zoneMode = (m_zoneMode == 1) ? 0 : 1;
        SDL_Log("Zone mode: %s", m_zoneMode == 1 ? "HABITATION" : "NONE");
    }
    if (m_input->isActionPressed(Action::ZONE_EXCHANGE)) {
        m_zoneMode = (m_zoneMode == 2) ? 0 : 2;
        SDL_Log("Zone mode: %s", m_zoneMode == 2 ? "EXCHANGE" : "NONE");
    }
    if (m_input->isActionPressed(Action::ZONE_FABRICATION)) {
        m_zoneMode = (m_zoneMode == 3) ? 0 : 3;
        SDL_Log("Zone mode: %s", m_zoneMode == 3 ? "FABRICATION" : "NONE");
    }

    // Place zone at camera focus
    if (m_input->isActionPressed(Action::ZONE_PLACE) && m_zoneMode > 0) {
        int32_t cx = static_cast<int32_t>(m_demoCamera.focus_point.x);
        int32_t cy = static_cast<int32_t>(m_demoCamera.focus_point.z);

        zone::ZoneType type = static_cast<zone::ZoneType>(m_zoneMode - 1);
        zone::ZonePlacementRequest request{};
        request.x = cx - 2;
        request.y = cy - 2;
        request.width = 5;
        request.height = 5;
        request.zone_type = type;
        request.density = zone::ZoneDensity::LowDensity;
        request.player_id = 0;

        auto result = m_zoneSystem->place_zones(request);
        SDL_Log("Placed %u zones at (%d,%d), skipped %u",
                result.placed_count, cx, cy, result.skipped_count);
    }

    // Demolish zone at camera focus
    if (m_input->isActionPressed(Action::ZONE_DEMOLISH)) {
        int32_t cx = static_cast<int32_t>(m_demoCamera.focus_point.x);
        int32_t cy = static_cast<int32_t>(m_demoCamera.focus_point.z);

        auto result = m_zoneSystem->remove_zones(cx - 2, cy - 2, 5, 5, 0);
        SDL_Log("Removed %u zones at (%d,%d), demolition requested: %u",
                result.removed_count, cx, cy, result.demolition_requested_count);
    }
}

void Application::renderZoneBuildingOverlay(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain) {
    if (!m_zoneBuildingInitialized || !m_zoneSystem) return;

    SDL_GPUDevice* device = m_gpuDevice->getHandle();

    // Build overlay vertex data
    // Vertex format: x, y, z, r, g, b, a (7 floats)
    float* mapped = static_cast<float*>(
        SDL_MapGPUTransferBuffer(device, m_overlayTransferBuffer, true)
    );
    if (!mapped) return;

    uint32_t vertexIndex = 0;
    const float ELEV_SCALE = terrain::ELEVATION_HEIGHT;
    const float ZONE_LIFT = 0.15f;  // Lift zone lines above terrain

    // Helper lambda to emit a line segment
    auto emitLine = [&](float x0, float y0, float z0,
                        float x1, float y1, float z1,
                        float r, float g, float b, float a) {
        if (vertexIndex + 2 > MAX_OVERLAY_VERTICES) return;
        uint32_t base = vertexIndex * 7;
        mapped[base + 0] = x0; mapped[base + 1] = y0; mapped[base + 2] = z0;
        mapped[base + 3] = r;  mapped[base + 4] = g;  mapped[base + 5] = b; mapped[base + 6] = a;
        base += 7;
        mapped[base + 0] = x1; mapped[base + 1] = y1; mapped[base + 2] = z1;
        mapped[base + 3] = r;  mapped[base + 4] = g;  mapped[base + 5] = b; mapped[base + 6] = a;
        vertexIndex += 2;
    };

    // Get elevation at grid position (clamped)
    auto getElevY = [&](int32_t x, int32_t z) -> float {
        if (m_terrainInitialized) {
            int32_t sx = std::max<int32_t>(0, std::min<int32_t>(x, m_terrainGrid.width - 1));
            int32_t sz = std::max<int32_t>(0, std::min<int32_t>(z, m_terrainGrid.height - 1));
            return static_cast<float>(m_terrainGrid.at(sx, sz).getElevation()) * ELEV_SCALE + ZONE_LIFT;
        }
        return ZONE_LIFT;
    };

    // Zone overlay colors
    const float zoneColors[3][4] = {
        {0.2f, 0.9f, 0.3f, 0.9f},  // Habitation = green
        {0.3f, 0.5f, 0.95f, 0.9f}, // Exchange = blue
        {0.95f, 0.55f, 0.1f, 0.9f} // Fabrication = orange
    };

    // Render zone cells as colored rectangles
    const auto& zoneGrid = m_zoneSystem->get_grid();
    uint16_t gridW = zoneGrid.getWidth();
    uint16_t gridH = zoneGrid.getHeight();

    // Only render zones near the camera focus (performance)
    int32_t focusX = static_cast<int32_t>(m_demoCamera.focus_point.x);
    int32_t focusZ = static_cast<int32_t>(m_demoCamera.focus_point.z);
    int32_t viewRadius = static_cast<int32_t>(m_demoCamera.distance * 0.8f);

    int32_t minX = std::max<int32_t>(0, focusX - viewRadius);
    int32_t maxX = std::min<int32_t>(gridW, focusX + viewRadius);
    int32_t minZ = std::max<int32_t>(0, focusZ - viewRadius);
    int32_t maxZ = std::min<int32_t>(gridH, focusZ + viewRadius);

    for (int32_t z = minZ; z < maxZ; ++z) {
        for (int32_t x = minX; x < maxX; ++x) {
            if (!zoneGrid.has_zone_at(x, z)) continue;

            zone::ZoneType type;
            if (!m_zoneSystem->get_zone_type(x, z, type)) continue;

            uint8_t typeIdx = static_cast<uint8_t>(type);
            if (typeIdx >= 3) continue;

            float r = zoneColors[typeIdx][0];
            float g = zoneColors[typeIdx][1];
            float b = zoneColors[typeIdx][2];
            float a = zoneColors[typeIdx][3];

            float fx = static_cast<float>(x);
            float fz = static_cast<float>(z);
            float y = getElevY(x, z);

            // Draw zone cell rectangle (4 line segments)
            emitLine(fx, y, fz, fx + 1.0f, y, fz, r, g, b, a);
            emitLine(fx + 1.0f, y, fz, fx + 1.0f, y, fz + 1.0f, r, g, b, a);
            emitLine(fx + 1.0f, y, fz + 1.0f, fx, y, fz + 1.0f, r, g, b, a);
            emitLine(fx, y, fz + 1.0f, fx, y, fz, r, g, b, a);
        }
    }

    // Building wireframe colors by state
    const float buildingColors[5][4] = {
        {0.0f, 0.8f, 1.0f, 1.0f},  // Materializing = cyan
        {1.0f, 1.0f, 1.0f, 1.0f},  // Active = white
        {1.0f, 0.8f, 0.0f, 1.0f},  // Abandoned = yellow
        {1.0f, 0.2f, 0.2f, 1.0f},  // Derelict = red
        {0.5f, 0.5f, 0.5f, 0.7f},  // Deconstructed = gray
    };

    // Render building wireframe boxes
    if (m_buildingSystem) {
        const auto& entities = m_buildingSystem->get_factory().get_entities();
        for (const auto& entity : entities) {
            uint8_t stateIdx = entity.building.state;
            if (stateIdx >= 5) continue;

            float r = buildingColors[stateIdx][0];
            float g = buildingColors[stateIdx][1];
            float b = buildingColors[stateIdx][2];
            float a = buildingColors[stateIdx][3];

            float bx = static_cast<float>(entity.grid_x);
            float bz = static_cast<float>(entity.grid_y);
            float bw = static_cast<float>(entity.building.footprint_w);
            float bh = static_cast<float>(entity.building.footprint_h);
            float by = getElevY(entity.grid_x, entity.grid_y);

            // Building height based on level (1-5)
            float buildingHeight = 1.0f + static_cast<float>(entity.building.level) * 0.8f;

            // If materializing, scale height by construction progress
            if (entity.building.state == static_cast<uint8_t>(building::BuildingState::Materializing)
                && entity.has_construction) {
                float progress = static_cast<float>(entity.construction.ticks_elapsed)
                               / static_cast<float>(std::max<uint16_t>(entity.construction.ticks_total, 1));
                buildingHeight *= progress;
            }

            float topY = by + buildingHeight;

            // Bottom rectangle
            emitLine(bx, by, bz, bx + bw, by, bz, r, g, b, a);
            emitLine(bx + bw, by, bz, bx + bw, by, bz + bh, r, g, b, a);
            emitLine(bx + bw, by, bz + bh, bx, by, bz + bh, r, g, b, a);
            emitLine(bx, by, bz + bh, bx, by, bz, r, g, b, a);

            // Top rectangle
            emitLine(bx, topY, bz, bx + bw, topY, bz, r, g, b, a);
            emitLine(bx + bw, topY, bz, bx + bw, topY, bz + bh, r, g, b, a);
            emitLine(bx + bw, topY, bz + bh, bx, topY, bz + bh, r, g, b, a);
            emitLine(bx, topY, bz + bh, bx, topY, bz, r, g, b, a);

            // Vertical edges
            emitLine(bx, by, bz, bx, topY, bz, r, g, b, a);
            emitLine(bx + bw, by, bz, bx + bw, topY, bz, r, g, b, a);
            emitLine(bx + bw, by, bz + bh, bx + bw, topY, bz + bh, r, g, b, a);
            emitLine(bx, by, bz + bh, bx, topY, bz + bh, r, g, b, a);
        }
    }

    // Draw cursor crosshair at camera focus (shows where zones will be placed)
    if (m_zoneMode > 0) {
        float cx = m_demoCamera.focus_point.x;
        float cz = m_demoCamera.focus_point.z;
        float cy = getElevY(static_cast<int32_t>(cx), static_cast<int32_t>(cz)) + 0.3f;
        float crossSize = 3.0f;

        // White crosshair
        emitLine(cx - crossSize, cy, cz, cx + crossSize, cy, cz, 1.0f, 1.0f, 1.0f, 1.0f);
        emitLine(cx, cy, cz - crossSize, cx, cy, cz + crossSize, 1.0f, 1.0f, 1.0f, 1.0f);

        // Zone placement preview (5x5 outline)
        float px = std::floor(cx) - 2.0f;
        float pz = std::floor(cz) - 2.0f;
        float py = cy;
        uint8_t typeIdx = static_cast<uint8_t>(m_zoneMode - 1);
        float pr = zoneColors[typeIdx][0];
        float pg = zoneColors[typeIdx][1];
        float pb = zoneColors[typeIdx][2];

        emitLine(px, py, pz, px + 5.0f, py, pz, pr, pg, pb, 0.6f);
        emitLine(px + 5.0f, py, pz, px + 5.0f, py, pz + 5.0f, pr, pg, pb, 0.6f);
        emitLine(px + 5.0f, py, pz + 5.0f, px, py, pz + 5.0f, pr, pg, pb, 0.6f);
        emitLine(px, py, pz + 5.0f, px, py, pz, pr, pg, pb, 0.6f);
    }

    m_overlayVertexCount = vertexIndex;
    SDL_UnmapGPUTransferBuffer(device, m_overlayTransferBuffer);

    if (m_overlayVertexCount == 0) return;

    // Upload vertex data to GPU
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmdBuffer);

    SDL_GPUTransferBufferLocation src{};
    src.transfer_buffer = m_overlayTransferBuffer;
    src.offset = 0;

    SDL_GPUBufferRegion dst{};
    dst.buffer = m_overlayVertexBuffer;
    dst.offset = 0;
    dst.size = m_overlayVertexCount * sizeof(float) * 7;

    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);

    // Calculate viewProjection matrix (same as terrain camera)
    float pitchRad = glm::radians(m_demoCamera.pitch);
    float yawRad = glm::radians(m_demoCamera.yaw);

    glm::vec3 offset;
    offset.x = m_demoCamera.distance * cos(pitchRad) * sin(yawRad);
    offset.y = m_demoCamera.distance * sin(pitchRad);
    offset.z = m_demoCamera.distance * cos(pitchRad) * cos(yawRad);

    glm::vec3 cameraPos = m_demoCamera.focus_point + offset;
    glm::vec3 target = m_demoCamera.focus_point;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(cameraPos, target, up);

    float aspectRatio = static_cast<float>(m_window->getWidth()) /
                        static_cast<float>(m_window->getHeight());
    glm::mat4 projection = glm::perspectiveRH_ZO(
        glm::radians(45.0f),
        aspectRatio,
        0.1f,
        1000.0f
    );

    glm::mat4 viewProjection = projection * view;

    // Begin overlay render pass (LOAD existing color, no depth)
    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.texture = swapchain;
    colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdBuffer, &colorTargetInfo, 1, nullptr);

    SDL_BindGPUGraphicsPipeline(renderPass, m_overlayPipeline);

    SDL_GPUBufferBinding vertexBinding{};
    vertexBinding.buffer = m_overlayVertexBuffer;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBinding, 1);

    SDL_PushGPUVertexUniformData(cmdBuffer, 0, glm::value_ptr(viewProjection), sizeof(glm::mat4));

    SDL_DrawGPUPrimitives(renderPass, m_overlayVertexCount, 1, 0, 0);

    SDL_EndGPURenderPass(renderPass);
}

void Application::cleanupZoneBuilding() {
    if (!m_gpuDevice) return;

    SDL_GPUDevice* device = m_gpuDevice->getHandle();
    if (!device) return;

    SDL_WaitForGPUIdle(device);

    if (m_overlayPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(device, m_overlayPipeline);
        m_overlayPipeline = nullptr;
    }

    if (m_overlayVertexBuffer) {
        SDL_ReleaseGPUBuffer(device, m_overlayVertexBuffer);
        m_overlayVertexBuffer = nullptr;
    }

    if (m_overlayTransferBuffer) {
        SDL_ReleaseGPUTransferBuffer(device, m_overlayTransferBuffer);
        m_overlayTransferBuffer = nullptr;
    }

    if (m_overlayVertShader) {
        SDL_ReleaseGPUShader(device, m_overlayVertShader);
        m_overlayVertShader = nullptr;
    }

    if (m_overlayFragShader) {
        SDL_ReleaseGPUShader(device, m_overlayFragShader);
        m_overlayFragShader = nullptr;
    }

    m_buildingSystem.reset();
    m_zoneSystem.reset();
    m_zoneBuildingInitialized = false;
}

} // namespace sims3000
