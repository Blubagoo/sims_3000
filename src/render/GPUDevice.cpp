/**
 * @file GPUDevice.cpp
 * @brief SDL_GPU device wrapper implementation.
 */

#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>
#include <cstring>

namespace sims3000 {

// =============================================================================
// Public API
// =============================================================================

GPUDevice::GPUDevice() {
#ifdef SIMS3000_DEBUG
    initialize(true);
#else
    initialize(false);
#endif
}

GPUDevice::GPUDevice(bool enableDebugLayers) {
    initialize(enableDebugLayers);
}

GPUDevice::~GPUDevice() {
    cleanup();
}

GPUDevice::GPUDevice(GPUDevice&& other) noexcept
    : m_device(other.m_device)
    , m_capabilities(std::move(other.m_capabilities))
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
}

GPUDevice& GPUDevice::operator=(GPUDevice&& other) noexcept {
    if (this != &other) {
        cleanup();

        m_device = other.m_device;
        m_capabilities = std::move(other.m_capabilities);
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
    }
    return *this;
}

bool GPUDevice::isValid() const {
    return m_device != nullptr;
}

SDL_GPUDevice* GPUDevice::getHandle() const {
    return m_device;
}

const GPUDeviceCapabilities& GPUDevice::getCapabilities() const {
    return m_capabilities;
}

const std::string& GPUDevice::getLastError() const {
    return m_lastError;
}

bool GPUDevice::claimWindow(SDL_Window* window) {
    if (!m_device) {
        m_lastError = "Cannot claim window: GPU device not initialized";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!window) {
        m_lastError = "Cannot claim window: window is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(m_device, window)) {
        m_lastError = "Failed to claim window for GPU device: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Successfully claimed window for rendering");
    return true;
}

void GPUDevice::releaseWindow(SDL_Window* window) {
    if (m_device && window) {
        SDL_ReleaseWindowFromGPUDevice(m_device, window);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Released window from GPU device");
    }
}

SDL_GPUCommandBuffer* GPUDevice::acquireCommandBuffer() {
    if (!m_device) {
        m_lastError = "Cannot acquire command buffer: GPU device not initialized";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return nullptr;
    }

    SDL_GPUCommandBuffer* cmdBuffer = SDL_AcquireGPUCommandBuffer(m_device);
    if (!cmdBuffer) {
        m_lastError = "Failed to acquire command buffer: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return nullptr;
    }

    return cmdBuffer;
}

bool GPUDevice::submit(SDL_GPUCommandBuffer* commandBuffer) {
    if (!commandBuffer) {
        m_lastError = "Cannot submit: command buffer is null";
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        m_lastError = "Failed to submit command buffer: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return false;
    }

    return true;
}

void GPUDevice::waitForIdle() {
    if (m_device) {
        SDL_WaitForGPUIdle(m_device);
    }
}

void GPUDevice::logCapabilities() const {
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "=== GPU Device Capabilities ===");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Backend: %s", m_capabilities.backendName.c_str());
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Driver: %s", m_capabilities.driverInfo.c_str());
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Debug layers: %s",
                m_capabilities.debugLayersEnabled ? "enabled" : "disabled");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "Shader formats:");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  SPIR-V: %s",
                m_capabilities.supportsSpirV ? "yes" : "no");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  DXIL: %s",
                m_capabilities.supportsDXIL ? "yes" : "no");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  DXBC: %s",
                m_capabilities.supportsDXBC ? "yes" : "no");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "  Metal: %s",
                m_capabilities.supportsMetalLib ? "yes" : "no");
    SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "================================");
}

bool GPUDevice::supportsShaderFormat(SDL_GPUShaderFormat format) const {
    if (!m_device) {
        return false;
    }

    // Query actual support from the device
    SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(m_device);
    return (supported & format) != 0;
}

// =============================================================================
// Private Implementation
// =============================================================================

