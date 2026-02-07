/**
 * @file CameraState.h
 * @brief Camera state structure for free camera with isometric presets.
 *
 * Defines CameraState for orbital camera with focus_point, distance, pitch, yaw.
 * Supports full free orbit/tilt and four isometric preset snap views.
 *
 * Resource ownership: None (pure data struct, no GPU/SDL resources).
 */

#ifndef SIMS3000_RENDER_CAMERASTATE_H
#define SIMS3000_RENDER_CAMERASTATE_H

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cstdint>

namespace sims3000 {

// ============================================================================
// Camera Configuration Constants
// ============================================================================

/**
 * @brief Camera configuration parameters.
 *
 * All camera-related constants centralized here to avoid magic numbers.
 * Adjust these values to tune camera behavior.
 */
namespace CameraConfig {
    // Pitch constraints (degrees)
    constexpr float PITCH_MIN = 15.0f;           ///< Minimum pitch angle (shallow view)
    constexpr float PITCH_MAX = 80.0f;           ///< Maximum pitch angle (top-down view)

    // Distance/zoom constraints (world units) - base values
    constexpr float DISTANCE_MIN = 5.0f;         ///< Minimum camera distance (closest zoom)
    constexpr float DISTANCE_MAX = 100.0f;       ///< Maximum camera distance (furthest zoom) - small maps
    constexpr float DISTANCE_DEFAULT = 50.0f;    ///< Default camera distance

    // Map-size-aware maximum distances (Ticket 2-024)
    constexpr float DISTANCE_MAX_SMALL = 100.0f;   ///< Max distance for 128x128 maps
    constexpr float DISTANCE_MAX_MEDIUM = 150.0f;  ///< Max distance for 256x256 maps
    constexpr float DISTANCE_MAX_LARGE = 250.0f;   ///< Max distance for 512x512 maps

    /**
     * @brief Get maximum camera distance for a given map size.
     * @param mapSize Map dimension (128, 256, or 512).
     * @return Maximum camera distance appropriate for the map size.
     */
    constexpr float getMaxDistanceForMapSize(int mapSize) {
        if (mapSize <= 128) return DISTANCE_MAX_SMALL;
        if (mapSize <= 256) return DISTANCE_MAX_MEDIUM;
        return DISTANCE_MAX_LARGE;
    }

    // Isometric preset pitch (arctan(1/sqrt(2)) in degrees)
    // This is the "true isometric" angle (~35.264 degrees)
    constexpr float ISOMETRIC_PITCH = 35.264f;

    // Isometric preset yaw angles (degrees)
    constexpr float PRESET_N_YAW = 45.0f;        ///< North preset: looking NE
    constexpr float PRESET_E_YAW = 135.0f;       ///< East preset: looking SE
    constexpr float PRESET_S_YAW = 225.0f;       ///< South preset: looking SW
    constexpr float PRESET_W_YAW = 315.0f;       ///< West preset: looking NW

    // Animation defaults
    constexpr float TRANSITION_DURATION_SEC = 0.5f;  ///< Default transition duration

    // Yaw wrap boundaries
    constexpr float YAW_MIN = 0.0f;
    constexpr float YAW_MAX = 360.0f;

    // Field of View configuration (degrees)
    // 35 degrees provides minimal foreshortening at isometric pitch (~35.264 degrees)
    constexpr float FOV_DEFAULT = 35.0f;             ///< Default vertical FOV
    constexpr float FOV_MIN = 20.0f;                 ///< Minimum vertical FOV
    constexpr float FOV_MAX = 90.0f;                 ///< Maximum vertical FOV

