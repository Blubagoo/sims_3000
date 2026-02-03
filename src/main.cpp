/**
 * @file main.cpp
 * @brief Entry point for Sims 3000 - SimCity-inspired city builder
 */

#include <SDL3/SDL.h>

int main(int argc, char* argv[])
{
    // Suppress unused parameter warnings
    (void)argc;
    (void)argv;

    // Initialize SDL with video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    // Create window centered on screen
    SDL_Window* window = SDL_CreateWindow(
        "Sims 3000",
        800,
        600,
        0  // No special flags
    );

    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Create renderer for the window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Main event loop
    bool running = true;
    SDL_Event event;

    while (running) {
        // Poll all pending events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;

                case SDL_EVENT_KEY_DOWN:
                    // Close on ESC key
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;

                default:
                    break;
            }
        }

        // Clear screen to dark blue-gray color
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderClear(renderer);

        // Present the rendered frame
        SDL_RenderPresent(renderer);
    }

    // Clean shutdown
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
