/**
 * @file Config.h
 * @brief Configuration system with JSON persistence.
 */

#ifndef SIMS3000_APP_CONFIG_H
#define SIMS3000_APP_CONFIG_H

#include <string>
#include <cstdint>

namespace sims3000 {

/**
 * @struct GraphicsConfig
 * @brief Graphics/display settings.
 */
struct GraphicsConfig {
    int windowWidth = 1280;
    int windowHeight = 720;
    bool fullscreen = false;
    bool vsync = true;
    int targetFps = 60;
};

/**
 * @struct AudioConfig
 * @brief Audio settings.
 */
struct AudioConfig {
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    bool muted = false;
};

/**
 * @struct GameConfig
 * @brief Gameplay settings.
 */
struct GameConfig {
    std::string playerName = "Player";
    std::string lastServer = "";
    int lastPort = 7777;
    bool autoSave = true;
    int autoSaveIntervalMinutes = 5;
};

/**
 * @class Config
 * @brief Application configuration manager.
 *
 * Loads/saves settings from JSON file in user's app data directory.
 * Falls back to defaults if file doesn't exist.
 */
class Config {
public:
    Config();
    ~Config() = default;

    /**
     * Load configuration from default path.
     * @return true if loaded successfully
     */
    bool load();

    /**
     * Load configuration from specific path.
     * @param path Path to config file
     * @return true if loaded successfully
     */
    bool load(const std::string& path);

    /**
     * Save configuration to default path.
     * @return true if saved successfully
     */
    bool save() const;

    /**
     * Save configuration to specific path.
     * @param path Path to config file
     * @return true if saved successfully
     */
    bool save(const std::string& path) const;

    /**
     * Reset all settings to defaults.
     */
    void resetToDefaults();

    /**
     * Get the default config path.
     * On Windows: %APPDATA%/Sims3000/config.json
     */
    static std::string getDefaultPath();

    // Settings accessors
    GraphicsConfig& graphics() { return m_graphics; }
    const GraphicsConfig& graphics() const { return m_graphics; }

    AudioConfig& audio() { return m_audio; }
    const AudioConfig& audio() const { return m_audio; }

    GameConfig& game() { return m_game; }
    const GameConfig& game() const { return m_game; }

private:
    GraphicsConfig m_graphics;
    AudioConfig m_audio;
    GameConfig m_game;
};

} // namespace sims3000

#endif // SIMS3000_APP_CONFIG_H
