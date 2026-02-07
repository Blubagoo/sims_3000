/**
 * @file ToonShaderConfig.cpp
 * @brief Implementation of runtime-configurable toon shader parameters.
 *
 * Includes terrain visual configuration integration (Ticket 3-039).
 */

#include "sims3000/render/ToonShaderConfig.h"
#include "sims3000/render/TerrainVisualConfig.h"
#include <algorithm>
#include <cmath>
#include <fstream>

#ifdef SIMS3000_HAS_JSON
#include <nlohmann/json.hpp>
#endif

namespace sims3000 {

// =============================================================================
// Singleton Instance
// =============================================================================

ToonShaderConfig& ToonShaderConfig::instance() {
    static ToonShaderConfig s_instance;
    return s_instance;
}

// =============================================================================
// Constructor
// =============================================================================

ToonShaderConfig::ToonShaderConfig() {
    initializeDefaults();
}

// =============================================================================
// Band Configuration
// =============================================================================

void ToonShaderConfig::setBandCount(std::size_t count) {
    count = std::clamp(count, static_cast<std::size_t>(1), MAX_BANDS);
    if (m_bandCount != count) {
        m_bandCount = count;
        markDirty();
    }
}

float ToonShaderConfig::getBandThreshold(std::size_t bandIndex) const {
    if (bandIndex >= MAX_BANDS) {
        return 1.0f;
    }
    return m_bands[bandIndex].threshold;
}

void ToonShaderConfig::setBandThreshold(std::size_t bandIndex, float threshold) {
    if (bandIndex >= MAX_BANDS) {
        return;
    }
    threshold = std::clamp(threshold, 0.0f, 1.0f);
    if (m_bands[bandIndex].threshold != threshold) {
        m_bands[bandIndex].threshold = threshold;
        markDirty();
    }
}

float ToonShaderConfig::getBandIntensity(std::size_t bandIndex) const {
    if (bandIndex >= MAX_BANDS) {
        return 1.0f;
    }
    return m_bands[bandIndex].intensity;
}

void ToonShaderConfig::setBandIntensity(std::size_t bandIndex, float intensity) {
    if (bandIndex >= MAX_BANDS) {
        return;
    }
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    if (m_bands[bandIndex].intensity != intensity) {
        m_bands[bandIndex].intensity = intensity;
        markDirty();
    }
}

// =============================================================================
// Shadow Configuration
// =============================================================================

void ToonShaderConfig::setShadowShiftAmount(float amount) {
    amount = std::clamp(amount, 0.0f, 1.0f);
    if (m_shadowShiftAmount != amount) {
        m_shadowShiftAmount = amount;
        markDirty();
    }
}

// =============================================================================
// Edge Configuration
// =============================================================================

void ToonShaderConfig::setEdgeLineWidth(float width) {
    width = std::clamp(width, 0.0f, 10.0f);
    if (m_edgeLineWidth != width) {
        m_edgeLineWidth = width;
        markDirty();
    }
}

void ToonShaderConfig::setEdgeColor(const glm::vec4& color) {
    if (m_edgeColor != color) {
        m_edgeColor = color;
        markDirty();
    }
}

// =============================================================================
// Bloom Configuration
// =============================================================================

void ToonShaderConfig::setBloomThreshold(float threshold) {
    threshold = std::clamp(threshold, 0.0f, 1.0f);
    if (m_bloomThreshold != threshold) {
        m_bloomThreshold = threshold;
        markDirty();
    }
}

void ToonShaderConfig::setBloomIntensity(float intensity) {
    intensity = std::clamp(intensity, 0.0f, 2.0f);
    if (m_bloomIntensity != intensity) {
        m_bloomIntensity = intensity;
        markDirty();
    }
}

// =============================================================================
// Emissive Configuration
// =============================================================================

void ToonShaderConfig::setEmissiveMultiplier(float multiplier) {
    multiplier = std::clamp(multiplier, 0.0f, 2.0f);
    if (m_emissiveMultiplier != multiplier) {
        m_emissiveMultiplier = multiplier;
        markDirty();
    }
}

const TerrainEmissivePreset& ToonShaderConfig::getTerrainEmissivePreset(TerrainType type) const {
    std::size_t index = static_cast<std::size_t>(type);
    if (index >= TERRAIN_TYPE_COUNT) {
        // Return FlatGround preset as fallback
        return m_terrainEmissivePresets[0];
    }
    return m_terrainEmissivePresets[index];
}

void ToonShaderConfig::setTerrainEmissivePreset(TerrainType type, const TerrainEmissivePreset& preset) {
    std::size_t index = static_cast<std::size_t>(type);
    if (index >= TERRAIN_TYPE_COUNT) {
        return;
    }
    m_terrainEmissivePresets[index] = preset;
    markDirty();
}

// =============================================================================
// Ambient Configuration
// =============================================================================

void ToonShaderConfig::setAmbientLevel(float level) {
    level = std::clamp(level, 0.0f, 1.0f);
    if (m_ambientLevel != level) {
        m_ambientLevel = level;
        markDirty();
    }
}

// =============================================================================
// Preset Application
// =============================================================================

void ToonShaderConfig::resetToDefaults() {
    initializeDefaults();
}

void ToonShaderConfig::applyDayPalette() {
    // Brighter ambient for daytime
    setAmbientLevel(0.12f);

    // Reduced bloom (less prominent glow in daylight)
    setBloomThreshold(0.8f);
    setBloomIntensity(0.7f);

    // Slightly reduced shadow shift (shadows less purple)
    setShadowShiftAmount(0.5f);

    // Reduced emissive multiplier (glow less prominent in daylight)
    setEmissiveMultiplier(0.8f);

    markDirty();
}

void ToonShaderConfig::applyNightPalette() {
    // Darker ambient for nighttime
    setAmbientLevel(0.05f);

    // Lower bloom threshold (more glow in darkness)
    setBloomThreshold(0.5f);
    setBloomIntensity(1.3f);

    // Full shadow shift (deep purple shadows)
    setShadowShiftAmount(0.85f);

    // Increased emissive multiplier (glow more prominent)
    setEmissiveMultiplier(1.4f);

    markDirty();
}

void ToonShaderConfig::applyHighContrastPreset() {
    // Higher ambient to ensure visibility
    setAmbientLevel(0.1f);

    // Thicker edge lines for visibility
    setEdgeLineWidth(2.0f);

    // Increased band contrast
    setBandIntensity(0, 0.1f);   // Darker deep shadows
    setBandIntensity(1, 0.25f);  // Darker shadows
    setBandIntensity(2, 0.7f);   // Brighter mid
    setBandIntensity(3, 1.0f);   // Full lit

    // Slightly brighter emissive
    setEmissiveMultiplier(1.2f);

    markDirty();
}

// =============================================================================
// Initialization
// =============================================================================

void ToonShaderConfig::initializeDefaults() {
    using namespace ToonShaderConfigDefaults;

    // Band configuration
    m_bandCount = MAX_BANDS;
    m_bands[0] = {BAND_THRESHOLD_0, BAND_INTENSITY_0};  // Deep shadow
    m_bands[1] = {BAND_THRESHOLD_1, BAND_INTENSITY_1};  // Shadow
    m_bands[2] = {BAND_THRESHOLD_2, BAND_INTENSITY_2};  // Mid
    m_bands[3] = {BAND_THRESHOLD_3, BAND_INTENSITY_3};  // Lit

    // Shadow configuration
    m_shadowColor = glm::vec3(SHADOW_COLOR_R, SHADOW_COLOR_G, SHADOW_COLOR_B);
    m_shadowShiftAmount = SHADOW_SHIFT_AMOUNT;

    // Edge configuration
    m_edgeLineWidth = EDGE_LINE_WIDTH;
    m_edgeColor = glm::vec4(EDGE_COLOR_R, EDGE_COLOR_G, EDGE_COLOR_B, EDGE_COLOR_A);

    // Bloom configuration
    m_bloomThreshold = BLOOM_THRESHOLD;
    m_bloomIntensity = BLOOM_INTENSITY;

    // Emissive configuration
    m_emissiveMultiplier = EMISSIVE_MULTIPLIER;
    initializeTerrainEmissivePresets();

    // Ambient configuration
    m_ambientLevel = AMBIENT_LEVEL;

    // Mark as dirty so initial values are uploaded
    m_dirty = true;
}

void ToonShaderConfig::initializeTerrainEmissivePresets() {
    // FlatGround: Subtle moss glow (dark green/teal)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::FlatGround)] = {
        glm::vec3(0.1f, 0.3f, 0.25f),  // Subtle dark green
        0.2f                           // Low intensity
    };

