/**
 * @file ToonShaderConfig.cpp
 * @brief Implementation of runtime-configurable toon shader parameters.
 */

#include "sims3000/render/ToonShaderConfig.h"
#include <algorithm>
#include <cmath>

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

} // namespace sims3000
