/**
 * @file ServerCLI.h
 * @brief Server command-line interface for operators.
 *
 * Provides essential commands: status, players, kick, say, save, shutdown, help.
 */

#ifndef SIMS3000_APP_SERVERCLI_H
#define SIMS3000_APP_SERVERCLI_H

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

namespace sims3000 {

// Forward declarations
class Application;
class NetworkServer;

/**
 * @class ServerCLI
 * @brief Command-line interface for dedicated server operators.
 *
 * Reads commands from stdin on a background thread and queues
 * them for processing on the main thread.
 */
class ServerCLI {
public:
    using ShutdownCallback = std::function<void()>;

    /**
     * Create server CLI.
     * @param mapSize Map size string (small/medium/large)
     */
    explicit ServerCLI(const std::string& mapSize = "medium");
    ~ServerCLI();

    // Non-copyable
    ServerCLI(const ServerCLI&) = delete;
    ServerCLI& operator=(const ServerCLI&) = delete;

    /**
     * Start the CLI input thread.
     */
    void start();

    /**
     * Stop the CLI input thread.
     */
    void stop();

    /**
     * Process any pending commands.
     * Call this from the main thread each frame.
     */
    void processCommands();

    /**
     * Set callback for shutdown command.
     */
    void setShutdownCallback(ShutdownCallback callback);

    /**
     * Update server uptime.
     * @param deltaTime Frame delta time
     */
    void update(float deltaTime);

    /**
     * Output periodic heartbeat (if enabled).
     */
    void heartbeat();

    /**
     * Set heartbeat interval in seconds (0 to disable).
     */
    void setHeartbeatInterval(float seconds);

    /**
     * Get server uptime in seconds.
     */
    float getUptime() const { return m_uptime; }

    /**
     * Set the NetworkServer instance for CLI commands.
     * @param server Network server to use for players/kick/say commands.
     */
    void setNetworkServer(NetworkServer* server) { m_networkServer = server; }

private:
    void inputThreadFunc();
    void processCommand(const std::string& input);

    // Command handlers
    void cmdHelp();
    void cmdStatus();
    void cmdPlayers();
    void cmdKick(const std::string& args);
    void cmdSay(const std::string& args);
    void cmdSave();
    void cmdShutdown();

    std::string m_mapSize;
    std::thread m_inputThread;
    std::atomic<bool> m_running{false};

    std::mutex m_commandMutex;
    std::queue<std::string> m_pendingCommands;

    ShutdownCallback m_shutdownCallback;

    float m_uptime = 0.0f;
    float m_heartbeatInterval = 30.0f;
    float m_timeSinceHeartbeat = 0.0f;

    NetworkServer* m_networkServer = nullptr;
};

} // namespace sims3000

#endif // SIMS3000_APP_SERVERCLI_H