    // Projection plane configuration
    constexpr float NEAR_PLANE = 0.1f;               ///< Near clipping plane distance
    constexpr float FAR_PLANE = 1000.0f;             ///< Far clipping plane distance
}

// ============================================================================
// Camera Mode Enum
// ============================================================================

/**
 * @enum CameraMode
 * @brief Camera operating mode.
 *
 * The camera can be in free mode (full orbit/pan/zoom/tilt),
 * one of four isometric preset positions, or animating between modes.
 */
enum class CameraMode : std::uint8_t {
    Free = 0,       ///< Full orbit/pan/zoom/tilt control
    Preset_N = 1,   ///< North isometric preset (yaw 45, pitch ~35.264)
    Preset_E = 2,   ///< East isometric preset (yaw 135, pitch ~35.264)
    Preset_S = 3,   ///< South isometric preset (yaw 225, pitch ~35.264)
    Preset_W = 4,   ///< West isometric preset (yaw 315, pitch ~35.264)
    Animating = 5   ///< Transitioning between modes/presets
};

// ============================================================================
// Transition State
// ============================================================================

/**
 * @struct TransitionState
 * @brief State for smooth animated transitions between camera modes.
 *
 * When switching between presets or modes, the camera interpolates
 * smoothly over a configurable duration.
 */
struct TransitionState {
    bool active = false;                ///< Is a transition currently in progress?

    // Source state (start of transition)
    glm::vec3 start_focus_point{0.0f};  ///< Focus point at transition start
    float start_distance = 0.0f;         ///< Distance at transition start
    float start_pitch = 0.0f;            ///< Pitch at transition start
    float start_yaw = 0.0f;              ///< Yaw at transition start

    // Target state (end of transition)
    glm::vec3 target_focus_point{0.0f}; ///< Target focus point
    float target_distance = 0.0f;        ///< Target distance
    float target_pitch = 0.0f;           ///< Target pitch
    float target_yaw = 0.0f;             ///< Target yaw

    // Animation progress
    float elapsed_time = 0.0f;           ///< Time elapsed in transition (seconds)
    float duration = CameraConfig::TRANSITION_DURATION_SEC; ///< Total transition duration

    CameraMode target_mode = CameraMode::Free; ///< Mode to switch to after transition

    /**
     * @brief Get the interpolation alpha (0.0 to 1.0).
     * @return Normalized progress through the transition.
     */
    float getAlpha() const {
        if (duration <= 0.0f) return 1.0f;
        float alpha = elapsed_time / duration;
        return (alpha < 0.0f) ? 0.0f : (alpha > 1.0f) ? 1.0f : alpha;
    }

    /**
     * @brief Check if transition is complete.
     * @return true if elapsed_time >= duration.
     */
    bool isComplete() const {
        return elapsed_time >= duration;
    }

    /**
     * @brief Reset transition state.
     */
    void reset() {
        active = false;
        elapsed_time = 0.0f;
    }
};

// ============================================================================
// Camera State
// ============================================================================

/**
 * @struct CameraState
 * @brief Complete camera state for orbital camera with isometric presets.
 *
 * The camera orbits around a focus_point at a given distance.
 * Pitch controls vertical angle (clamped 15-80 degrees).
 * Yaw controls horizontal rotation (wraps 0-360 degrees).
 *
 * Default mode is Preset_N (north isometric view) per project requirements.
 */
struct CameraState {
    // Core orbital camera parameters
    glm::vec3 focus_point{0.0f, 0.0f, 0.0f};  ///< Point the camera orbits around
    float distance = CameraConfig::DISTANCE_DEFAULT; ///< Distance from focus point
    float pitch = CameraConfig::ISOMETRIC_PITCH;     ///< Vertical angle in degrees (clamped 15-80)
    float yaw = CameraConfig::PRESET_N_YAW;          ///< Horizontal angle in degrees (wraps 0-360)

    // Mode and transition
    CameraMode mode = CameraMode::Preset_N;          ///< Current camera mode
    TransitionState transition;                       ///< Smooth transition state

    // ========================================================================
    // Utility Methods
    // ========================================================================

    /**
     * @brief Clamp pitch to valid range.
     *
     * Enforces the pitch constraint: 15-80 degrees.
     */
    void clampPitch() {
        if (pitch < CameraConfig::PITCH_MIN) pitch = CameraConfig::PITCH_MIN;
        if (pitch > CameraConfig::PITCH_MAX) pitch = CameraConfig::PITCH_MAX;
    }

    /**
     * @brief Wrap yaw to valid range (0-360).
     *
     * Ensures yaw stays within 0-360 degrees with proper wrapping.
     */
    void wrapYaw() {
        while (yaw < CameraConfig::YAW_MIN) yaw += CameraConfig::YAW_MAX;
        while (yaw >= CameraConfig::YAW_MAX) yaw -= CameraConfig::YAW_MAX;
    }

