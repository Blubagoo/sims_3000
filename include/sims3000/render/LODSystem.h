/**
 * @file LODSystem.h
 * @brief Level-of-Detail (LOD) system for distance-based model selection.
 *
 * Provides LOD level selection based on camera distance to optimize rendering
 * on large maps (512x512 with 262k+ entities). Supports 2+ LOD levels per model
 * with configurable distance thresholds.
 *
 * Features:
 * - Distance-based LOD level selection
 * - Configurable distance thresholds per model type
 * - Support for 2+ LOD levels (extensible framework)
 * - Optional smooth crossfade to prevent pop-in (via blend alpha)
 * - Aggressive distance margins to hide transitions
 * - Per-frame LOD evaluation for entity batches
 *
 * LOD Levels (default):
 * - Level 0: Full detail (<50m from camera)
 * - Level 1: Simplified (50-150m)
 * - Level 2+: Further simplified or billboard (>150m, optional)
 *
 * Usage:
 * @code
 *   LODSystem lodSystem;
 *
 *   // Configure LOD for a model type
 *   LODConfig config;
 *   config.thresholds = {{50.0f, 150.0f}};  // 2 levels: <50m, 50-150m, >150m
 *   config.crossfadeRange = 5.0f;            // 5m crossfade zone
 *   lodSystem.setConfig(modelTypeId, config);
 *
 *   // Each frame: select LOD level for an entity
 *   float distance = computeCameraDistance(entityPos, cameraPos);
 *   LODResult result = lodSystem.selectLOD(modelTypeId, distance);
 *   // result.level = 0, 1, or 2
 *   // result.blendAlpha = 0.0-1.0 for crossfade
 * @endcode
 *
 * Resource ownership:
 * - LODSystem owns LODConfig data (pure data, no GPU resources)
 * - No GPU resources (pure CPU LOD selection)
 */

#ifndef SIMS3000_RENDER_LOD_SYSTEM_H
#define SIMS3000_RENDER_LOD_SYSTEM_H

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <limits>

namespace sims3000 {

// ============================================================================
// LOD Configuration Constants
// ============================================================================

/**
 * @namespace LODConfig
 * @brief Default configuration constants for LOD system.
 */
namespace LODDefaults {
    /// Default distance threshold for LOD 0 -> LOD 1 transition
    constexpr float LOD0_TO_LOD1_DISTANCE = 50.0f;

    /// Default distance threshold for LOD 1 -> LOD 2 transition
    constexpr float LOD1_TO_LOD2_DISTANCE = 150.0f;

    /// Default crossfade range (distance over which blend occurs)
    constexpr float CROSSFADE_RANGE = 5.0f;

    /// Minimum LOD levels supported
    constexpr std::uint8_t MIN_LOD_LEVELS = 2;

    /// Maximum LOD levels supported
    constexpr std::uint8_t MAX_LOD_LEVELS = 8;

    /// Invalid LOD level marker
    constexpr std::uint8_t INVALID_LOD_LEVEL = 255;
}

// ============================================================================
// LOD Transition Mode
// ============================================================================

/**
 * @enum LODTransitionMode
 * @brief How LOD transitions are handled visually.
 */
enum class LODTransitionMode : std::uint8_t {
    Instant = 0,    ///< Immediate switch (may cause pop-in)
    Crossfade = 1,  ///< Smooth alpha blend between levels
    Aggressive = 2  ///< Use aggressive distance margins to hide transitions
};

// ============================================================================
// LOD Configuration
// ============================================================================

/**
 * @struct LODThreshold
 * @brief Single LOD transition threshold.
 *
 * Defines the distance at which to transition from one LOD level to the next.
 */
struct LODThreshold {
    float distance = 0.0f;  ///< Distance at which this level ends
    float hysteresis = 2.0f; ///< Hysteresis margin to prevent rapid switching

    LODThreshold() = default;
    LODThreshold(float dist, float hyst = 2.0f)
        : distance(dist), hysteresis(hyst) {}
};

/**
 * @struct LODConfig
 * @brief Configuration for LOD levels of a model type.
 *
 * Contains distance thresholds and transition settings for LOD selection.
 * The number of thresholds determines the number of LOD levels (N thresholds = N+1 levels).
 */
struct LODConfig {
    /// Distance thresholds for LOD transitions (sorted ascending)
    /// N thresholds define N+1 LOD levels
    /// e.g., {50.0, 150.0} defines 3 levels: <50m, 50-150m, >150m
    std::vector<LODThreshold> thresholds;

    /// Transition mode for visual quality
    LODTransitionMode transitionMode = LODTransitionMode::Aggressive;

    /// Crossfade range in world units (only used with Crossfade mode)
    float crossfadeRange = LODDefaults::CROSSFADE_RANGE;

    /// Enable LOD selection (false = always use LOD 0)
    bool enabled = true;

