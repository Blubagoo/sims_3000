/**
 * @file main.cpp
 * @brief Entry point for Sims 3000 - SimCity-inspired city builder
 *
 * Usage:
 *   sims_3000             - Run as client (default)
 *   sims_3000 --server    - Run as dedicated server
 *   sims_3000 --server --port 7778  - Server on custom port
 */

#include "sims3000/app/Application.h"
#include <SDL3/SDL_log.h>
#include <cstring>
#include <cstdlib>

namespace {

void printUsage(const char* programName) {
    SDL_Log("Usage: %s [options]", programName);
    SDL_Log("Options:");
    SDL_Log("  --server       Run as dedicated server (headless)");
    SDL_Log("  --port <num>   Server port (default: 7777)");
    SDL_Log("  --fullscreen   Start in fullscreen mode");
    SDL_Log("  --width <num>  Window width (default: 1280)");
    SDL_Log("  --height <num> Window height (default: 720)");
    SDL_Log("  --help         Show this help message");
}

sims3000::ApplicationConfig parseArgs(int argc, char* argv[]) {
    sims3000::ApplicationConfig config;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--server") == 0) {
            config.serverMode = true;
        } else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            config.serverPort = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--fullscreen") == 0) {
            config.startFullscreen = true;
        } else if (std::strcmp(argv[i], "--width") == 0 && i + 1 < argc) {
            config.windowWidth = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--height") == 0 && i + 1 < argc) {
            config.windowHeight = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            std::exit(0);
        }
    }

    return config;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    SDL_Log("Sims 3000 starting...");

    // Parse command line arguments
    sims3000::ApplicationConfig config = parseArgs(argc, argv);

    // Override title for server mode
    if (config.serverMode) {
        config.title = "Sims 3000 Server";
    }

    // Create and run application
    sims3000::Application app(config);

    if (!app.isValid()) {
        SDL_Log("Failed to initialize application");
        return 1;
    }

    return app.run();
}