    // Hills: Glowing vein patterns (blue/cyan)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::Hills)] = {
        glm::vec3(0.2f, 0.5f, 0.8f),   // Blue-cyan veins
        0.3f
    };

    // Ocean: Deep water bioluminescence (deep blue with soft glow)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::Ocean)] = {
        glm::vec3(0.1f, 0.2f, 0.5f),   // Deep blue
        0.4f
    };

    // River: Flowing water particles (soft blue/white)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::River)] = {
        glm::vec3(0.3f, 0.5f, 0.7f),   // Soft blue
        0.5f
    };

    // Lake: Inland water glow (teal)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::Lake)] = {
        glm::vec3(0.2f, 0.6f, 0.7f),   // Teal
        0.4f
    };

    // Forest: Alien vegetation (teal/green bioluminescence)
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::Forest)] = {
        glm::vec3(0.0f, 0.8f, 0.6f),   // Teal-green
        0.6f
    };

    // CrystalFields: Bright magenta/cyan crystal spires
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::CrystalFields)] = {
        glm::vec3(0.8f, 0.2f, 0.8f),   // Magenta-pink
        0.9f                           // High intensity
    };

    // SporePlains: Pulsing green/teal spore clouds
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::SporePlains)] = {
        glm::vec3(0.2f, 0.9f, 0.5f),   // Bright green with teal
        0.7f
    };

    // ToxicMarshes: Sickly yellow-green glow
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::ToxicMarshes)] = {
        glm::vec3(0.7f, 0.8f, 0.1f),   // Yellow-green
        0.6f
    };

    // VolcanicRock: Orange/red glow cracks
    m_terrainEmissivePresets[static_cast<std::size_t>(TerrainType::VolcanicRock)] = {
        glm::vec3(0.9f, 0.4f, 0.1f),   // Orange-red
        0.7f
    };
}

