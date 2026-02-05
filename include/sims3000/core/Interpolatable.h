/**
 * @file Interpolatable.h
 * @brief Template wrapper for smooth interpolation between simulation ticks.
 *
 * Provides double-buffered state storage for values that need to be
 * interpolated between the fixed 20Hz simulation rate and variable
 * framerate rendering (typically 60fps).
 *
 * @see Ticket 0-007: Interpolation Pattern for Smooth Rendering
 */

#ifndef SIMS3000_CORE_INTERPOLATABLE_H
#define SIMS3000_CORE_INTERPOLATABLE_H

#include <type_traits>
#include <glm/glm.hpp>

namespace sims3000 {

/**
 * @class Interpolatable
 * @brief Double-buffered value wrapper for smooth visual interpolation.
 *
 * This template stores both the previous and current simulation state
 * for a value, allowing the renderer to interpolate between them for
 * smooth visuals between discrete simulation ticks.
 *
 * Usage Pattern:
 * 1. Before each simulation tick, call rotateTick() to shift current -> previous
 * 2. During simulation, update via set() or direct assignment
 * 3. During rendering, call lerp(alpha) where alpha is from SimulationClock::getInterpolation()
 *
 * @tparam T Value type. Must support:
 *   - Copy construction and assignment
 *   - For lerp(): glm::mix() must work, OR specialize lerp_impl
 *
 * Example:
 * @code
 *     Interpolatable<glm::vec3> position(glm::vec3(0.0f));
 *
 *     // Each tick
 *     position.rotateTick();
 *     position.set(newPosition);
 *
 *     // Each frame
 *     float alpha = clock.getInterpolation();
 *     glm::vec3 renderPos = position.lerp(alpha);
 * @endcode
 */
template <typename T>
class Interpolatable {
public:
    /**
     * Default constructor. Both previous and current are default-constructed.
     */
    Interpolatable() = default;

    /**
     * Construct with an initial value for both previous and current.
     * @param initial Initial value
     */
    explicit Interpolatable(const T& initial)
        : m_previous(initial)
        , m_current(initial)
    {
    }

    /**
     * Construct with explicit previous and current values.
     * @param previous Previous tick value
     * @param current Current tick value
     */
    Interpolatable(const T& previous, const T& current)
        : m_previous(previous)
        , m_current(current)
    {
    }

    /**
     * Rotate buffers: current becomes previous.
     * Call this at the START of each simulation tick, BEFORE updating current.
     */
    void rotateTick() {
        m_previous = m_current;
    }

    /**
     * Set the current value.
     * @param value New current value
     */
    void set(const T& value) {
        m_current = value;
    }

    /**
     * Set both previous and current to the same value.
     * Use when teleporting or initializing to avoid interpolation artifacts.
     * @param value Value for both slots
     */
    void setBoth(const T& value) {
        m_previous = value;
        m_current = value;
    }

    /**
     * Get the current (most recent) value.
     * @return Reference to current value
     */
    const T& current() const {
        return m_current;
    }

    /**
     * Get the previous tick's value.
     * @return Reference to previous value
     */
    const T& previous() const {
        return m_previous;
    }

    /**
     * Get mutable reference to current value for direct modification.
     * @return Mutable reference to current value
     */
    T& current() {
        return m_current;
    }

    /**
     * Linear interpolation between previous and current.
     *
     * @param alpha Interpolation factor from SimulationClock::getInterpolation()
     *              0.0 = previous value, 1.0 = current value
     * @return Interpolated value
     */
    T lerp(float alpha) const {
        return lerp_impl(m_previous, m_current, alpha);
    }

    /**
     * Assignment operator sets the current value.
     * @param value New current value
     * @return Reference to this
     */
    Interpolatable& operator=(const T& value) {
        m_current = value;
        return *this;
    }

    /**
     * Implicit conversion to current value for convenience.
     * @return Current value
     */
    operator const T&() const {
        return m_current;
    }

private:
    T m_previous{};
    T m_current{};

    /**
     * Internal lerp implementation using glm::mix.
     * Works for float, glm::vec2, glm::vec3, glm::vec4, glm::quat, etc.
     */
    template <typename U = T>
    static auto lerp_impl(const U& a, const U& b, float t)
        -> std::enable_if_t<!std::is_integral_v<U>, U>
    {
        return glm::mix(a, b, t);
    }

    /**
     * Integral type specialization: no interpolation, return current.
     * Integer values (like grid coordinates) should not be interpolated
     * between discrete values - use the current value instead.
     */
    template <typename U = T>
    static auto lerp_impl(const U& /*a*/, const U& b, float /*t*/)
        -> std::enable_if_t<std::is_integral_v<U>, U>
    {
        return b;  // Integers snap to current value
    }
};

// ============================================================================
// Convenience type aliases
// ============================================================================

/// Interpolatable float value
using InterpolatableFloat = Interpolatable<float>;

/// Interpolatable 2D vector
using InterpolatableVec2 = Interpolatable<glm::vec2>;

/// Interpolatable 3D vector (common for positions)
using InterpolatableVec3 = Interpolatable<glm::vec3>;

/// Interpolatable 4D vector
using InterpolatableVec4 = Interpolatable<glm::vec4>;

// ============================================================================
// Free function helpers
// ============================================================================

/**
 * @brief Linear interpolation helper for common types.
 *
 * Convenience free function that works with any type supported by glm::mix.
 *
 * @tparam T Value type
 * @param a Start value (alpha = 0)
 * @param b End value (alpha = 1)
 * @param alpha Interpolation factor [0.0, 1.0]
 * @return Interpolated value
 */
template <typename T>
inline T lerpValue(const T& a, const T& b, float alpha) {
    return glm::mix(a, b, alpha);
}

/**
 * @brief Clamp alpha to valid range [0.0, 1.0].
 * @param alpha Raw alpha value
 * @return Clamped alpha
 */
inline float clampAlpha(float alpha) {
    return glm::clamp(alpha, 0.0f, 1.0f);
}

} // namespace sims3000

#endif // SIMS3000_CORE_INTERPOLATABLE_H
