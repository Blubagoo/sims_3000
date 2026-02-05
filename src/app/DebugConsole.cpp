/**
 * @file DebugConsole.cpp
 * @brief Debug console output implementation.
 */

#include "sims3000/app/DebugConsole.h"
#include "sims3000/assets/AssetManager.h"
#include "sims3000/ecs/Registry.h"
#include <SDL3/SDL_log.h>
#include <cstdio>

namespace sims3000 {

DebugConsole::DebugConsole() = default;

void DebugConsole::toggle() {
    m_enabled = !m_enabled;
    if (m_enabled) {
        SDL_Log("[DEBUG] Console output ENABLED (interval: %.1fs)", m_outputInterval);
    } else {
        SDL_Log("[DEBUG] Console output DISABLED");
    }
}

void DebugConsole::setEnabled(bool enabled) {
    m_enabled = enabled;
}

void DebugConsole::setOutputInterval(float seconds) {
    m_outputInterval = seconds > 0.1f ? seconds : 0.1f;
}

void DebugConsole::update(float deltaTime,
                          const FrameStats& frameStats,
                          const AssetManager* assets,
                          const Registry* registry) {
    if (!m_enabled) {
        return;
    }

    m_timeSinceOutput += deltaTime;
    if (m_timeSinceOutput >= m_outputInterval) {
        m_timeSinceOutput = 0.0f;
        outputStats(frameStats, assets, registry);
    }
}

void DebugConsole::outputStats(const FrameStats& frameStats,
                               const AssetManager* assets,
                               const Registry* registry) {
    // Output to stdout for developer monitoring
    std::printf("\n=== DEBUG STATS ===\n");

    // Frame timing
    std::printf("FPS: %.1f\n", frameStats.getFPS());
    std::printf("Frame time: %.2f ms (min: %.2f, max: %.2f)\n",
                frameStats.getFrameTimeMs(),
                frameStats.getMinFrameTimeMs(),
                frameStats.getMaxFrameTimeMs());

    // Simulation timing
    float avgTickMs = frameStats.getAvgTickTimeMs();
    if (avgTickMs > 0.0f) {
        std::printf("Tick time: %.2f ms (%.1f ticks/sec capacity)\n",
                    avgTickMs, 1000.0f / avgTickMs);
    }

    // Asset cache stats
    if (assets) {
        size_t textureCount, modelCount, textureBytes, modelBytes;
        assets->getCacheStats(textureCount, modelCount, textureBytes, modelBytes);
        std::printf("Textures: %zu (%.1f KB)\n", textureCount, textureBytes / 1024.0f);
        std::printf("Models: %zu (%.1f KB)\n", modelCount, modelBytes / 1024.0f);
    }

    // Entity count
    if (registry) {
        std::printf("Entities: %zu\n", registry->size());
    }

    std::printf("===================\n");
    std::fflush(stdout);
}

} // namespace sims3000
