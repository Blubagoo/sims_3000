/**
 * @file main.cpp
 * @brief Entry point for Sims 3000 - SimCity-inspired city builder
 */

#include "sims3000/app/Application.h"
#include <SDL3/SDL_log.h>

int main(int argc, char* argv[]) {
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;

    SDL_Log("Sims 3000 starting...");

    // Configure application
    sims3000::ApplicationConfig config;
    config.title = "Sims 3000";
    config.windowWidth = 1280;
    config.windowHeight = 720;
    config.startFullscreen = false;

    // Create and run application
    sims3000::Application app(config);

    if (!app.isValid()) {
        SDL_Log("Failed to initialize application");
        return 1;
    }

    return app.run();
}