    /**
     * @brief Clamp distance to valid range.
     *
     * Enforces zoom limits: 5-100 units.
     */
    void clampDistance() {
        if (distance < CameraConfig::DISTANCE_MIN) distance = CameraConfig::DISTANCE_MIN;
        if (distance > CameraConfig::DISTANCE_MAX) distance = CameraConfig::DISTANCE_MAX;
    }

    /**
     * @brief Apply all constraints (pitch, yaw, distance).
     */
    void applyConstraints() {
        clampPitch();
        wrapYaw();
        clampDistance();
    }

    /**
     * @brief Get pitch for a given preset mode.
     * @param presetMode The preset mode.
     * @return The pitch angle in degrees (isometric pitch for all presets).
     */
    static float getPitchForPreset(CameraMode presetMode) {
        switch (presetMode) {
            case CameraMode::Preset_N:
            case CameraMode::Preset_E:
            case CameraMode::Preset_S:
            case CameraMode::Preset_W:
                return CameraConfig::ISOMETRIC_PITCH;
            default:
                return CameraConfig::ISOMETRIC_PITCH; // Default to isometric
        }
    }

    /**
     * @brief Get yaw for a given preset mode.
     * @param presetMode The preset mode.
     * @return The yaw angle in degrees.
     */
    static float getYawForPreset(CameraMode presetMode) {
        switch (presetMode) {
            case CameraMode::Preset_N: return CameraConfig::PRESET_N_YAW;
            case CameraMode::Preset_E: return CameraConfig::PRESET_E_YAW;
            case CameraMode::Preset_S: return CameraConfig::PRESET_S_YAW;
            case CameraMode::Preset_W: return CameraConfig::PRESET_W_YAW;
            default: return CameraConfig::PRESET_N_YAW; // Default to north
        }
    }

    /**
     * @brief Check if the current mode is a preset (not Free or Animating).
     * @return true if in a preset mode.
     */
    bool isPresetMode() const {
        return mode == CameraMode::Preset_N ||
               mode == CameraMode::Preset_E ||
               mode == CameraMode::Preset_S ||
               mode == CameraMode::Preset_W;
    }

    /**
     * @brief Check if the camera is currently animating.
     * @return true if mode is Animating and transition is active.
     */
    bool isAnimating() const {
        return mode == CameraMode::Animating && transition.active;
    }

    /**
     * @brief Start a transition to a new mode.
     *
     * Captures current state as the start, sets target based on mode.
     *
     * @param targetMode The mode to transition to.
     * @param durationSec Transition duration in seconds.
     */
    void startTransition(CameraMode targetMode, float durationSec = CameraConfig::TRANSITION_DURATION_SEC) {
        // Capture current state as start
        transition.start_focus_point = focus_point;
        transition.start_distance = distance;
        transition.start_pitch = pitch;
        transition.start_yaw = yaw;

        // Set target based on mode
        transition.target_focus_point = focus_point; // Focus doesn't change for mode switch
        transition.target_distance = distance;       // Distance preserved

        if (targetMode == CameraMode::Free) {
            // Free mode: keep current angles
            transition.target_pitch = pitch;
            transition.target_yaw = yaw;
        } else if (targetMode != CameraMode::Animating) {
            // Preset mode: use preset angles
            transition.target_pitch = getPitchForPreset(targetMode);
            transition.target_yaw = getYawForPreset(targetMode);
        }

        transition.target_mode = targetMode;
        transition.duration = durationSec;
        transition.elapsed_time = 0.0f;
        transition.active = true;

        mode = CameraMode::Animating;
    }

    /**
     * @brief Reset to default state (Preset_N).
     */
    void resetToDefault() {
        focus_point = glm::vec3(0.0f);
        distance = CameraConfig::DISTANCE_DEFAULT;
        pitch = CameraConfig::ISOMETRIC_PITCH;
        yaw = CameraConfig::PRESET_N_YAW;
        mode = CameraMode::Preset_N;
        transition.reset();
    }
};

// Static assertions for ECS-friendly component size
static_assert(sizeof(CameraMode) == 1, "CameraMode must be 1 byte");

} // namespace sims3000

#endif // SIMS3000_RENDER_CAMERASTATE_H
