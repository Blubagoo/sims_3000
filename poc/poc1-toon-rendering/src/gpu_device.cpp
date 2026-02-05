#include "gpu_device.h"

GPUDevice::GPUDevice(SDL_Window* window)
    : m_window(window)
{
    if (!m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice: Cannot create device with null window");
        return;
    }

    // Create GPU device with SPIRV and DXIL shader format support
    // Enable debug mode for development
    m_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL,
        true,   // debug_mode
        nullptr // name (let SDL choose the best backend)
    );

    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice: Failed to create GPU device: %s", SDL_GetError());
        return;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Successfully created GPU device");

    // Log the device driver name for debugging
    const char* driverName = SDL_GetGPUDeviceDriver(m_device);
    if (driverName) {
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Using driver: %s", driverName);
    }
}

GPUDevice::~GPUDevice()
{
    if (m_device) {
        // The device destruction will handle releasing the window claim
        SDL_DestroyGPUDevice(m_device);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Destroyed GPU device");
        m_device = nullptr;
    }
    m_windowClaimed = false;
    m_window = nullptr;
}

GPUDevice::GPUDevice(GPUDevice&& other) noexcept
    : m_device(other.m_device)
    , m_window(other.m_window)
    , m_windowClaimed(other.m_windowClaimed)
{
    other.m_device = nullptr;
    other.m_window = nullptr;
    other.m_windowClaimed = false;
}

GPUDevice& GPUDevice::operator=(GPUDevice&& other) noexcept
{
    if (this != &other) {
        // Clean up existing resources
        if (m_device) {
            SDL_DestroyGPUDevice(m_device);
        }

        // Move from other
        m_device = other.m_device;
        m_window = other.m_window;
        m_windowClaimed = other.m_windowClaimed;

        // Clear other
        other.m_device = nullptr;
        other.m_window = nullptr;
        other.m_windowClaimed = false;
    }
    return *this;
}

SDL_GPUDevice* GPUDevice::GetDevice() const
{
    return m_device;
}

SDL_Window* GPUDevice::GetWindow() const
{
    return m_window;
}

bool GPUDevice::ClaimWindow()
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::ClaimWindow: Device not initialized");
        return false;
    }

    if (!m_window) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::ClaimWindow: Window is null");
        return false;
    }

    if (m_windowClaimed) {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "GPUDevice::ClaimWindow: Window already claimed");
        return true;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_device, m_window)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::ClaimWindow: Failed to claim window: %s", SDL_GetError());
        return false;
    }

    m_windowClaimed = true;
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice::ClaimWindow: Successfully claimed window for GPU rendering");
    return true;
}

SDL_GPUCommandBuffer* GPUDevice::AcquireCommandBuffer()
{
    if (!m_device) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::AcquireCommandBuffer: Device not initialized");
        return nullptr;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_device);
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::AcquireCommandBuffer: Failed to acquire command buffer: %s", SDL_GetError());
        return nullptr;
    }

    return commandBuffer;
}

bool GPUDevice::Submit(SDL_GPUCommandBuffer* commandBuffer)
{
    if (!commandBuffer) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::Submit: Command buffer is null");
        return false;
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "GPUDevice::Submit: Failed to submit command buffer: %s", SDL_GetError());
        return false;
    }

    return true;
}

bool GPUDevice::IsValid() const
{
    return m_device != nullptr;
}
