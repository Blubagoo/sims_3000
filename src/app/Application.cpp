/**
 * @file Application.cpp
 * @brief Application implementation.
 */

#include "sims3000/app/Application.h"
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

#ifdef SIMS3000_DEBUG
    m_systems->setProfilingEnabled(true);
#endif

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

        // Client-specific frame setup
        if (!m_serverMode) {
            m_input->beginFrame();
            m_frameStats.update(deltaTime);
            processEvents();
        }

        // Update simulation (both modes)
        updateSimulation();

        // Render (client only)
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
            // TODO: Start connection attempt
            break;

        case AppState::Loading:
            // TODO: Start asset loading
            break;

        case AppState::Playing:
            m_clock.setPaused(false);
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
            // TODO: Cancel connection if in progress
            break;

        case AppState::Loading:
            // TODO: Finalize loaded assets
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
    // Calculate frame delta for this update
    std::uint64_t currentTime = SDL_GetPerformanceCounter();
    float deltaTime = static_cast<float>(currentTime - m_lastFrameTime) /
                      static_cast<float>(SDL_GetPerformanceFrequency());

    // Accumulate time and process ticks
    int tickCount = m_clock.accumulate(deltaTime);

    auto tickStart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < tickCount; ++i) {
        m_systems->tick(m_clock);
        m_clock.advanceTick();
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

    // Clear systems first (they may reference assets)
    if (m_systems) {
        m_systems->clear();
    }

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

} // namespace sims3000
