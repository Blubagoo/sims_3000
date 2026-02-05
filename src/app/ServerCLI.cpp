/**
 * @file ServerCLI.cpp
 * @brief Server CLI implementation.
 */

#include "sims3000/app/ServerCLI.h"
#include <SDL3/SDL_log.h>
#include <iostream>
#include <sstream>
#include <cstdio>

namespace sims3000 {

ServerCLI::ServerCLI(const std::string& mapSize)
    : m_mapSize(mapSize)
{}

ServerCLI::~ServerCLI() {
    stop();
}

void ServerCLI::start() {
    if (m_running) {
        return;
    }

    m_running = true;
    m_inputThread = std::thread(&ServerCLI::inputThreadFunc, this);

    std::printf("\n");
    std::printf("=================================\n");
    std::printf("  ZergCity Server Console\n");
    std::printf("  Type 'help' for commands\n");
    std::printf("=================================\n");
    std::printf("\n> ");
    std::fflush(stdout);
}

void ServerCLI::stop() {
    m_running = false;
    if (m_inputThread.joinable()) {
        // Note: Thread may be blocked on stdin read
        // In production, would need platform-specific interrupt
        m_inputThread.detach();
    }
}

void ServerCLI::inputThreadFunc() {
    std::string line;
    while (m_running) {
        if (std::getline(std::cin, line)) {
            if (!line.empty()) {
                std::lock_guard<std::mutex> lock(m_commandMutex);
                m_pendingCommands.push(line);
            }
        } else {
            // EOF or error
            break;
        }
    }
}

void ServerCLI::processCommands() {
    std::queue<std::string> commands;
    {
        std::lock_guard<std::mutex> lock(m_commandMutex);
        std::swap(commands, m_pendingCommands);
    }

    while (!commands.empty()) {
        processCommand(commands.front());
        commands.pop();
        std::printf("> ");
        std::fflush(stdout);
    }
}

void ServerCLI::processCommand(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;

    // Get remaining args
    std::string args;
    std::getline(iss, args);
    if (!args.empty() && args[0] == ' ') {
        args = args.substr(1);
    }

    // Convert to lowercase
    for (char& c : cmd) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    if (cmd == "help" || cmd == "?") {
        cmdHelp();
    } else if (cmd == "status") {
        cmdStatus();
    } else if (cmd == "players") {
        cmdPlayers();
    } else if (cmd == "kick") {
        cmdKick(args);
    } else if (cmd == "say") {
        cmdSay(args);
    } else if (cmd == "save") {
        cmdSave();
    } else if (cmd == "shutdown" || cmd == "quit" || cmd == "exit") {
        cmdShutdown();
    } else {
        std::printf("Unknown command: '%s'. Type 'help' for available commands.\n", cmd.c_str());
    }
}

void ServerCLI::cmdHelp() {
    std::printf("\nAvailable commands:\n");
    std::printf("  help      - Show this help message\n");
    std::printf("  status    - Show server status (tick rate, uptime, map size)\n");
    std::printf("  players   - List connected overseers\n");
    std::printf("  kick <id> - Kick a player (placeholder)\n");
    std::printf("  say <msg> - Broadcast message to all players (placeholder)\n");
    std::printf("  save      - Force database checkpoint (placeholder)\n");
    std::printf("  shutdown  - Graceful server shutdown\n");
    std::printf("\n");
}

void ServerCLI::cmdStatus() {
    int hours = static_cast<int>(m_uptime / 3600);
    int minutes = static_cast<int>((m_uptime - hours * 3600) / 60);
    int seconds = static_cast<int>(m_uptime) % 60;

    std::printf("\n=== Server Status ===\n");
    std::printf("Uptime: %02d:%02d:%02d\n", hours, minutes, seconds);
    std::printf("Tick rate: 20 ticks/sec (50ms per tick)\n");
    std::printf("Map size: %s\n", m_mapSize.c_str());
    std::printf("Connected players: 0\n");  // Placeholder until Epic 1
    std::printf("=====================\n\n");
}

void ServerCLI::cmdPlayers() {
    std::printf("\n=== Connected Overseers ===\n");
    std::printf("No players connected.\n");  // Placeholder until Epic 1
    std::printf("===========================\n\n");
}

void ServerCLI::cmdKick(const std::string& args) {
    if (args.empty()) {
        std::printf("Usage: kick <player_id>\n");
        return;
    }
    std::printf("[PLACEHOLDER] Would kick player: %s\n", args.c_str());
    // Actual implementation in Epic 1 (networking)
}

void ServerCLI::cmdSay(const std::string& args) {
    if (args.empty()) {
        std::printf("Usage: say <message>\n");
        return;
    }
    std::printf("[PLACEHOLDER] Would broadcast: %s\n", args.c_str());
    // Actual implementation in Epic 1 (networking)
}

void ServerCLI::cmdSave() {
    std::printf("[PLACEHOLDER] Would force database checkpoint.\n");
    // Actual implementation in Epic 16 (save/load)
}

void ServerCLI::cmdShutdown() {
    std::printf("\n*** Server shutting down ***\n");
    if (m_shutdownCallback) {
        m_shutdownCallback();
    }
}

void ServerCLI::setShutdownCallback(ShutdownCallback callback) {
    m_shutdownCallback = std::move(callback);
}

void ServerCLI::update(float deltaTime) {
    m_uptime += deltaTime;
    m_timeSinceHeartbeat += deltaTime;
}

void ServerCLI::heartbeat() {
    if (m_heartbeatInterval <= 0.0f) {
        return;
    }

    if (m_timeSinceHeartbeat >= m_heartbeatInterval) {
        m_timeSinceHeartbeat = 0.0f;
        int hours = static_cast<int>(m_uptime / 3600);
        int minutes = static_cast<int>((m_uptime - hours * 3600) / 60);
        int seconds = static_cast<int>(m_uptime) % 60;
        SDL_Log("[HEARTBEAT] Server alive - Uptime: %02d:%02d:%02d",
                hours, minutes, seconds);
    }
}

void ServerCLI::setHeartbeatInterval(float seconds) {
    m_heartbeatInterval = seconds;
}

} // namespace sims3000
