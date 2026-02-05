/**
 * @file AsyncLoader.cpp
 * @brief Async asset loader implementation.
 */

#include "sims3000/assets/AsyncLoader.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_timer.h>
#include <fstream>

// Simple image loading for background thread (CPU only)
// In production, would use stb_image or SDL_image
namespace {

bool loadImageData(const std::string& path,
                   std::vector<unsigned char>& data,
                   int& width, int& height, int& channels) {
    // Placeholder: Read raw file data
    // Real implementation would decode PNG/JPG to raw pixels
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        return false;
    }

    // For now, just store raw file data
    // GPU upload will handle actual decoding
    width = 0;
    height = 0;
    channels = 0;

    return true;
}

} // anonymous namespace

namespace sims3000 {

AsyncLoader::AsyncLoader() = default;

AsyncLoader::~AsyncLoader() {
    stop();
}

void AsyncLoader::start() {
    if (m_running) {
        return;
    }

    m_running = true;
    m_workerThread = std::thread(&AsyncLoader::workerThreadFunc, this);
    SDL_Log("AsyncLoader: Worker thread started");
}

void AsyncLoader::stop() {
    if (!m_running) {
        return;
    }

    m_running = false;
    m_requestCV.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    SDL_Log("AsyncLoader: Worker thread stopped");
}

void AsyncLoader::queueLoad(const std::string& path,
                            LoadPriority priority,
                            std::function<void(bool)> callback) {
    {
        std::lock_guard<std::mutex> progressLock(m_progressMutex);
        m_progress.totalRequests++;
    }

    {
        std::lock_guard<std::mutex> lock(m_requestMutex);
        m_requests.push({path, priority, std::move(callback)});
    }
    m_requestCV.notify_one();
}

void AsyncLoader::workerThreadFunc() {
    while (m_running) {
        LoadRequest request;

        {
            std::unique_lock<std::mutex> lock(m_requestMutex);
            m_requestCV.wait(lock, [this] {
                return !m_running || !m_requests.empty();
            });

            if (!m_running && m_requests.empty()) {
                break;
            }

            if (!m_requests.empty()) {
                request = m_requests.top();
                m_requests.pop();
            } else {
                continue;
            }
        }

        // Update current loading path
        {
            std::lock_guard<std::mutex> lock(m_progressMutex);
            m_progress.currentPath = request.path;
        }

        processRequest(request);
    }
}

void AsyncLoader::processRequest(const LoadRequest& request) {
    PendingUpload upload;
    upload.path = request.path;
    upload.callback = request.callback;

    bool success = loadImageData(request.path,
                                  upload.data,
                                  upload.width,
                                  upload.height,
                                  upload.channels);

    if (success) {
        std::lock_guard<std::mutex> lock(m_uploadMutex);
        m_pendingUploads.push(std::move(upload));
    } else {
        SDL_Log("AsyncLoader: Failed to load '%s'", request.path.c_str());

        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_progress.failedRequests++;
        m_progress.currentPath.clear();

        // Call callback with failure
        if (request.callback) {
            // Note: In production, would queue this for main thread
            request.callback(false);
        }
    }
}

int AsyncLoader::processUploads(float maxTimeMs) {
    std::uint64_t startTime = SDL_GetPerformanceCounter();
    std::uint64_t freq = SDL_GetPerformanceFrequency();
    float elapsedMs = 0.0f;
    int processed = 0;

    while (elapsedMs < maxTimeMs) {
        PendingUpload upload;

        {
            std::lock_guard<std::mutex> lock(m_uploadMutex);
            if (m_pendingUploads.empty()) {
                break;
            }
            upload = std::move(m_pendingUploads.front());
            m_pendingUploads.pop();
        }

        // In real implementation, would create GPU texture here
        // For now, just mark as complete
        {
            std::lock_guard<std::mutex> lock(m_progressMutex);
            m_progress.completedRequests++;
            m_progress.currentPath.clear();
        }

        if (upload.callback) {
            upload.callback(true);
        }

        processed++;

        std::uint64_t currentTime = SDL_GetPerformanceCounter();
        elapsedMs = static_cast<float>(currentTime - startTime) /
                    static_cast<float>(freq) * 1000.0f;
    }

    // Call progress callback
    if (processed > 0 && m_progressCallback) {
        LoadProgress progress = getProgress();
        m_progressCallback(progress);
    }

    return processed;
}

LoadProgress AsyncLoader::getProgress() const {
    std::lock_guard<std::mutex> lock(m_progressMutex);
    return m_progress;
}

bool AsyncLoader::isLoading() const {
    std::lock_guard<std::mutex> requestLock(m_requestMutex);
    std::lock_guard<std::mutex> uploadLock(m_uploadMutex);

    return !m_requests.empty() || !m_pendingUploads.empty();
}

void AsyncLoader::setProgressCallback(std::function<void(const LoadProgress&)> callback) {
    m_progressCallback = std::move(callback);
}

} // namespace sims3000
