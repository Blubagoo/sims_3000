/**
 * @file TextureLoader.cpp
 * @brief TextureLoader implementation using stb_image for PNG/JPG loading.
 */

// Define stb_image implementation in this compilation unit
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "sims3000/assets/TextureLoader.h"
#include "sims3000/render/Window.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_filesystem.h>
#include <cstring>

namespace sims3000 {

TextureLoader::TextureLoader(Window& window)
    : m_window(window)
{
    m_linearSampler = createSampler(TextureFilter::Linear);
    m_nearestSampler = createSampler(TextureFilter::Nearest);

    if (!m_linearSampler) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "TextureLoader: Failed to create linear sampler");
    }
    if (!m_nearestSampler) {
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "TextureLoader: Failed to create nearest sampler");
    }
}

TextureLoader::~TextureLoader() {
    clearAll();

    auto device = m_window.getDevice();
    if (device) {
        if (m_linearSampler) {
            SDL_ReleaseGPUSampler(device, m_linearSampler);
            m_linearSampler = nullptr;
        }
        if (m_nearestSampler) {
            SDL_ReleaseGPUSampler(device, m_nearestSampler);
            m_nearestSampler = nullptr;
        }
    }
}

TextureHandle TextureLoader::load(const std::string& path) {
    return load(path, TextureLoadOptions{});
}

TextureHandle TextureLoader::load(const std::string& path, const TextureLoadOptions& options) {
    // Check cache first
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    // Load image using stb_image
    int width, height, channels;
    unsigned char* pixels = loadImageFile(path, &width, &height, &channels);

    if (!pixels) {
        // stb_image failed - try SDL's BMP loader as fallback
        SDL_Surface* surface = SDL_LoadBMP(path.c_str());
        if (surface) {
            SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
            SDL_DestroySurface(surface);

            if (converted) {
                SDL_GPUTexture* gpuTexture = createGPUTexture(converted);
                width = converted->w;
                height = converted->h;
                channels = 4;
                SDL_DestroySurface(converted);

                if (gpuTexture) {
                    Texture tex;
                    tex.gpuTexture = gpuTexture;
                    tex.sampler = (options.filter == TextureFilter::Linear)
                        ? m_linearSampler : m_nearestSampler;
                    tex.width = width;
                    tex.height = height;
                    tex.channels = channels;
                    tex.refCount = 1;
                    tex.path = path;
                    tex.filter = options.filter;

                    // Get modification time
                    SDL_PathInfo info;
                    if (SDL_GetPathInfo(path.c_str(), &info)) {
                        tex.lastModified = static_cast<std::uint64_t>(info.modify_time);
                    }

                    auto result = m_cache.emplace(path, std::move(tex));
                    return &result.first->second;
                }
            }
        }

        m_lastError = "Failed to load texture: " + path;
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return nullptr;
    }

    // Create GPU texture from loaded pixels
    SDL_GPUTexture* gpuTexture = createGPUTextureFromPixels(pixels, width, height);
    stbi_image_free(pixels);

    if (!gpuTexture) {
        return nullptr;
    }

    // Add to cache
    Texture tex;
    tex.gpuTexture = gpuTexture;
    tex.sampler = (options.filter == TextureFilter::Linear)
        ? m_linearSampler : m_nearestSampler;
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.refCount = 1;
    tex.path = path;
    tex.filter = options.filter;

    // Get modification time
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path.c_str(), &info)) {
        tex.lastModified = static_cast<std::uint64_t>(info.modify_time);
    }

    auto result = m_cache.emplace(path, std::move(tex));
    SDL_LogDebug(SDL_LOG_CATEGORY_GPU, "TextureLoader: Loaded %s (%dx%d, %d channels)",
                 path.c_str(), width, height, channels);

    return &result.first->second;
}

