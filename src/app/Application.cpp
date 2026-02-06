/**
 * @file Application.cpp
 * @brief Application implementation.
 */

#include "sims3000/app/Application.h"
#include "sims3000/net/ENetTransport.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_log.h>
#include <chrono>
#include <thread>

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
        // Client mode - create window
        m_window = std::make_unique<Window>(
            m_appConfig.title,
            m_appConfig.windowWidth,
            m_appConfig.windowHeight
        );

        if (!m_window->isValid()) {
            SDL_Log("Failed to create window");
            return;
        }

        if (m_appConfig.startFullscreen) {
            m_window->toggleFullscreen();
        }

        // Create input system (client only)
        m_input = std::make_unique<InputSystem>();

        // Create asset manager (client only)
        m_assets = std::make_unique<AssetManager>(*m_window);

#ifdef SIMS3000_DEBUG
        m_assets->setHotReloadEnabled(true);
#endif
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

        // 5. Render (client only)
        if (!m_serverMode) {
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
    if (!m_window) return;

    SDL_GPUCommandBuffer* cmdBuffer = m_window->acquireCommandBuffer();
    if (!cmdBuffer) {
        return;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!m_window->acquireSwapchainTexture(cmdBuffer, &swapchainTexture)) {
        return;
    }

    if (swapchainTexture) {
        // Begin render pass
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = swapchainTexture;
        colorTarget.clear_color = {0.1f, 0.1f, 0.15f, 1.0f};  // Dark blue-gray
        colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
            cmdBuffer, &colorTarget, 1, nullptr);

        // TODO: Render scene here
        // For now just clear to show we're working

        SDL_EndGPURenderPass(renderPass);
    }

    m_window->submit(cmdBuffer);
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

    // Destroy systems in reverse order of creation
    m_systems.reset();
    m_registry.reset();
    m_assets.reset();
    m_input.reset();
    m_window.reset();

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

} // namespace sims3000