void GPUDevice::initialize(bool enableDebugLayers) {
    m_capabilities.debugLayersEnabled = enableDebugLayers;

    // Request SPIR-V and DXIL shader format support for cross-platform compatibility
    // - SPIR-V: Used by Vulkan
    // - DXIL: Used by D3D12 (DirectX 12)
    // SDL3 will automatically select the best available backend
    SDL_GPUShaderFormat requestedFormats =
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL;

    SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                "GPUDevice: Creating device (debug=%s, formats=SPIRV|DXIL)",
                enableDebugLayers ? "true" : "false");

    m_device = SDL_CreateGPUDevice(
        requestedFormats,
        enableDebugLayers,
        nullptr  // Let SDL choose the best backend automatically
    );

    if (!m_device) {
        m_lastError = "Failed to create GPU device: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());

        // Provide helpful error message with fallback suggestions
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "GPU device creation failed. Possible causes:");
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "  - No compatible GPU driver installed");
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "  - GPU does not support Vulkan or D3D12");
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "  - Debug layers requested but not installed");
        SDL_LogError(SDL_LOG_CATEGORY_GPU,
                     "  - Running on a system without GPU support");

        // Try again without debug layers if that might be the issue
        if (enableDebugLayers) {
            SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                        "Retrying GPU device creation without debug layers...");

            m_device = SDL_CreateGPUDevice(requestedFormats, false, nullptr);
            if (m_device) {
                m_capabilities.debugLayersEnabled = false;
                SDL_LogWarn(SDL_LOG_CATEGORY_GPU,
                           "GPU device created without debug layers. "
                           "Install GPU validation layers for debugging.");
            }
        }
    }

    if (m_device) {
        detectCapabilities();
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU,
                    "GPUDevice: Successfully created using %s backend",
                    m_capabilities.backendName.c_str());
    }
}

void GPUDevice::detectCapabilities() {
    if (!m_device) {
        return;
    }

    // Get backend/driver name
    const char* driverName = SDL_GetGPUDeviceDriver(m_device);
    if (driverName) {
        m_capabilities.driverInfo = driverName;
        m_capabilities.backend = parseBackendName(driverName);
        m_capabilities.backendName = getBackendName(m_capabilities.backend);
    } else {
        m_capabilities.driverInfo = "Unknown";
        m_capabilities.backendName = "Unknown";
    }

    // Query shader format support
    SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(m_device);
    m_capabilities.supportsSpirV = (supported & SDL_GPU_SHADERFORMAT_SPIRV) != 0;
    m_capabilities.supportsDXIL = (supported & SDL_GPU_SHADERFORMAT_DXIL) != 0;
    m_capabilities.supportsDXBC = (supported & SDL_GPU_SHADERFORMAT_DXBC) != 0;
    m_capabilities.supportsMetalLib = (supported & SDL_GPU_SHADERFORMAT_METALLIB) != 0;
}

GPUBackend GPUDevice::parseBackendName(const char* name) {
    if (!name) {
        return GPUBackend::Unknown;
    }

    // SDL3 driver names are lowercase
    if (std::strstr(name, "d3d12") != nullptr ||
        std::strstr(name, "D3D12") != nullptr ||
        std::strstr(name, "direct3d12") != nullptr) {
        return GPUBackend::D3D12;
    }
    if (std::strstr(name, "vulkan") != nullptr ||
        std::strstr(name, "Vulkan") != nullptr) {
        return GPUBackend::Vulkan;
    }
    if (std::strstr(name, "metal") != nullptr ||
        std::strstr(name, "Metal") != nullptr) {
        return GPUBackend::Metal;
    }

    return GPUBackend::Unknown;
}

void GPUDevice::cleanup() {
    if (m_device) {
        // Wait for any pending GPU work to complete
        SDL_WaitForGPUIdle(m_device);

        SDL_DestroyGPUDevice(m_device);
        SDL_LogInfo(SDL_LOG_CATEGORY_GPU, "GPUDevice: Destroyed GPU device");
        m_device = nullptr;
    }
}

// =============================================================================
// Free Functions
// =============================================================================

const char* getBackendName(GPUBackend backend) {
    switch (backend) {
        case GPUBackend::D3D12:  return "Direct3D 12";
        case GPUBackend::Vulkan: return "Vulkan";
        case GPUBackend::Metal:  return "Metal";
        default:                 return "Unknown";
    }
}

} // namespace sims3000
