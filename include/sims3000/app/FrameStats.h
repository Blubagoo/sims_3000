/**
 * @file FrameStats.h
 * @brief Frame timing and performance statistics.
 */

#ifndef SIMS3000_APP_FRAMESTATS_H
#define SIMS3000_APP_FRAMESTATS_H

#include <cstdint>

namespace sims3000 {

/**
 * @class FrameStats
 * @brief Tracks frame timing and performance metrics.
 *
 * Provides FPS calculation, frame time tracking, and
 * statistics for debug overlay display.
 */
class FrameStats {
public:
    FrameStats();
    ~FrameStats() = default;

    /**
     * Update with the latest frame delta.
     * Call once per rendered frame.
     * @param deltaSeconds Time elapsed since last frame
     */
    void update(float deltaSeconds);

    /**
     * Get current frames per second.
     * Smoothed over the sample window.
     */
    float getFPS() const;

    /**
     * Get average frame time in milliseconds.
     */
    float getFrameTimeMs() const;

    /**
     * Get minimum frame time in the sample window.
     */
    float getMinFrameTimeMs() const;

    /**
     * Get maximum frame time in the sample window.
     */
    float getMaxFrameTimeMs() const;

    /**
     * Get total frames rendered.
     */
    std::uint64_t getTotalFrames() const;

    /**
     * Record simulation tick timing.
     * @param tickTimeMs Time spent on simulation tick
     */
    void recordTickTime(float tickTimeMs);

    /**
     * Get average tick time.
     */
    float getAvgTickTimeMs() const;

    /**
     * Reset all statistics.
     */
    void reset();

private:
    static constexpr int SAMPLE_COUNT = 60;

    float m_frameTimes[SAMPLE_COUNT] = {};
    float m_tickTimes[SAMPLE_COUNT] = {};
    int m_frameIndex = 0;
    int m_tickIndex = 0;
    std::uint64_t m_totalFrames = 0;
    float m_fps = 0.0f;
    float m_avgFrameTime = 0.0f;
    float m_minFrameTime = 1000.0f;
    float m_maxFrameTime = 0.0f;
    float m_avgTickTime = 0.0f;
};

} // namespace sims3000

#endif // SIMS3000_APP_FRAMESTATS_H
