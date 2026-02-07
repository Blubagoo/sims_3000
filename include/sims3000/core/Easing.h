/**
 * @file Easing.h
 * @brief Easing functions for smooth animations.
 *
 * Provides common easing functions used for camera transitions,
 * UI animations, and other interpolation needs.
 *
 * All functions take t in [0,1] and return a value in [0,1].
 * Values outside [0,1] are clamped.
 *
 * Resource ownership: None (pure functions, no state).
 */

#ifndef SIMS3000_CORE_EASING_H
#define SIMS3000_CORE_EASING_H

#include <cmath>
#include <algorithm>

namespace sims3000 {

/**
 * @namespace Easing
 * @brief Collection of easing functions for interpolation.
 *
 * Usage:
 * @code
 *   float t = elapsed / duration;  // Raw progress 0-1
 *   float eased = Easing::easeInOutCubic(t);
 *   float value = start + (end - start) * eased;
 * @endcode
 */
namespace Easing {

/**
 * @brief Clamp value to [0, 1] range.
 * @param t Input value.
 * @return Clamped value.
 */
inline float clamp01(float t) {
    return std::clamp(t, 0.0f, 1.0f);
}

// ============================================================================
// Linear
// ============================================================================

/**
 * @brief Linear interpolation (no easing).
 *
 * f(t) = t
 *
 * @param t Progress from 0 to 1.
 * @return Eased value (same as input).
 */
inline float linear(float t) {
    return clamp01(t);
}

// ============================================================================
// Quadratic
// ============================================================================

/**
 * @brief Ease in quadratic - starts slow, accelerates.
 *
 * f(t) = t^2
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInQuad(float t) {
    t = clamp01(t);
    return t * t;
}

/**
 * @brief Ease out quadratic - starts fast, decelerates.
 *
 * f(t) = 1 - (1-t)^2
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeOutQuad(float t) {
    t = clamp01(t);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

/**
 * @brief Ease in-out quadratic - slow start and end.
 *
 * Combines ease-in for first half and ease-out for second half.
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInOutQuad(float t) {
    t = clamp01(t);
    if (t < 0.5f) {
        return 2.0f * t * t;
    } else {
        float p = -2.0f * t + 2.0f;
        return 1.0f - (p * p) / 2.0f;
    }
}

// ============================================================================
// Cubic
// ============================================================================

/**
 * @brief Ease in cubic - starts slow, accelerates.
 *
 * f(t) = t^3
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInCubic(float t) {
    t = clamp01(t);
    return t * t * t;
}

/**
 * @brief Ease out cubic - starts fast, decelerates.
 *
 * f(t) = 1 - (1-t)^3
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeOutCubic(float t) {
    t = clamp01(t);
    float p = 1.0f - t;
    return 1.0f - p * p * p;
}

/**
 * @brief Ease in-out cubic - slow start and end.
 *
 * Combines ease-in for first half and ease-out for second half.
 * This is the most commonly used easing for smooth transitions.
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInOutCubic(float t) {
    t = clamp01(t);
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float p = -2.0f * t + 2.0f;
        return 1.0f - (p * p * p) / 2.0f;
    }
}

// ============================================================================
// Sine
// ============================================================================

/**
 * @brief Ease in sine - gentle start.
 *
 * f(t) = 1 - cos(t * PI/2)
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInSine(float t) {
    t = clamp01(t);
    constexpr float PI_OVER_2 = 1.5707963267948966f;
    return 1.0f - std::cos(t * PI_OVER_2);
}

/**
 * @brief Ease out sine - gentle end.
 *
 * f(t) = sin(t * PI/2)
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeOutSine(float t) {
    t = clamp01(t);
    constexpr float PI_OVER_2 = 1.5707963267948966f;
    return std::sin(t * PI_OVER_2);
}

/**
 * @brief Ease in-out sine - gentle start and end.
 *
 * f(t) = -(cos(t * PI) - 1) / 2
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInOutSine(float t) {
    t = clamp01(t);
    constexpr float PI = 3.14159265358979323846f;
    return -(std::cos(PI * t) - 1.0f) / 2.0f;
}

// ============================================================================
// Exponential
// ============================================================================

/**
 * @brief Ease in exponential - very slow start, fast end.
 *
 * f(t) = 2^(10 * (t - 1))
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInExpo(float t) {
    t = clamp01(t);
    if (t <= 0.0f) return 0.0f;
    return std::pow(2.0f, 10.0f * (t - 1.0f));
}

/**
 * @brief Ease out exponential - fast start, very slow end.
 *
 * f(t) = 1 - 2^(-10 * t)
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeOutExpo(float t) {
    t = clamp01(t);
    if (t >= 1.0f) return 1.0f;
    return 1.0f - std::pow(2.0f, -10.0f * t);
}

/**
 * @brief Ease in-out exponential - very smooth at edges.
 *
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float easeInOutExpo(float t) {
    t = clamp01(t);
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    if (t < 0.5f) {
        return std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f;
    } else {
        return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
    }
}

// ============================================================================
// Easing Function Type
// ============================================================================

/**
 * @enum EasingType
 * @brief Enumeration of available easing functions.
 */
enum class EasingType {
    Linear,
    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,
    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,
    EaseInSine,
    EaseOutSine,
    EaseInOutSine,
    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo
};

/**
 * @brief Apply easing function by type.
 *
 * Convenience function to select easing at runtime.
 *
 * @param type The easing function to apply.
 * @param t Progress from 0 to 1.
 * @return Eased value.
 */
inline float applyEasing(EasingType type, float t) {
    switch (type) {
        case EasingType::Linear:          return linear(t);
        case EasingType::EaseInQuad:      return easeInQuad(t);
        case EasingType::EaseOutQuad:     return easeOutQuad(t);
        case EasingType::EaseInOutQuad:   return easeInOutQuad(t);
        case EasingType::EaseInCubic:     return easeInCubic(t);
        case EasingType::EaseOutCubic:    return easeOutCubic(t);
        case EasingType::EaseInOutCubic:  return easeInOutCubic(t);
        case EasingType::EaseInSine:      return easeInSine(t);
        case EasingType::EaseOutSine:     return easeOutSine(t);
        case EasingType::EaseInOutSine:   return easeInOutSine(t);
        case EasingType::EaseInExpo:      return easeInExpo(t);
        case EasingType::EaseOutExpo:     return easeOutExpo(t);
        case EasingType::EaseInOutExpo:   return easeInOutExpo(t);
        default:                          return linear(t);
    }
}

} // namespace Easing

} // namespace sims3000

#endif // SIMS3000_CORE_EASING_H
