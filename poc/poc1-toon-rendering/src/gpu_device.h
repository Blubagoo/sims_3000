#ifndef GPU_DEVICE_H
#define GPU_DEVICE_H

#include <SDL3/SDL.h>

/**
 * GPUDevice - Wrapper for SDL_GPUDevice
 *
 * Provides a clean interface for GPU device management including:
 * - Device creation and destruction
 * - Window swapchain claiming
 * - Command buffer acquisition and submission
 */
class GPUDevice {
public:
    /**
     * Construct a GPUDevice with an associated window.
     * Creates the SDL GPU device with SPIRV and DXIL shader support.
     *
     * @param window The SDL window to associate with this device
     */
    explicit GPUDevice(SDL_Window* window);

    /**
     * Destructor - Releases the GPU device and unclaims the window.
     */
    ~GPUDevice();

    // Non-copyable
    GPUDevice(const GPUDevice&) = delete;
    GPUDevice& operator=(const GPUDevice&) = delete;

    // Movable
    GPUDevice(GPUDevice&& other) noexcept;
    GPUDevice& operator=(GPUDevice&& other) noexcept;

    /**
     * Get the underlying SDL GPU device.
     * @return Pointer to SDL_GPUDevice, or nullptr if not initialized
     */
    SDL_GPUDevice* GetDevice() const;

    /**
     * Get the associated window.
     * @return Pointer to SDL_Window
     */
    SDL_Window* GetWindow() const;

    /**
     * Claim the window for GPU rendering (swapchain setup).
     * Must be called before rendering to the window.
     *
     * @return true if successful, false otherwise
     */
    bool ClaimWindow();

    /**
     * Acquire a command buffer for recording GPU commands.
     * The caller is responsible for submitting the buffer via Submit().
     *
     * @return Pointer to SDL_GPUCommandBuffer, or nullptr on failure
     */
    SDL_GPUCommandBuffer* AcquireCommandBuffer();

    /**
     * Submit a command buffer for execution.
     * After submission, the command buffer is no longer valid.
     *
     * @param commandBuffer The command buffer to submit
     * @return true if successful, false otherwise
     */
    bool Submit(SDL_GPUCommandBuffer* commandBuffer);

    /**
     * Check if the device was initialized successfully.
     * @return true if the device is valid and ready for use
     */
    bool IsValid() const;

private:
    SDL_GPUDevice* m_device = nullptr;
    SDL_Window* m_window = nullptr;
    bool m_windowClaimed = false;
};

#endif // GPU_DEVICE_H