// =============================================================================
// Terrain Visual Configuration Integration (Ticket 3-039)
// =============================================================================

TerrainVisualConfigManager& ToonShaderConfig::getTerrainVisualConfig() {
    return TerrainVisualConfigManager::instance();
}

const TerrainVisualConfigManager& ToonShaderConfig::getTerrainVisualConfig() const {
    return TerrainVisualConfigManager::instance();
}

bool ToonShaderConfig::isTerrainConfigDirty() const {
    return TerrainVisualConfigManager::instance().isDirty();
}

void ToonShaderConfig::clearTerrainDirtyFlag() {
    TerrainVisualConfigManager::instance().clearDirtyFlag();
}

bool ToonShaderConfig::isAnyDirty() const {
    return m_dirty || TerrainVisualConfigManager::instance().isDirty();
}

void ToonShaderConfig::clearAllDirtyFlags() {
    m_dirty = false;
    TerrainVisualConfigManager::instance().clearDirtyFlag();
}

bool ToonShaderConfig::loadTerrainConfigFromFile(const std::string& filepath) {
    return TerrainVisualConfigManager::instance().loadFromFile(filepath);
}

bool ToonShaderConfig::saveTerrainConfigToFile(const std::string& filepath) const {
    return TerrainVisualConfigManager::instance().saveToFile(filepath);
}

void ToonShaderConfig::resetTerrainConfigToDefaults() {
    TerrainVisualConfigManager::instance().resetToDefaults();
}

// =============================================================================
// TerrainVisualConfigManager Implementation
// =============================================================================

#ifdef SIMS3000_HAS_JSON

namespace {
    // Helper to convert GlowBehavior to string
    std::string glowBehaviorToString(GlowBehavior behavior) {
        switch (behavior) {
            case GlowBehavior::Static: return "static";
            case GlowBehavior::Pulse: return "pulse";
            case GlowBehavior::Shimmer: return "shimmer";
            case GlowBehavior::Flow: return "flow";
            case GlowBehavior::Irregular: return "irregular";
            default: return "static";
        }
    }

