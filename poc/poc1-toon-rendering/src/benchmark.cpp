#include "benchmark.h"
#include <algorithm>
#include <numeric>
#include <limits>

namespace poc1 {

Benchmark::Benchmark()
    : m_performanceFrequency(SDL_GetPerformanceFrequency())
    , m_minFrameTimeMs(std::numeric_limits<float>::max())
    , m_maxFrameTimeMs(0.0f)
{
    // Initialize frame time buffer to zero
    m_frameTimes.fill(0.0f);
}

void Benchmark::StartFrame()
{
    m_frameStartCounter = SDL_GetPerformanceCounter();
}

void Benchmark::EndFrame()
{
    uint64_t frameEndCounter = SDL_GetPerformanceCounter();
    uint64_t elapsedTicks = frameEndCounter - m_frameStartCounter;

    // Convert to milliseconds
    // frameTimeMs = (elapsedTicks / frequency) * 1000
    m_currentFrameTimeMs = static_cast<float>(
        (static_cast<double>(elapsedTicks) / static_cast<double>(m_performanceFrequency)) * 1000.0
    );

    // Update min/max
    if (m_currentFrameTimeMs < m_minFrameTimeMs) {
        m_minFrameTimeMs = m_currentFrameTimeMs;
    }
    if (m_currentFrameTimeMs > m_maxFrameTimeMs) {
        m_maxFrameTimeMs = m_currentFrameTimeMs;
    }

    // Update rolling average
    UpdateRollingAverage(m_currentFrameTimeMs);

    ++m_totalFrames;
}

float Benchmark::GetFPS() const
{
    if (m_averageFrameTimeMs <= 0.0f) {
        return 0.0f;
    }
    return 1000.0f / m_averageFrameTimeMs;
}

void Benchmark::ResetMinMax()
{
    m_minFrameTimeMs = std::numeric_limits<float>::max();
    m_maxFrameTimeMs = 0.0f;
}

void Benchmark::UpdateRollingAverage(float frameTimeMs)
{
    m_frameTimes[m_frameTimeIndex] = frameTimeMs;
    m_frameTimeIndex = (m_frameTimeIndex + 1) % ROLLING_AVERAGE_SIZE;

    if (!m_bufferFilled && m_frameTimeIndex == 0) {
        m_bufferFilled = true;
    }

    // Calculate average
    size_t count = m_bufferFilled ? ROLLING_AVERAGE_SIZE : m_frameTimeIndex;
    if (count == 0) {
        count = 1; // Avoid division by zero on first frame
    }

    float sum = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum += m_frameTimes[i];
    }
    m_averageFrameTimeMs = sum / static_cast<float>(count);
}

void Benchmark::PrintReport() const
{
    SDL_Log("=== POC-1 Benchmark Report ===");
    SDL_Log("Frame Time: %.2f ms (avg), %.2f ms (min), %.2f ms (max)",
            m_averageFrameTimeMs,
            m_minFrameTimeMs == std::numeric_limits<float>::max() ? 0.0f : m_minFrameTimeMs,
            m_maxFrameTimeMs);
    SDL_Log("FPS: %.1f", GetFPS());
    SDL_Log("Draw Calls: %u", m_drawCalls);
    SDL_Log("Instances: %u", m_instanceCount);
    float memMB = static_cast<float>(m_gpuMemoryBytes) / (1024.0f * 1024.0f);
    SDL_Log("GPU Memory (est): %.2f MB", memMB);
    SDL_Log("==============================");
}

} // namespace poc1
