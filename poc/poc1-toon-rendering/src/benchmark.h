#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>

namespace poc1 {

/**
 * Performance benchmarking utility for the rendering POC.
 * Tracks frame times, FPS, and draw call statistics.
 */
class Benchmark {
public:
    Benchmark();

    /**
     * Call at the start of each frame to begin timing.
     */
    void StartFrame();

    /**
     * Call at the end of each frame to complete timing.
     */
    void EndFrame();

    /**
     * Gets the time of the most recent frame in milliseconds.
     * @return Frame time in ms
     */
    float GetFrameTimeMs() const { return m_currentFrameTimeMs; }

    /**
     * Gets the rolling average frame time over the last 100 frames.
     * @return Average frame time in ms
     */
    float GetAverageFrameTimeMs() const { return m_averageFrameTimeMs; }

    /**
     * Gets the minimum frame time recorded.
     * @return Minimum frame time in ms
     */
    float GetMinFrameTimeMs() const { return m_minFrameTimeMs; }

    /**
     * Gets the maximum frame time recorded.
     * @return Maximum frame time in ms
     */
    float GetMaxFrameTimeMs() const { return m_maxFrameTimeMs; }

    /**
     * Gets the current frames per second.
     * @return FPS (based on average frame time)
     */
    float GetFPS() const;

    /**
     * Gets the number of draw calls this frame.
     * @return Draw call count
     */
    uint32_t GetDrawCalls() const { return m_drawCalls; }

    /**
     * Increments the draw call counter.
     */
    void IncrementDrawCalls() { ++m_drawCalls; }

    /**
     * Increments draw calls by a specific amount.
     * @param count Number of draw calls to add
     */
    void AddDrawCalls(uint32_t count) { m_drawCalls += count; }

    /**
     * Resets the draw call counter to zero.
     * Call this at the start of each frame.
     */
    void ResetDrawCalls() { m_drawCalls = 0; }

    /**
     * Sets the current instance count for reporting.
     * @param count Number of instances rendered this frame
     */
    void SetInstanceCount(uint32_t count) { m_instanceCount = count; }

    /**
     * Gets the instance count.
     * @return Number of instances
     */
    uint32_t GetInstanceCount() const { return m_instanceCount; }

    /**
     * Sets the estimated GPU memory usage in bytes.
     * @param bytes Total GPU memory used by buffers and textures
     */
    void SetGPUMemoryBytes(uint64_t bytes) { m_gpuMemoryBytes = bytes; }

    /**
     * Gets the estimated GPU memory usage in bytes.
     * @return GPU memory in bytes
     */
    uint64_t GetGPUMemoryBytes() const { return m_gpuMemoryBytes; }

    /**
     * Gets the total number of frames recorded.
     * @return Frame count
     */
    uint64_t GetTotalFrames() const { return m_totalFrames; }

    /**
     * Resets min/max statistics.
     */
    void ResetMinMax();

    /**
     * Prints a formatted benchmark report to the console.
     */
    void PrintReport() const;

private:
    static constexpr size_t ROLLING_AVERAGE_SIZE = 100;

    // Performance counter values
    uint64_t m_frameStartCounter = 0;
    uint64_t m_performanceFrequency = 0;

    // Frame timing
    float m_currentFrameTimeMs = 0.0f;
    float m_averageFrameTimeMs = 0.0f;
    float m_minFrameTimeMs = 0.0f;
    float m_maxFrameTimeMs = 0.0f;

    // Rolling average buffer
    std::array<float, ROLLING_AVERAGE_SIZE> m_frameTimes = {};
    size_t m_frameTimeIndex = 0;
    bool m_bufferFilled = false;

    // Statistics
    uint32_t m_drawCalls = 0;
    uint32_t m_instanceCount = 0;
    uint64_t m_totalFrames = 0;
    uint64_t m_gpuMemoryBytes = 0;

    void UpdateRollingAverage(float frameTimeMs);
};

} // namespace poc1
