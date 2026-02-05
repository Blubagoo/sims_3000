/**
 * @file Application.h
 * @brief Main application class managing the game lifecycle.
 */

#ifndef SIMS3000_APP_APPLICATION_H
#define SIMS3000_APP_APPLICATION_H

#include "sims3000/app/SimulationClock.h"
#include "sims3000/app/FrameStats.h"
#include "sims3000/render/Window.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/input/CameraController.h"
#include "sims3000/ecs/Registry.h"
#include "sims3000/ecs/SystemManager.h"
#include "sims3000/assets/AssetManager.h"

#include <memory>

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
     * Get the simulation clock.
     */
    const ISimulationTime& getSimulationTime() const;

    /**
     * Get the input system.
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
     * Get the asset manager.
     */
    AssetManager& getAssets();

    /**
     * Get frame statistics.
     */
    const FrameStats& getFrameStats() const;

    /**
     * Get the window.
     */
    Window& getWindow();

private:
    void processEvents();
    void updateSimulation();
    void render();
    void shutdown();

    ApplicationConfig m_config;
    bool m_valid = false;
    bool m_running = false;

    // Core systems (order matters for initialization)
    std::unique_ptr<Window> m_window;
    std::unique_ptr<InputSystem> m_input;
    std::unique_ptr<AssetManager> m_assets;
    std::unique_ptr<Registry> m_registry;
    std::unique_ptr<SystemManager> m_systems;

    SimulationClock m_clock;
    FrameStats m_frameStats;

    // Timing
    std::uint64_t m_lastFrameTime = 0;
};

} // namespace sims3000

#endif // SIMS3000_APP_APPLICATION_H
