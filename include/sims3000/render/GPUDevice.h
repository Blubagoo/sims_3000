/**
 * @file GPUDevice.h
 * @brief SDL_GPU device wrapper with capability detection and lifecycle management.
 *
 * Encapsulates SDL_GPU device creation, backend detection (D3D12, Vulkan, Metal),
 * shader format support detection (SPIR-V, DXIL), and debug layer configuration.
 *
 * Resource ownership:
 * - GPUDevice owns the SDL_GPUDevice
 * - Window claiming transfers swapchain ownership to the device
 * - Destruction order: release window claim -> destroy device
 */

#ifndef SIMS3000_RENDER_GPU_DEVICE_H
#define SIMS3000_RENDER_GPU_DEVICE_H

#include <SDL3/SDL.h>
#include <cstdint>
#include <string>

namespace sims3000 {

/**
 * @enum GPUBackend
 * @brief Enumeration of supported GPU backends.
 */
enum class GPUBackend {
    Unknown,
    D3D12,
    Vulkan,
    Metal
};

/**
 * @struct GPUDeviceCapabilities
 * @brief Detected GPU device capabilities.
 */
struct GPUDeviceCapabilities {
    GPUBackend backend = GPUBackend::Unknown;
    std::string backendName;
    std::string driverInfo;

    // Shader format support
    bool supportsSpirV = false;
    bool supportsDXIL = false;
    bool supportsDXBC = false;
    bool supportsMetalLib = false;

    // Feature flags
    bool debugLayersEnabled = false;
};

/**
 * @class GPUDevice
 * @brief RAII wrapper for SDL_GPUDevice with capability detection.
 *
 * Manages GPU device lifecycle, shader format detection, backend selection,
 * and debug layer configuration. Provides clean interface for command buffer
 * management and window swapchain claiming.
 *
 * Usage:
 * @code
 *   GPUDevice device;
 *   if (!device.isValid()) {
 *       // Handle error - check device.getLastError()
 *   }
 *
 *   // Log capabilities
 *   const auto& caps = device.getCapabilities();
 *   SDL_Log("Backend: %s", caps.backendName.c_str());
 *
 *   // Claim window for rendering
 *   if (!device.claimWindow(myWindow)) {
 *       // Handle error
 *   }
 *
 *   // Render loop
 *   SDL_GPUCommandBuffer* cmd = device.acquireCommandBuffer();
 *   // ... record commands ...
 *   device.submit(cmd);
 * @endcode
 */
class GPUDevice {
public:
    /**
     * Create GPU device with automatic backend selection.
     * Enables debug layers in debug builds (SIMS3000_DEBUG defined).
     * Requests SPIR-V and DXIL shader format support.
     */
    GPUDevice();

    /**
     * Create GPU device with explicit debug mode control.
     * @param enableDebugLayers Force debug layer state (overrides build type)
     */
    explicit GPUDevice(bool enableDebugLayers);

    ~GPUDevice();

    // Non-copyable
    GPUDevice(const GPUDevice&) = delete;
    GPUDevice& operator=(const GPUDevice&) = delete;

    // Movable
    GPUDevice(GPUDevice&& other) noexcept;
    GPUDevice& operator=(GPUDevice&& other) noexcept;

    /**
     * Check if device was created successfully.
     * @return true if device is valid and ready for use
     */
    bool isValid() const;

    /**
     * Get the underlying SDL GPU device handle.
     * @return Pointer to SDL_GPUDevice, or nullptr if not initialized
     */
    SDL_GPUDevice* getHandle() const;

    /**
     * Get detected device capabilities.
     * @return Reference to capabilities structure
     */
    const GPUDeviceCapabilities& getCapabilities() const;

    /**
     * Get the last error message from device creation or operations.
     * @return Error message string
     */
    const std::string& getLastError() const;

    /**
     * Claim a window for GPU rendering (swapchain setup).
     * Must be called before rendering to the window.
     * @param window SDL window to claim
     * @return true if successful, false otherwise (check getLastError())
     */
    bool claimWindow(SDL_Window* window);

    /**
     * Release a previously claimed window.
     * @param window SDL window to release
     */
    void releaseWindow(SDL_Window* window);

    /**
     * Acquire a command buffer for recording GPU commands.
     * Caller is responsible for submitting via submit() or canceling.
     * @return Pointer to SDL_GPUCommandBuffer, or nullptr on failure
     */
    SDL_GPUCommandBuffer* acquireCommandBuffer();

    /**
     * Submit a command buffer for execution.
     * After submission, the command buffer is no longer valid.
     * @param commandBuffer The command buffer to submit
     * @return true if successful, false otherwise
     */
    bool submit(SDL_GPUCommandBuffer* commandBuffer);

    /**
     * Wait for GPU to become idle.
     * Blocks until all submitted work completes.
     */
    void waitForIdle();

    /**
     * Log device capabilities to SDL log.
     * Outputs backend, driver info, shader format support, and debug status.
     */
    void logCapabilities() const;

    /**
     * Check if a specific shader format is supported.
     * @param format SDL_GPUShaderFormat to check
     * @return true if the format is supported
     */
    bool supportsShaderFormat(SDL_GPUShaderFormat format) const;

private:
    /**
     * Initialize the device with specified debug mode.
     * @param enableDebugLayers Whether to enable debug/validation layers
     */
    void initialize(bool enableDebugLayers);

    /**
     * Detect and populate capabilities from the created device.
     */
    void detectCapabilities();

    /**
     * Parse backend name to enum.
     * @param name Driver name string from SDL
     * @return Corresponding GPUBackend enum value
     */
    static GPUBackend parseBackendName(const char* name);

    /**
     * Clean up device resources.
     */
    void cleanup();

    SDL_GPUDevice* m_device = nullptr;
    GPUDeviceCapabilities m_capabilities;
    std::string m_lastError;
};

/**
 * Convert GPUBackend enum to string.
 * @param backend Backend enum value
 * @return Human-readable string representation
 */
const char* getBackendName(GPUBackend backend);

} // namespace sims3000

#endif // SIMS3000_RENDER_GPU_DEVICE_H
