/**
 * @file LODSystem.cpp
 * @brief Implementation of LOD system for distance-based model selection.
 */

#include "sims3000/render/LODSystem.h"

#include <cmath>
#include <algorithm>

namespace sims3000 {

// =============================================================================
// Configuration Management
// =============================================================================

bool LODSystem::setConfig(std::uint64_t modelTypeId, const LODConfig& config) {
    if (!config.isValid()) {
        return false;
    }

    m_configs[modelTypeId] = config;
    return true;
}

const LODConfig* LODSystem::getConfig(std::uint64_t modelTypeId) const {
    auto it = m_configs.find(modelTypeId);
    if (it != m_configs.end()) {
        return &it->second;
    }
    return nullptr;
}

void LODSystem::removeConfig(std::uint64_t modelTypeId) {
    m_configs.erase(modelTypeId);
}

void LODSystem::clearConfigs() {
    m_configs.clear();
}

void LODSystem::setDefaultConfig(const LODConfig& config) {
    if (config.isValid()) {
        m_defaultConfig = config;
    }
}

// =============================================================================
// LOD Selection
// =============================================================================

LODResult LODSystem::selectLOD(std::uint64_t modelTypeId, float distance) const {
    const LODConfig* config = getConfig(modelTypeId);
    if (config == nullptr) {
        config = &m_defaultConfig;
    }

    return selectLODInternal(*config, distance, LODDefaults::INVALID_LOD_LEVEL);
}

LODResult LODSystem::selectLODDefault(float distance) const {
    return selectLODInternal(m_defaultConfig, distance, LODDefaults::INVALID_LOD_LEVEL);
}

float LODSystem::computeDistance(const glm::vec3& worldPos,
                                  const glm::vec3& cameraPos) {
    return glm::length(worldPos - cameraPos);
}

LODResult LODSystem::selectLODForPosition(std::uint64_t modelTypeId,
                                           const glm::vec3& worldPos,
                                           const glm::vec3& cameraPos) const {
    float distance = computeDistance(worldPos, cameraPos);
    return selectLOD(modelTypeId, distance);
}

LODResult LODSystem::selectLODInternal(const LODConfig& config,
                                        float distance,
                                        std::uint8_t lastLevel) const {
    LODResult result;

    // If LOD disabled, always return level 0
    if (!config.enabled) {
        result.level = 0;
        result.nextLevel = 0;
        result.blendAlpha = 0.0f;
        result.isBlending = false;
        return result;
    }

    // No thresholds = single LOD level
    if (config.thresholds.empty()) {
        result.level = 0;
        result.nextLevel = 0;
        result.blendAlpha = 0.0f;
        result.isBlending = false;
        return result;
    }

    // Find the appropriate LOD level based on distance
    std::uint8_t selectedLevel = 0;
    float crossfadeStart = 0.0f;
    float crossfadeEnd = 0.0f;
    bool inCrossfadeZone = false;

    for (std::size_t i = 0; i < config.thresholds.size(); ++i) {
        const LODThreshold& threshold = config.thresholds[i];

        // Calculate effective threshold considering hysteresis
        float effectiveThreshold = threshold.distance;

        // Apply hysteresis if we have a previous level
        if (lastLevel != LODDefaults::INVALID_LOD_LEVEL) {
            // If moving to lower detail (level increasing), use threshold + hysteresis
            // If moving to higher detail (level decreasing), use threshold - hysteresis
            if (lastLevel <= i) {
                // Currently at or below this level, need to exceed threshold + hysteresis to increase
                effectiveThreshold = threshold.distance + threshold.hysteresis;
            } else {
                // Currently above this level, need to go below threshold - hysteresis to decrease
                effectiveThreshold = threshold.distance - threshold.hysteresis;
            }
        }

        if (distance < effectiveThreshold) {
            selectedLevel = static_cast<std::uint8_t>(i);

            // Check for crossfade zone
            if (config.transitionMode == LODTransitionMode::Crossfade) {
                crossfadeStart = threshold.distance - config.crossfadeRange;
                crossfadeEnd = threshold.distance;

                if (distance >= crossfadeStart && distance < crossfadeEnd) {
                    inCrossfadeZone = true;
                }
            }

            break;
        }

        // If we've passed all thresholds, use the highest LOD level
        selectedLevel = static_cast<std::uint8_t>(i + 1);
    }

    result.level = selectedLevel;
    result.nextLevel = selectedLevel;
    result.blendAlpha = 0.0f;
    result.isBlending = false;

    // Handle crossfade blending
    if (inCrossfadeZone && config.transitionMode == LODTransitionMode::Crossfade) {
        result.isBlending = true;
        result.nextLevel = selectedLevel + 1;

        // Calculate blend alpha (0.0 at crossfadeStart, 1.0 at crossfadeEnd)
        float range = crossfadeEnd - crossfadeStart;
        if (range > 0.0001f) {
            result.blendAlpha = (distance - crossfadeStart) / range;
            result.blendAlpha = std::max(0.0f, std::min(1.0f, result.blendAlpha));
        }
    }

    return result;
}

// =============================================================================
// Batch Operations
// =============================================================================

void LODSystem::beginFrame() {
    m_stats.reset();
}

void LODSystem::recordSelection(const LODResult& result) {
    m_stats.recordSelection(result);
}

// =============================================================================
// Hysteresis Management
// =============================================================================

void LODSystem::updateHysteresis(std::uint32_t entityId, std::uint8_t currentLevel) {
    m_hysteresisState[entityId] = currentLevel;
}

std::uint8_t LODSystem::getLastLevel(std::uint32_t entityId) const {
    auto it = m_hysteresisState.find(entityId);
    if (it != m_hysteresisState.end()) {
        return it->second;
    }
    return LODDefaults::INVALID_LOD_LEVEL;
}

void LODSystem::clearHysteresis() {
    m_hysteresisState.clear();
}

} // namespace sims3000