TextureHandle TextureLoader::loadFromMemory(const std::string& name,
                                             const void* data, std::size_t size,
                                             const TextureLoadOptions& options) {
    // Check cache first
    auto it = m_cache.find(name);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    // Load image using stb_image
    int width, height, channels;
    unsigned char* pixels = loadImageMemory(data, size, &width, &height, &channels);

    if (!pixels) {
        m_lastError = "Failed to load texture from memory: " + name;
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return nullptr;
    }

    // Create GPU texture from loaded pixels
    SDL_GPUTexture* gpuTexture = createGPUTextureFromPixels(pixels, width, height);
    stbi_image_free(pixels);

    if (!gpuTexture) {
        return nullptr;
    }

    // Add to cache
    Texture tex;
    tex.gpuTexture = gpuTexture;
    tex.sampler = (options.filter == TextureFilter::Linear)
        ? m_linearSampler : m_nearestSampler;
    tex.width = width;
    tex.height = height;
    tex.channels = channels;
    tex.refCount = 1;
    tex.path = name;
    tex.filter = options.filter;
    tex.lastModified = 0;  // No file to track

    auto result = m_cache.emplace(name, std::move(tex));
    return &result.first->second;
}

TextureHandle TextureLoader::createFromPixels(const std::string& name,
                                               const void* pixels, int width, int height,
                                               const TextureLoadOptions& options) {
    // Check cache first
    auto it = m_cache.find(name);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    if (!pixels || width <= 0 || height <= 0) {
        m_lastError = "Invalid pixel data for texture: " + name;
        return nullptr;
    }

    // Create GPU texture directly from pixels
    SDL_GPUTexture* gpuTexture = createGPUTextureFromPixels(pixels, width, height);
    if (!gpuTexture) {
        return nullptr;
    }

    // Add to cache
    Texture tex;
    tex.gpuTexture = gpuTexture;
    tex.sampler = (options.filter == TextureFilter::Linear)
        ? m_linearSampler : m_nearestSampler;
    tex.width = width;
    tex.height = height;
    tex.channels = 4;  // Always RGBA
    tex.refCount = 1;
    tex.path = name;
    tex.filter = options.filter;
    tex.lastModified = 0;

    auto result = m_cache.emplace(name, std::move(tex));
    return &result.first->second;
}

TextureHandle TextureLoader::createFallback() {
    // Check if already created
    const std::string fallbackPath = "__fallback__";
    auto it = m_cache.find(fallbackPath);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    // Create a magenta/black checkerboard pattern
    const int size = 64;
    const int checkerSize = 8;

    std::vector<std::uint32_t> pixels(size * size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool isEven = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            pixels[y * size + x] = isEven ? 0xFFFF00FF : 0xFF000000;  // ABGR: magenta or black
        }
    }

    return createFromPixels(fallbackPath, pixels.data(), size, size, TextureLoadOptions{});
}

void TextureLoader::addRef(TextureHandle handle) {
    if (handle) {
        handle->refCount++;
    }
}

void TextureLoader::release(TextureHandle handle) {
    if (handle && handle->refCount > 0) {
        handle->refCount--;
    }
}