    /**
     * @brief Get the number of LOD levels defined by this config.
     * @return Number of LOD levels (thresholds.size() + 1).
     */
    std::uint8_t getLevelCount() const {
        return static_cast<std::uint8_t>(thresholds.size() + 1);
    }

    /**
     * @brief Check if config is valid.
     * @return true if thresholds are properly ordered and count is valid.
     */
    bool isValid() const {
        if (thresholds.empty()) return true;  // 1 level is valid

        // Check ascending order
        for (std::size_t i = 1; i < thresholds.size(); ++i) {
            if (thresholds[i].distance <= thresholds[i - 1].distance) {
                return false;
            }
        }

        // Check max levels
        if (thresholds.size() + 1 > LODDefaults::MAX_LOD_LEVELS) {
            return false;
        }

        return true;
    }

    /**
     * @brief Create a default 2-level LOD config.
     * @return Config with LOD 0 (<50m) and LOD 1 (>50m).
     */
    static LODConfig createDefault2Level() {
        LODConfig config;
        config.thresholds.push_back({LODDefaults::LOD0_TO_LOD1_DISTANCE, 2.0f});
        return config;
    }

    /**
     * @brief Create a default 3-level LOD config.
     * @return Config with LOD 0 (<50m), LOD 1 (50-150m), LOD 2 (>150m).
     */
    static LODConfig createDefault3Level() {
        LODConfig config;
        config.thresholds.push_back({LODDefaults::LOD0_TO_LOD1_DISTANCE, 2.0f});
        config.thresholds.push_back({LODDefaults::LOD1_TO_LOD2_DISTANCE, 5.0f});
        return config;
    }
};

// ============================================================================
// LOD Selection Result
// ============================================================================

/**
 * @struct LODResult
 * @brief Result of LOD level selection for an entity.
 *
 * Contains the selected LOD level and optional blend parameters
 * for smooth transitions.
 */
struct LODResult {
    /// Selected LOD level (0 = highest detail)
    std::uint8_t level = 0;

    /// Blend alpha for crossfade (0.0 = fully current, 1.0 = fully next)
    float blendAlpha = 0.0f;

    /// Next LOD level for crossfade (same as level if not blending)
    std::uint8_t nextLevel = 0;

    /// Whether currently in a crossfade transition
    bool isBlending = false;

    /**
     * @brief Check if this result indicates the entity should be rendered.
     *
     * Always returns true since all LOD levels are renderable.
     * Could be extended to return false for cull-LOD levels.
     *
     * @return true if entity should be rendered.
     */
    bool shouldRender() const { return true; }

    /**
     * @brief Get the primary LOD level for single-pass rendering.
     * @return Primary LOD level.
     */
    std::uint8_t getPrimaryLevel() const { return level; }
};

// ============================================================================
// LOD Statistics
// ============================================================================

/**
 * @struct LODStats
 * @brief Statistics about LOD selection for the current frame.
 */
struct LODStats {
    /// Count of entities at each LOD level
    std::uint32_t levelCounts[LODDefaults::MAX_LOD_LEVELS] = {};

    /// Total entities evaluated
    std::uint32_t totalEvaluated = 0;

    /// Entities currently crossfading
    std::uint32_t crossfadingCount = 0;

    void reset() {
        for (auto& count : levelCounts) count = 0;
        totalEvaluated = 0;
        crossfadingCount = 0;
    }

    void recordSelection(const LODResult& result) {
        if (result.level < LODDefaults::MAX_LOD_LEVELS) {
            levelCounts[result.level]++;
        }
        totalEvaluated++;
        if (result.isBlending) crossfadingCount++;
    }
};

// ============================================================================
// LOD System Class
// ============================================================================

/**
 * @class LODSystem
 * @brief Central system for LOD level selection based on camera distance.
 *
 * Manages per-model-type LOD configurations and provides efficient
 * LOD level selection for large numbers of entities.
 */
class LODSystem {
public:
    /**
     * @brief Construct LOD system with default settings.
     */
    LODSystem() = default;
    ~LODSystem() = default;

    // Non-copyable (owns configuration data)
    LODSystem(const LODSystem&) = delete;
    LODSystem& operator=(const LODSystem&) = delete;

    // Moveable
    LODSystem(LODSystem&&) = default;
    LODSystem& operator=(LODSystem&&) = default;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set LOD configuration for a model type.
     *
     * @param modelTypeId Unique identifier for the model type.
     * @param config LOD configuration.
     * @return true if config was set successfully, false if invalid.
     */
    bool setConfig(std::uint64_t modelTypeId, const LODConfig& config);

    /**
     * @brief Get LOD configuration for a model type.
     *
     * @param modelTypeId Model type identifier.
     * @return Pointer to config, or nullptr if not configured.
     */
    const LODConfig* getConfig(std::uint64_t modelTypeId) const;

    /**
     * @brief Remove LOD configuration for a model type.
     *
     * @param modelTypeId Model type identifier.
     */
    void removeConfig(std::uint64_t modelTypeId);

