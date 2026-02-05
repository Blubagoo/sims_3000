/**
 * @file AsyncLoader.h
 * @brief Background thread asset loading with progress tracking.
 *
 * Two-phase loading pattern:
 * - Background thread: File I/O and decoding
 * - Main thread: GPU resource creation
 */

#ifndef SIMS3000_ASSETS_ASYNCLOADER_H
#define SIMS3000_ASSETS_ASYNCLOADER_H

#include <functional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace sims3000 {

/**
 * @enum LoadPriority
 * @brief Priority levels for async loading.
 */
enum class LoadPriority {
    Low = 0,
    Normal = 1,
    High = 2
};

/**
 * @struct LoadRequest
 * @brief A pending asset load request.
 */
struct LoadRequest {
    std::string path;
    LoadPriority priority = LoadPriority::Normal;
    std::function<void(bool success)> callback;

    bool operator<(const LoadRequest& other) const {
        return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
};

/**
 * @struct LoadProgress
 * @brief Current loading progress information.
 */
struct LoadProgress {
    std::size_t totalRequests = 0;
    std::size_t completedRequests = 0;
    std::size_t failedRequests = 0;
    std::string currentPath;

    float getProgress() const {
        if (totalRequests == 0) return 1.0f;
        return static_cast<float>(completedRequests) / static_cast<float>(totalRequests);
    }

    bool isComplete() const {
        return completedRequests + failedRequests >= totalRequests;
    }
};

/**
 * @struct PendingUpload
 * @brief Data waiting for main thread GPU upload.
 */
struct PendingUpload {
    std::string path;
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;
    int channels = 0;
    std::function<void(bool success)> callback;
};

/**
 * @class AsyncLoader
 * @brief Background asset loading with progress tracking.
 *
 * Manages a worker thread for file I/O and decoding.
 * Main thread calls processUploads() to create GPU resources.
 */
class AsyncLoader {
public:
    AsyncLoader();
    ~AsyncLoader();

    // Non-copyable
    AsyncLoader(const AsyncLoader&) = delete;
    AsyncLoader& operator=(const AsyncLoader&) = delete;

    /**
     * Start the background loader thread.
     */
    void start();

    /**
     * Stop the background loader thread.
     */
    void stop();

    /**
     * Queue an asset for async loading.
     * @param path Asset path
     * @param priority Load priority
     * @param callback Called when load completes (on main thread)
     */
    void queueLoad(const std::string& path,
                   LoadPriority priority = LoadPriority::Normal,
                   std::function<void(bool)> callback = nullptr);

    /**
     * Process pending GPU uploads on main thread.
     * @param maxTimeMs Maximum time to spend uploading (default 2ms)
     * @return Number of uploads processed
     */
    int processUploads(float maxTimeMs = 2.0f);

    /**
     * Get current loading progress.
     */
    LoadProgress getProgress() const;

    /**
     * Check if there are pending loads or uploads.
     */
    bool isLoading() const;

    /**
     * Set progress callback (called on main thread during processUploads).
     */
    void setProgressCallback(std::function<void(const LoadProgress&)> callback);

private:
    void workerThreadFunc();
    void processRequest(const LoadRequest& request);

    std::thread m_workerThread;
    std::atomic<bool> m_running{false};

    // Request queue (worker thread reads)
    mutable std::mutex m_requestMutex;
    std::condition_variable m_requestCV;
    std::priority_queue<LoadRequest> m_requests;

    // Upload queue (main thread reads)
    mutable std::mutex m_uploadMutex;
    std::queue<PendingUpload> m_pendingUploads;

    // Progress tracking
    mutable std::mutex m_progressMutex;
    LoadProgress m_progress;

    std::function<void(const LoadProgress&)> m_progressCallback;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_ASYNCLOADER_H
