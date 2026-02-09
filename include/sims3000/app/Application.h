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
#include "sims3000/render/CameraState.h"
#include "sims3000/render/ShaderCompiler.h"
#include "sims3000/terrain/TerrainGrid.h"
#include "sims3000/terrain/TerrainChunk.h"
#include "sims3000/terrain/TerrainChunkMeshGenerator.h"
#include "sims3000/terrain/TerrainVertex.h"
#include "sims3000/zone/ZoneSystem.h"
#include "sims3000/building/BuildingSystem.h"
#include "sims3000/building/ForwardDependencyStubs.h"
#include "sims3000/energy/EnergySystem.h"
#include "sims3000/fluid/FluidSystem.h"
#include "sims3000/transport/TransportSystem.h"
#include "sims3000/transport/RailSystem.h"
#include "sims3000/port/PortSystem.h"
#include "sims3000/services/ServicesSystem.h"

#include <memory>
#include <string>
#include <vector>

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

    // Demo rendering (for manual testing Epic 2)
    bool initDemo();
    void updateDemoCamera(float deltaTime);
    void renderDemo(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain);
    void cleanupDemo();

    CameraState m_demoCamera;
    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    SDL_GPUBuffer* m_demoVertexBuffer = nullptr;
    SDL_GPUBuffer* m_demoIndexBuffer = nullptr;
    SDL_GPUBuffer* m_demoUniformBuffer = nullptr;
    SDL_GPUGraphicsPipeline* m_demoPipeline = nullptr;
    SDL_GPUShader* m_demoVertShader = nullptr;
    SDL_GPUShader* m_demoFragShader = nullptr;
    bool m_demoInitialized = false;

    // Terrain rendering (Epic 3)
    bool initTerrain();
    void renderTerrain(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain);
    void cleanupTerrain();

    terrain::TerrainGrid m_terrainGrid;
    std::vector<terrain::TerrainChunk> m_terrainChunks;
    terrain::TerrainChunkMeshGenerator m_terrainMeshGenerator;
    SDL_GPUGraphicsPipeline* m_terrainPipeline = nullptr;
    SDL_GPUShader* m_terrainVertShader = nullptr;
    SDL_GPUShader* m_terrainFragShader = nullptr;
    bool m_terrainInitialized = false;

    // Zone/Building demo integration (Epic 4)
    bool initZoneBuilding();
    void tickZoneBuilding();
    void handleZoneBuildingInput();
    void renderZoneBuildingOverlay(SDL_GPUCommandBuffer* cmdBuffer, SDL_GPUTexture* swapchain);
    void cleanupZoneBuilding();

    std::unique_ptr<zone::ZoneSystem> m_zoneSystem;
    std::unique_ptr<building::BuildingSystem> m_buildingSystem;
    building::StubTransportProvider m_stubTransport;
    building::StubEnergyProvider m_stubEnergy;
    building::StubFluidProvider m_stubFluid;
    building::StubLandValueProvider m_stubLandValue;
    building::StubDemandProvider m_stubDemand;
    building::StubCreditProvider m_stubCredits;

    // Overlay rendering
    SDL_GPUGraphicsPipeline* m_overlayPipeline = nullptr;
    SDL_GPUShader* m_overlayVertShader = nullptr;
    SDL_GPUShader* m_overlayFragShader = nullptr;
    SDL_GPUBuffer* m_overlayVertexBuffer = nullptr;
    SDL_GPUTransferBuffer* m_overlayTransferBuffer = nullptr;
    uint32_t m_overlayVertexCount = 0;
    static constexpr uint32_t MAX_OVERLAY_VERTICES = 65536;
    bool m_zoneBuildingInitialized = false;

    // Zone placement mode: 0=none, 1=hab, 2=exch, 3=fab
    int m_zoneMode = 0;
    uint32_t m_zoneBuildingTickCounter = 0;

    // Energy demo integration (Epic 5)
    bool initEnergy();
    void tickEnergy();
    void handleEnergyInput();
    void cleanupEnergy();

    std::unique_ptr<energy::EnergySystem> m_energySystem;
    int m_energyMode = 0;  // 0=none, 1=carbon, 2=wind, 3=solar, 4=conduit
    bool m_energyOverlayEnabled = true;
    uint32_t m_energyTickLogCounter = 0;

    // Fluid demo integration (Epic 6)
    bool initFluid();
    void tickFluid();
    void handleFluidInput();
    void cleanupFluid();

    std::unique_ptr<fluid::FluidSystem> m_fluidSystem;
    int m_fluidMode = 0;  // 0=none, 1=extractor, 2=reservoir, 3=conduit
    bool m_fluidOverlayEnabled = true;
    uint32_t m_fluidTickLogCounter = 0;

    // Transport demo integration (Epic 7)
    bool initTransport();
    void tickTransport();
    void handleTransportInput();
    void cleanupTransport();

    std::unique_ptr<transport::TransportSystem> m_transportSystem;
    std::unique_ptr<transport::RailSystem> m_railSystem;
    int m_transportMode = 0;  // 0=none, 1=basic_pathway, 2=transit_corridor, 3=pedestrian, 4=rail, 5=terminal
    bool m_transportOverlayEnabled = true;
    uint32_t m_transportTickLogCounter = 0;

    // Port demo integration (Epic 8)
    bool initPort();
    void tickPort();
    void handlePortInput();
    void cleanupPort();

    std::unique_ptr<port::PortSystem> m_portSystem;
    int m_portMode = 0;  // 0=none, 1=aero, 2=aqua
    uint32_t m_portTickLogCounter = 0;

    // Services demo integration (Epic 9)
    bool initServices();
    void tickServices();
    void handleServicesInput();
    void cleanupServices();

    std::unique_ptr<services::ServicesSystem> m_services;
    int m_serviceMode = 0;  // 0=none, 1=enforcer, 2=hazard, 3=medical, 4=education
    uint32_t m_serviceTickLogCounter = 0;
};

} // namespace sims3000

#endif // SIMS3000_APP_APPLICATION_H