void TextureLoader::clearUnused() {
    auto device = m_window.getDevice();
    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (it->second.refCount == 0) {
            if (it->second.gpuTexture && device) {
                SDL_ReleaseGPUTexture(device, it->second.gpuTexture);
            }
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void TextureLoader::clearAll() {
    auto device = m_window.getDevice();
    for (auto& [path, tex] : m_cache) {
        if (tex.gpuTexture && device) {
            SDL_ReleaseGPUTexture(device, tex.gpuTexture);
        }
    }
    m_cache.clear();
}

void TextureLoader::getStats(size_t& outCount, size_t& outBytes) const {
    outCount = m_cache.size();
    outBytes = 0;
    for (const auto& [path, tex] : m_cache) {
        // Estimate 4 bytes per pixel (RGBA)
        outBytes += static_cast<size_t>(tex.width) * tex.height * 4;
    }
}

bool TextureLoader::reloadIfModified(TextureHandle handle) {
    if (!handle || handle->path.empty() || handle->path[0] == '_') {
        return false;  // Can't reload procedural textures
    }

    // Get current modification time
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(handle->path.c_str(), &info)) {
        return false;
    }

    if (static_cast<std::uint64_t>(info.modify_time) <= handle->lastModified) {
        return false;
    }

    // Reload the texture
    int width, height, channels;
    unsigned char* pixels = loadImageFile(handle->path, &width, &height, &channels);

    if (!pixels) {
        // Try BMP fallback
        SDL_Surface* surface = SDL_LoadBMP(handle->path.c_str());
        if (!surface) return false;

        SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) return false;

        SDL_GPUTexture* newTexture = createGPUTexture(converted);
        width = converted->w;
        height = converted->h;
        SDL_DestroySurface(converted);

        if (!newTexture) return false;

        // Replace old texture
        if (handle->gpuTexture) {
            SDL_ReleaseGPUTexture(m_window.getDevice(), handle->gpuTexture);
        }
        handle->gpuTexture = newTexture;
        handle->width = width;
        handle->height = height;
        handle->lastModified = static_cast<std::uint64_t>(info.modify_time);

        SDL_Log("Hot-reloaded texture: %s", handle->path.c_str());
        return true;
    }

    SDL_GPUTexture* newTexture = createGPUTextureFromPixels(pixels, width, height);
    stbi_image_free(pixels);

    if (!newTexture) return false;

    // Replace old texture
    if (handle->gpuTexture) {
        SDL_ReleaseGPUTexture(m_window.getDevice(), handle->gpuTexture);
    }
    handle->gpuTexture = newTexture;
    handle->width = width;
    handle->height = height;
    handle->channels = channels;
    handle->lastModified = static_cast<std::uint64_t>(info.modify_time);

    SDL_Log("Hot-reloaded texture: %s", handle->path.c_str());
    return true;
}

unsigned char* TextureLoader::loadImageFile(const std::string& path,
                                             int* outWidth, int* outHeight, int* outChannels) {
    // Force RGBA output (4 channels)
    unsigned char* data = stbi_load(path.c_str(), outWidth, outHeight, outChannels, 4);
    if (!data) {
        m_lastError = "stbi_load failed: ";
        m_lastError += stbi_failure_reason();
    }
    return data;
}

unsigned char* TextureLoader::loadImageMemory(const void* data, std::size_t size,
                                               int* outWidth, int* outHeight, int* outChannels) {
    // Force RGBA output (4 channels)
    unsigned char* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(data),
        static_cast<int>(size),
        outWidth, outHeight, outChannels, 4);
    if (!pixels) {
        m_lastError = "stbi_load_from_memory failed: ";
        m_lastError += stbi_failure_reason();
    }
    return pixels;
}

SDL_GPUTexture* TextureLoader::createGPUTextureFromPixels(const void* pixels,
                                                           int width, int height) {
    auto device = m_window.getDevice();
    if (!device) {
        m_lastError = "No GPU device available";
        return nullptr;
    }

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = static_cast<Uint32>(width);
    texInfo.height = static_cast<Uint32>(height);
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texInfo);
    if (!texture) {
        m_lastError = "Failed to create GPU texture: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
        return nullptr;
    }

    // Upload pixel data via transfer buffer
    std::uint32_t dataSize = static_cast<std::uint32_t>(width * height * 4);

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        SDL_ReleaseGPUTexture(device, texture);
        m_lastError = "Failed to create transfer buffer: ";
        m_lastError += SDL_GetError();
        return nullptr;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped) {
        std::memcpy(mapped, pixels, dataSize);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transfer;
    srcInfo.offset = 0;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = texture;
    dstRegion.w = static_cast<Uint32>(width);
    dstRegion.h = static_cast<Uint32>(height);
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return texture;
}

SDL_GPUTexture* TextureLoader::createGPUTexture(SDL_Surface* surface) {
    if (!surface) {
        return nullptr;
    }
    return createGPUTextureFromPixels(surface->pixels, surface->w, surface->h);
}

SDL_GPUSampler* TextureLoader::createSampler(TextureFilter filter) {
    auto device = m_window.getDevice();
    if (!device) {
        return nullptr;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {};

    if (filter == TextureFilter::Linear) {
        samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    } else {
        samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
        samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    }

    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!sampler) {
        m_lastError = "Failed to create sampler: ";
        m_lastError += SDL_GetError();
        SDL_LogError(SDL_LOG_CATEGORY_GPU, "%s", m_lastError.c_str());
    }

    return sampler;
}

} // namespace sims3000