    // Helper to convert string to GlowBehavior
    GlowBehavior stringToGlowBehavior(const std::string& str) {
        if (str == "pulse") return GlowBehavior::Pulse;
        if (str == "shimmer") return GlowBehavior::Shimmer;
        if (str == "flow") return GlowBehavior::Flow;
        if (str == "irregular") return GlowBehavior::Irregular;
        return GlowBehavior::Static;
    }
}

bool TerrainVisualConfigManager::loadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json j;
        file >> j;

        // Load base colors
        if (j.contains("base_colors") && j["base_colors"].is_array()) {
            const auto& colors = j["base_colors"];
            for (std::size_t i = 0; i < std::min(colors.size(), TERRAIN_PALETTE_SIZE); ++i) {
                const auto& c = colors[i];
                float r = c.value("r", 0.0f);
                float g = c.value("g", 0.0f);
                float b = c.value("b", 0.0f);
                float a = c.value("a", 1.0f);
                m_config.setBaseColor(i, glm::vec4(r, g, b, a));
            }
        }

        // Load emissive colors
        if (j.contains("emissive_colors") && j["emissive_colors"].is_array()) {
            const auto& colors = j["emissive_colors"];
            for (std::size_t i = 0; i < std::min(colors.size(), TERRAIN_PALETTE_SIZE); ++i) {
                const auto& c = colors[i];
                float r = c.value("r", 0.0f);
                float g = c.value("g", 0.0f);
                float b = c.value("b", 0.0f);
                float intensity = c.value("intensity", 0.5f);
                m_config.setEmissiveColor(i, glm::vec3(r, g, b), intensity);
            }
        }

        // Load glow parameters
        if (j.contains("glow_params") && j["glow_params"].is_array()) {
            const auto& params = j["glow_params"];
            for (std::size_t i = 0; i < std::min(params.size(), TERRAIN_PALETTE_SIZE); ++i) {
                const auto& p = params[i];
                GlowParameters gp;
                gp.behavior = stringToGlowBehavior(p.value("behavior", "static"));
                gp.period = p.value("period", 0.0f);
                gp.amplitude = p.value("amplitude", 0.0f);
                gp.phase_offset = p.value("phase_offset", 0.0f);
                m_config.setGlowParameters(i, gp);
            }
        }

        // Load sea level
        if (j.contains("sea_level")) {
            m_config.setSeaLevel(j["sea_level"].get<float>());
        }

        markDirty();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool TerrainVisualConfigManager::saveToFile(const std::string& filepath) const {
    try {
        nlohmann::json j;

        // Save base colors
        nlohmann::json baseColors = nlohmann::json::array();
        for (std::size_t i = 0; i < TERRAIN_PALETTE_SIZE; ++i) {
            const auto& c = m_config.base_colors[i];
            baseColors.push_back({
                {"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}
            });
        }
        j["base_colors"] = baseColors;

        // Save emissive colors
        nlohmann::json emissiveColors = nlohmann::json::array();
        for (std::size_t i = 0; i < TERRAIN_PALETTE_SIZE; ++i) {
            const auto& c = m_config.emissive_colors[i];
            emissiveColors.push_back({
                {"r", c.r}, {"g", c.g}, {"b", c.b}, {"intensity", c.a}
            });
        }
        j["emissive_colors"] = emissiveColors;

        // Save glow parameters
        nlohmann::json glowParams = nlohmann::json::array();
        for (std::size_t i = 0; i < TERRAIN_PALETTE_SIZE; ++i) {
            const auto& p = m_config.glow_params[i];
            glowParams.push_back({
                {"behavior", glowBehaviorToString(p.behavior)},
                {"period", p.period},
                {"amplitude", p.amplitude},
                {"phase_offset", p.phase_offset}
            });
        }
        j["glow_params"] = glowParams;

        // Save sea level
        j["sea_level"] = m_config.sea_level;

        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << j.dump(2);
        return true;
    }
    catch (...) {
        return false;
    }
}

#else

// Stub implementations when JSON is not available
bool TerrainVisualConfigManager::loadFromFile(const std::string& /*filepath*/) {
    return false;
}

bool TerrainVisualConfigManager::saveToFile(const std::string& /*filepath*/) const {
    return false;
}

#endif // SIMS3000_HAS_JSON

} // namespace sims3000
