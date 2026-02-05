/**
 * @file FrameStats.cpp
 * @brief FrameStats implementation.
 */

#include "sims3000/app/FrameStats.h"
#include <algorithm>

namespace sims3000 {

FrameStats::FrameStats() {
    reset();
}

void FrameStats::update(float deltaSeconds) {
    float frameTimeMs = deltaSeconds * 1000.0f;

    // Store in circular buffer
    m_frameTimes[m_frameIndex] = frameTimeMs;
    m_frameIndex = (m_frameIndex + 1) % SAMPLE_COUNT;
    m_totalFrames++;

    // Calculate statistics
    float sum = 0.0f;
    float minVal = 1000.0f;
    float maxVal = 0.0f;

    int sampleCount = std::min(static_cast<int>(m_totalFrames), SAMPLE_COUNT);
    for (int i = 0; i < sampleCount; ++i) {
        float t = m_frameTimes[i];
        sum += t;
        minVal = std::min(minVal, t);
        maxVal = std::max(maxVal, t);
    }

    m_avgFrameTime = sum / static_cast<float>(sampleCount);
    m_minFrameTime = minVal;
    m_maxFrameTime = maxVal;
    m_fps = (m_avgFrameTime > 0.0f) ? (1000.0f / m_avgFrameTime) : 0.0f;
}

float FrameStats::getFPS() const {
    return m_fps;
}

float FrameStats::getFrameTimeMs() const {
    return m_avgFrameTime;
}

float FrameStats::getMinFrameTimeMs() const {
    return m_minFrameTime;
}

float FrameStats::getMaxFrameTimeMs() const {
    return m_maxFrameTime;
}

std::uint64_t FrameStats::getTotalFrames() const {
    return m_totalFrames;
}

void FrameStats::recordTickTime(float tickTimeMs) {
    m_tickTimes[m_tickIndex] = tickTimeMs;
    m_tickIndex = (m_tickIndex + 1) % SAMPLE_COUNT;

    // Calculate average tick time
    float sum = 0.0f;
    int count = 0;
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        if (m_tickTimes[i] > 0.0f) {
            sum += m_tickTimes[i];
            count++;
        }
    }
    m_avgTickTime = (count > 0) ? (sum / static_cast<float>(count)) : 0.0f;
}

float FrameStats::getAvgTickTimeMs() const {
    return m_avgTickTime;
}

void FrameStats::reset() {
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        m_frameTimes[i] = 0.0f;
        m_tickTimes[i] = 0.0f;
    }
    m_frameIndex = 0;
    m_tickIndex = 0;
    m_totalFrames = 0;
    m_fps = 0.0f;
    m_avgFrameTime = 0.0f;
    m_minFrameTime = 1000.0f;
    m_maxFrameTime = 0.0f;
    m_avgTickTime = 0.0f;
}

} // namespace sims3000
