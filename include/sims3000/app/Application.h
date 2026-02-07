/**
 * @file Application.h
 * @brief Main application class managing the game lifecycle.
 */

#ifndef SIMS3000_APP_APPLICATION_H
#define SIMS3000_APP_APPLICATION_H

#include "sims3000/app/AppState.h"
#include "sims3000/app/Config.h"
#include "sims3000/app/SimulationClock.h"
#include "sims3000/app/FrameStats.h"
#include "sims3000/render/Window.h"
#include "sims3000/render/GPUDevice.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/input/CameraController.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/SystemManager.h"
#include "sims3000/assets/AssetManager.h"
#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/NetworkClient.h"
#include "sims3000/sync/SyncSystem.h"
#include "sims3000/render/ToonPipeline.h"

#include <memory>
#include <string>

namespace sims3000 {

/**
 * @struct ApplicationConfig
 * @brief Configuration for application initialization.
 */
struct ApplicationConfig {
    const char* title = "Sims 3000";
    int windowWidth = 1280;
    int windowHeight = 720;
    bool startFullscreen = false;
    bool serverMode = false;        // Run as headless server
    int serverPort = 7777;          // Server listen port
    std::string connectAddress;     // Server address to connect to (client mode)
    std::uint16_t connectPort = 0;  // Server port to connect to (client mode, 0 = don't auto-connect)
    std::string playerName = "Player";  // Player name for multiplayer
    MapSizeTier mapSize = MapSizeTier::Medium;  // Map size tier (server mode)
};

/**
 * @class Application
 * @brief Main application class orchestrating all subsystems.
 *
 * Manages the game loop, including:
 * - Fixed timestep simulation (20 Hz)
 * - Variable rate rendering (~60 fps target)
 * - Input handling
 * - System updates
 * - Asset management
 *
 * Supports both client and dedicated server modes.
 */
class Application {
public:
    /**
     * Create application with configuration.
     */
    explicit Application(const ApplicationConfig& config = {});
    ~Application();

    // Non-copyable, non-movable (singleton-like lifecycle)
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * Check if initialization succeeded.
     */
    bool isValid() const;

    /**
     * Check if running in server mode.
     */
    bool isServerMode() const;

    /**
     * Run the main game loop.
     * Blocks until shutdown is requested.
     * @return Exit code (0 = success)
     */
    int run();

    /**
     * Request application shutdown.
     * Can be called from any system or input handler.
     */
    void requestShutdown();

    /**
     * Get current application state.
     */
    AppState getState() const;

    /**
     * Request state transition.
     * @param newState Target state
     */
    void requestStateChange(AppState newState);

    /**
     * Get the simulation clock.
     */
    const ISimulationTime& getSimulationTime() const;

    /**
     * Get the input system (client only).
     */
    InputSystem& getInput();

    /**
     * Get the ECS registry.
     */
    Registry& getRegistry();

    /**
     * Get the system manager.
     */
    SystemManager& getSystems();

    /**
     * Get the asset manager (client only).
     */
    AssetManager& getAssets();

    /**
     * Get frame statistics.
     */
    const FrameStats& getFrameStats() const;

    /**
     * Get the window (client only).
     */
    Window& getWindow();

    /**
     * Get the GPU device (client only).
     */
    GPUDevice& getGPUDevice();

    /**
     * Get the toon pipeline (client only).
     */
    ToonPipeline& getToonPipeline();

    /**
     * Check if wireframe mode is enabled (client only).
     */
    bool isWireframeEnabled() const;

    /**
     * Toggle wireframe rendering mode (client only).
     * @return New wireframe state (true = enabled)
     */
    bool toggleWireframe();

    /**
     * Get configuration.
     */
    Config& getConfig();
    const Config& getConfig() const;

    /**
     * Get the network server (server mode only).
     * @return Pointer to NetworkServer, nullptr in client mode.
     */
    NetworkServer* getNetworkServer();

    /**
     * Get the network client (client mode only).
     * @return Pointer to NetworkClient, nullptr in server mode.
     */
    NetworkClient* getNetworkClient();

    /**
     * Get the sync system.
     */
    SyncSystem& getSyncSystem();

    /**
     * Connect to a server (client mode).
     * @param address Server IP/hostname.
     * @param port Server port.
     * @return true if connection attempt started.
     */
    bool connectToServer(const std::string& address, std::uint16_t port);

    /**
     * Disconnect from server (client mode).
     */
    void disconnectFromServer();

    /**
     * Get current tick number (for display/debugging).
     */
    SimulationTick getCurrentTick() const;

private:
    void processEvents();
    void processNetworkMessages();
    void updateSimulation();
    void generateAndSendDeltas();
    void applyPendingStateUpdates();
    void render();
    void shutdown();
    void transitionState(AppState newState);
    void onStateEnter(AppState state);
    void onStateExit(AppState state);
    void onClientStateChange(ConnectionState oldState, ConnectionState newState);
    void initializeNetworking();
    void shutdownNetworking();

    ApplicationConfig m_appConfig;
    Config m_config;
    bool m_valid = false;
    bool m_running = false;
    bool m_serverMode = false;
    AppState m_currentState = AppState::Menu;
    AppState m_pendingState = AppState::Menu;
    bool m_stateChangeRequested = false;

    // Core systems (order matters for initialization)
    // Window, GPUDevice, Input, Assets, ToonPipeline are nullptr in server mode
    std::unique_ptr<GPUDevice> m_gpuDevice;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<InputSystem> m_input;
    std::unique_ptr<AssetManager> m_assets;
    std::unique_ptr<Registry> m_registry;
    std::unique_ptr<SystemManager> m_systems;
    std::unique_ptr<ToonPipeline> m_toonPipeline;

    // Networking (server XOR client, not both)
    std::unique_ptr<NetworkServer> m_networkServer;
    std::unique_ptr<NetworkClient> m_networkClient;
    std::unique_ptr<SyncSystem> m_syncSystem;

    SimulationClock m_clock;
    FrameStats m_frameStats;

    // Timing
    std::uint64_t m_lastFrameTime = 0;
};

} // namespace sims3000

#endif // SIMS3000_APP_APPLICATION_H
