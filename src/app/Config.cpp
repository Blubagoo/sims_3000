/**
 * @file Config.cpp
 * @brief Config implementation.
 */

#include "sims3000/app/Config.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_filesystem.h>
#include <fstream>

#ifdef SIMS3000_HAS_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace sims3000 {

Config::Config() {
    resetToDefaults();
}

bool Config::load() {
    return load(getDefaultPath());
}

bool Config::load(const std::string& path) {
#ifdef SIMS3000_HAS_JSON
    std::ifstream file(path);
    if (!file.is_open()) {
        SDL_Log("Config file not found: %s (using defaults)", path.c_str());
        return false;
    }

    try {
        json j;
        file >> j;

        // Graphics
        if (j.contains("graphics")) {
            auto& g = j["graphics"];
            if (g.contains("windowWidth")) m_graphics.windowWidth = g["windowWidth"];
            if (g.contains("windowHeight")) m_graphics.windowHeight = g["windowHeight"];
            if (g.contains("fullscreen")) m_graphics.fullscreen = g["fullscreen"];
            if (g.contains("vsync")) m_graphics.vsync = g["vsync"];
            if (g.contains("targetFps")) m_graphics.targetFps = g["targetFps"];
        }

        // Audio
        if (j.contains("audio")) {
            auto& a = j["audio"];
            if (a.contains("masterVolume")) m_audio.masterVolume = a["masterVolume"];
            if (a.contains("musicVolume")) m_audio.musicVolume = a["musicVolume"];
            if (a.contains("sfxVolume")) m_audio.sfxVolume = a["sfxVolume"];
            if (a.contains("muted")) m_audio.muted = a["muted"];
        }

        // Game
        if (j.contains("game")) {
            auto& gm = j["game"];
            if (gm.contains("playerName")) m_game.playerName = gm["playerName"];
            if (gm.contains("lastServer")) m_game.lastServer = gm["lastServer"];
            if (gm.contains("lastPort")) m_game.lastPort = gm["lastPort"];
            if (gm.contains("autoSave")) m_game.autoSave = gm["autoSave"];
            if (gm.contains("autoSaveIntervalMinutes")) m_game.autoSaveIntervalMinutes = gm["autoSaveIntervalMinutes"];
        }

        SDL_Log("Config loaded from: %s", path.c_str());
        return true;
    } catch (const std::exception& e) {
        SDL_Log("Failed to parse config: %s", e.what());
        return false;
    }
#else
    (void)path;
    SDL_Log("JSON support not available, using default config");
    return false;
#endif
}

bool Config::save() const {
    return save(getDefaultPath());
}

bool Config::save(const std::string& path) const {
#ifdef SIMS3000_HAS_JSON
    // Ensure directory exists
    std::string dir = path.substr(0, path.find_last_of("/\\"));
    SDL_CreateDirectory(dir.c_str());

    json j;

    // Graphics
    j["graphics"]["windowWidth"] = m_graphics.windowWidth;
    j["graphics"]["windowHeight"] = m_graphics.windowHeight;
    j["graphics"]["fullscreen"] = m_graphics.fullscreen;
    j["graphics"]["vsync"] = m_graphics.vsync;
    j["graphics"]["targetFps"] = m_graphics.targetFps;

    // Audio
    j["audio"]["masterVolume"] = m_audio.masterVolume;
    j["audio"]["musicVolume"] = m_audio.musicVolume;
    j["audio"]["sfxVolume"] = m_audio.sfxVolume;
    j["audio"]["muted"] = m_audio.muted;

    // Game
    j["game"]["playerName"] = m_game.playerName;
    j["game"]["lastServer"] = m_game.lastServer;
    j["game"]["lastPort"] = m_game.lastPort;
    j["game"]["autoSave"] = m_game.autoSave;
    j["game"]["autoSaveIntervalMinutes"] = m_game.autoSaveIntervalMinutes;

    std::ofstream file(path);
    if (!file.is_open()) {
        SDL_Log("Failed to open config file for writing: %s", path.c_str());
        return false;
    }

    file << j.dump(2);
    SDL_Log("Config saved to: %s", path.c_str());
    return true;
#else
    (void)path;
    SDL_Log("JSON support not available, cannot save config");
    return false;
#endif
}

void Config::resetToDefaults() {
    m_graphics = GraphicsConfig{};
    m_audio = AudioConfig{};
    m_game = GameConfig{};
}

std::string Config::getDefaultPath() {
    // Get user's app data directory
    char* prefPath = SDL_GetPrefPath("Sims3000", "Sims3000");
    if (prefPath) {
        std::string path = std::string(prefPath) + "config.json";
        SDL_free(prefPath);
        return path;
    }
    return "config.json";
}

} // namespace sims3000
