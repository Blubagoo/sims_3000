/**
 * @file TextureLoader.cpp
 * @brief TextureLoader implementation.
 */

#include "sims3000/assets/TextureLoader.h"
#include "sims3000/render/Window.h"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_filesystem.h>

namespace sims3000 {

TextureLoader::TextureLoader(Window& window)
    : m_window(window)
{
    m_defaultSampler = createSampler();
}

TextureLoader::~TextureLoader() {
    clearAll();

    if (m_defaultSampler) {
        SDL_ReleaseGPUSampler(m_window.getDevice(), m_defaultSampler);
        m_defaultSampler = nullptr;
    }
}

TextureHandle TextureLoader::load(const std::string& path) {
    // Check cache first
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    // Load image
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface) {
        // Try other formats via SDL_image if available
        SDL_Log("Failed to load texture: %s - %s", path.c_str(), SDL_GetError());
        return nullptr;
    }

    // Convert to RGBA if needed
    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);

    if (!converted) {
        SDL_Log("Failed to convert texture format: %s", SDL_GetError());
        return nullptr;
    }

    // Create GPU texture
    SDL_GPUTexture* gpuTexture = createGPUTexture(converted);
    SDL_DestroySurface(converted);

    if (!gpuTexture) {
        return nullptr;
    }

    // Add to cache
    Texture tex;
    tex.gpuTexture = gpuTexture;
    tex.sampler = m_defaultSampler;
    tex.width = converted->w;
    tex.height = converted->h;
    tex.refCount = 1;
    tex.path = path;

    auto result = m_cache.emplace(path, std::move(tex));
    return &result.first->second;
}

TextureHandle TextureLoader::createFallback() {
    // Create a magenta/black checkerboard pattern
    const int size = 64;
    const int checkerSize = 8;

    std::vector<std::uint32_t> pixels(size * size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool isEven = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
            pixels[y * size + x] = isEven ? 0xFFFF00FF : 0xFF000000;  // RGBA: magenta or black
        }
    }

    // Create surface
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        size, size,
        SDL_PIXELFORMAT_RGBA32,
        pixels.data(),
        size * sizeof(std::uint32_t)
    );

    if (!surface) {
        SDL_Log("Failed to create fallback surface: %s", SDL_GetError());
        return nullptr;
    }

    SDL_GPUTexture* gpuTexture = createGPUTexture(surface);
    SDL_DestroySurface(surface);

    if (!gpuTexture) {
        return nullptr;
    }

    // Add to cache with special name
    const std::string fallbackPath = "__fallback__";

    Texture tex;
    tex.gpuTexture = gpuTexture;
    tex.sampler = m_defaultSampler;
    tex.width = size;
    tex.height = size;
    tex.refCount = 1;
    tex.path = fallbackPath;

    auto result = m_cache.emplace(fallbackPath, std::move(tex));
    return &result.first->second;
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
            if (it->second.gpuTexture) {
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
        if (tex.gpuTexture) {
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
    if (!handle) return false;

    // Get current modification time
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(handle->path.c_str(), &info)) {
        return false;
    }

    if (static_cast<std::uint64_t>(info.modify_time) <= handle->lastModified) {
        return false;
    }

    // Reload the texture
    SDL_Surface* surface = SDL_LoadBMP(handle->path.c_str());
    if (!surface) return false;

    SDL_Surface* converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    if (!converted) return false;

    SDL_GPUTexture* newTexture = createGPUTexture(converted);
    SDL_DestroySurface(converted);
    if (!newTexture) return false;

    // Replace old texture
    if (handle->gpuTexture) {
        SDL_ReleaseGPUTexture(m_window.getDevice(), handle->gpuTexture);
    }
    handle->gpuTexture = newTexture;
    handle->width = converted->w;
    handle->height = converted->h;
    handle->lastModified = static_cast<std::uint64_t>(info.modify_time);

    SDL_Log("Hot-reloaded texture: %s", handle->path.c_str());
    return true;
}

SDL_GPUTexture* TextureLoader::createGPUTexture(SDL_Surface* surface) {
    auto device = m_window.getDevice();

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texInfo.width = static_cast<Uint32>(surface->w);
    texInfo.height = static_cast<Uint32>(surface->h);
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(device, &texInfo);
    if (!texture) {
        SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
        return nullptr;
    }

    // Upload data
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = static_cast<Uint32>(surface->w * surface->h * 4);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        SDL_ReleaseGPUTexture(device, texture);
        return nullptr;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped) {
        memcpy(mapped, surface->pixels, transferInfo.size);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer = transfer;
    srcInfo.offset = 0;

    SDL_GPUTextureRegion dstRegion = {};
    dstRegion.texture = texture;
    dstRegion.w = static_cast<Uint32>(surface->w);
    dstRegion.h = static_cast<Uint32>(surface->h);
    dstRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &srcInfo, &dstRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return texture;
}

SDL_GPUSampler* TextureLoader::createSampler() {
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

    return SDL_CreateGPUSampler(m_window.getDevice(), &samplerInfo);
}

} // namespace sims3000