    /**
     * @brief Clear all LOD configurations.
     */
    void clearConfigs();

    /**
     * @brief Set the default LOD configuration for unconfigured models.
     *
     * @param config Default configuration to use.
     */
    void setDefaultConfig(const LODConfig& config);

    /**
     * @brief Get the default LOD configuration.
     * @return Reference to default configuration.
     */
    const LODConfig& getDefaultConfig() const { return m_defaultConfig; }

    // =========================================================================
    // LOD Selection
    // =========================================================================

    /**
     * @brief Select LOD level for an entity based on camera distance.
     *
     * @param modelTypeId Model type identifier.
     * @param distance Distance from camera to entity in world units.
     * @return LOD selection result.
     */
    LODResult selectLOD(std::uint64_t modelTypeId, float distance) const;

    /**
     * @brief Select LOD level using default config.
     *
     * @param distance Distance from camera to entity in world units.
     * @return LOD selection result.
     */
    LODResult selectLODDefault(float distance) const;

    /**
     * @brief Compute distance from camera to a world position.
     *
     * @param worldPos Entity world position.
     * @param cameraPos Camera world position.
     * @return Euclidean distance.
     */
    static float computeDistance(const glm::vec3& worldPos,
                                  const glm::vec3& cameraPos);

    /**
     * @brief Select LOD for an entity given world positions.
     *
     * Convenience method that computes distance and selects LOD.
     *
     * @param modelTypeId Model type identifier.
     * @param worldPos Entity world position.
     * @param cameraPos Camera world position.
     * @return LOD selection result.
     */
    LODResult selectLODForPosition(std::uint64_t modelTypeId,
                                    const glm::vec3& worldPos,
                                    const glm::vec3& cameraPos) const;

    // =========================================================================
    // Batch Operations
    // =========================================================================

    /**
     * @brief Begin a new frame for LOD evaluation.
     *
     * Resets per-frame statistics.
     */
    void beginFrame();

    /**
     * @brief Get statistics for the current frame.
     * @return Reference to statistics.
     */
    const LODStats& getStats() const { return m_stats; }

    /**
     * @brief Record a LOD selection in statistics.
     *
     * @param result LOD result to record.
     */
    void recordSelection(const LODResult& result);

    // =========================================================================
    // Hysteresis Management
    // =========================================================================

    /**
     * @brief Track hysteresis state for an entity.
     *
     * Used to prevent rapid LOD switching when entity is near threshold.
     *
     * @param entityId Entity identifier.
     * @param currentLevel Current LOD level.
     */
    void updateHysteresis(std::uint32_t entityId, std::uint8_t currentLevel);

    /**
     * @brief Get last LOD level for an entity (for hysteresis).
     *
     * @param entityId Entity identifier.
     * @return Last LOD level, or INVALID_LOD_LEVEL if not tracked.
     */
    std::uint8_t getLastLevel(std::uint32_t entityId) const;

    /**
     * @brief Clear all hysteresis tracking data.
     */
    void clearHysteresis();

private:
    /**
     * @brief Select LOD level using a specific config.
     *
     * @param config LOD configuration.
     * @param distance Distance from camera.
     * @param lastLevel Last LOD level for hysteresis (use INVALID for none).
     * @return LOD selection result.
     */
    LODResult selectLODInternal(const LODConfig& config,
                                 float distance,
                                 std::uint8_t lastLevel) const;

    /// Per-model-type configurations
    std::unordered_map<std::uint64_t, LODConfig> m_configs;

    /// Default configuration for unconfigured models
    LODConfig m_defaultConfig = LODConfig::createDefault2Level();

    /// Per-entity last LOD level for hysteresis
    mutable std::unordered_map<std::uint32_t, std::uint8_t> m_hysteresisState;

    /// Per-frame statistics
    mutable LODStats m_stats;
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief Compute squared distance (faster than sqrt for comparisons).
 *
 * @param worldPos Entity world position.
 * @param cameraPos Camera world position.
 * @return Squared Euclidean distance.
 */
inline float computeDistanceSquared(const glm::vec3& worldPos,
                                    const glm::vec3& cameraPos) {
    glm::vec3 delta = worldPos - cameraPos;
    return glm::dot(delta, delta);
}

/**
 * @brief Convert LOD level to a debug color for visualization.
 *
 * @param level LOD level (0-7).
 * @return Debug color (green=LOD0, yellow=LOD1, orange=LOD2, red=higher).
 */
inline glm::vec4 getLODDebugColor(std::uint8_t level) {
    switch (level) {
        case 0: return glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Green
        case 1: return glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
        case 2: return glm::vec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange
        case 3: return glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);  // Red
        default: return glm::vec4(0.5f, 0.0f, 0.5f, 1.0f); // Purple
    }
}

} // namespace sims3000

#endif // SIMS3000_RENDER_LOD_SYSTEM_H
