/**
 * @file GPUMesh.cpp
 * @brief GPU mesh representation implementation.
 *
 * Converts ModelLoader data into GPU-ready structures with resolved texture handles.
 */

#include "sims3000/render/GPUMesh.h"
#include "sims3000/assets/ModelLoader.h"
#include "sims3000/assets/TextureLoader.h"

#include <SDL3/SDL_log.h>

namespace sims3000 {

// =============================================================================
// ModelAsset Implementation
// =============================================================================

ModelAsset ModelAsset::fromModel(ModelHandle model, TextureLoader& textureLoader) {
    ModelAsset asset;

    if (!model) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "ModelAsset::fromModel: null model handle");
        return asset;
    }

    asset.sourceModel = model;

    // Initialize combined bounds to empty
    asset.bounds = AABB::empty();

    // Convert materials with texture resolution
    asset.materials.reserve(model->materials.size());
    for (const auto& srcMat : model->materials) {
        GPUMaterial gpuMat;
        gpuMat.name = srcMat.name;

        // Base color / diffuse
        gpuMat.baseColorFactor = srcMat.baseColorFactor;
        if (!srcMat.baseColorTexturePath.empty()) {
            gpuMat.diffuseTexture = textureLoader.load(srcMat.baseColorTexturePath);
            if (gpuMat.diffuseTexture) {
                textureLoader.addRef(gpuMat.diffuseTexture);
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Failed to load diffuse texture: %s",
                            srcMat.baseColorTexturePath.c_str());
            }
        }

        // Emissive
        gpuMat.emissiveColor = srcMat.emissiveFactor;
        if (!srcMat.emissiveTexturePath.empty()) {
            gpuMat.emissiveTexture = textureLoader.load(srcMat.emissiveTexturePath);
            if (gpuMat.emissiveTexture) {
                textureLoader.addRef(gpuMat.emissiveTexture);
            } else {
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                            "Failed to load emissive texture: %s",
                            srcMat.emissiveTexturePath.c_str());
            }
        }

        // Metallic-roughness
        gpuMat.metallicFactor = srcMat.metallicFactor;
        gpuMat.roughnessFactor = srcMat.roughnessFactor;
        if (!srcMat.metallicRoughnessTexturePath.empty()) {
            gpuMat.metallicRoughnessTexture = textureLoader.load(srcMat.metallicRoughnessTexturePath);
            if (gpuMat.metallicRoughnessTexture) {
                textureLoader.addRef(gpuMat.metallicRoughnessTexture);
            }
        }

        // Normal map
        gpuMat.normalScale = srcMat.normalScale;
        if (!srcMat.normalTexturePath.empty()) {
            gpuMat.normalTexture = textureLoader.load(srcMat.normalTexturePath);
            if (gpuMat.normalTexture) {
                textureLoader.addRef(gpuMat.normalTexture);
            }
        }

        // Alpha mode
        switch (srcMat.alphaMode) {
            case Material::AlphaMode::Opaque:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Opaque;
                break;
            case Material::AlphaMode::Mask:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Mask;
                break;
            case Material::AlphaMode::Blend:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Blend;
                break;
        }
        gpuMat.alphaCutoff = srcMat.alphaCutoff;
        gpuMat.doubleSided = srcMat.doubleSided;

        asset.materials.push_back(std::move(gpuMat));
    }

    // Convert meshes
    asset.meshes.reserve(model->meshes.size());
    for (const auto& srcMesh : model->meshes) {
        GPUMesh gpuMesh;
        gpuMesh.vertexBuffer = srcMesh.vertexBuffer;
        gpuMesh.indexBuffer = srcMesh.indexBuffer;
        gpuMesh.vertexCount = srcMesh.vertexCount;
        gpuMesh.indexCount = srcMesh.indexCount;
        gpuMesh.materialIndex = srcMesh.materialIndex;

        // Per-mesh bounds: use overall model bounds for now
        // (ModelLoader computes bounds per-vertex during load, stored in model)
        gpuMesh.bounds.min = model->boundsMin;
        gpuMesh.bounds.max = model->boundsMax;

        asset.meshes.push_back(gpuMesh);
    }

    // Set overall bounds from model
    asset.bounds.min = model->boundsMin;
    asset.bounds.max = model->boundsMax;

    SDL_Log("Created ModelAsset: %zu meshes, %zu materials from %s",
            asset.meshes.size(), asset.materials.size(), model->path.c_str());

    return asset;
}

ModelAsset ModelAsset::fromModelNoTextures(ModelHandle model) {
    ModelAsset asset;

    if (!model) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "ModelAsset::fromModelNoTextures: null model handle");
        return asset;
    }

    asset.sourceModel = model;

    // Initialize combined bounds to empty
    asset.bounds = AABB::empty();

    // Convert materials without texture resolution
    asset.materials.reserve(model->materials.size());
    for (const auto& srcMat : model->materials) {
        GPUMaterial gpuMat;
        gpuMat.name = srcMat.name;

        // Base color / diffuse (no texture, factor only)
        gpuMat.baseColorFactor = srcMat.baseColorFactor;
        gpuMat.diffuseTexture = nullptr;

        // Emissive (no texture, color only)
        gpuMat.emissiveColor = srcMat.emissiveFactor;
        gpuMat.emissiveTexture = nullptr;

        // Metallic-roughness
        gpuMat.metallicFactor = srcMat.metallicFactor;
        gpuMat.roughnessFactor = srcMat.roughnessFactor;
        gpuMat.metallicRoughnessTexture = nullptr;

        // Normal map
        gpuMat.normalScale = srcMat.normalScale;
        gpuMat.normalTexture = nullptr;

        // Alpha mode
        switch (srcMat.alphaMode) {
            case Material::AlphaMode::Opaque:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Opaque;
                break;
            case Material::AlphaMode::Mask:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Mask;
                break;
            case Material::AlphaMode::Blend:
                gpuMat.alphaMode = GPUMaterial::AlphaMode::Blend;
                break;
        }
        gpuMat.alphaCutoff = srcMat.alphaCutoff;
        gpuMat.doubleSided = srcMat.doubleSided;

        asset.materials.push_back(std::move(gpuMat));
    }

    // Convert meshes
    asset.meshes.reserve(model->meshes.size());
    for (const auto& srcMesh : model->meshes) {
        GPUMesh gpuMesh;
        gpuMesh.vertexBuffer = srcMesh.vertexBuffer;
        gpuMesh.indexBuffer = srcMesh.indexBuffer;
        gpuMesh.vertexCount = srcMesh.vertexCount;
        gpuMesh.indexCount = srcMesh.indexCount;
        gpuMesh.materialIndex = srcMesh.materialIndex;

        // Per-mesh bounds
        gpuMesh.bounds.min = model->boundsMin;
        gpuMesh.bounds.max = model->boundsMax;

        asset.meshes.push_back(gpuMesh);
    }

    // Set overall bounds from model
    asset.bounds.min = model->boundsMin;
    asset.bounds.max = model->boundsMax;

    return asset;
}

void ModelAsset::releaseTextures(TextureLoader& textureLoader) {
    for (auto& mat : materials) {
        if (mat.diffuseTexture) {
            textureLoader.release(mat.diffuseTexture);
            mat.diffuseTexture = nullptr;
        }
        if (mat.emissiveTexture) {
            textureLoader.release(mat.emissiveTexture);
            mat.emissiveTexture = nullptr;
        }
        if (mat.metallicRoughnessTexture) {
            textureLoader.release(mat.metallicRoughnessTexture);
            mat.metallicRoughnessTexture = nullptr;
        }
        if (mat.normalTexture) {
            textureLoader.release(mat.normalTexture);
            mat.normalTexture = nullptr;
        }
    }
}

void ModelAsset::reloadTextures(TextureLoader& textureLoader) {
    if (!sourceModel) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "ModelAsset::reloadTextures: no source model");
        return;
    }

    // Release current textures
    releaseTextures(textureLoader);

    // Reload from source material paths
    for (std::size_t i = 0; i < materials.size() && i < sourceModel->materials.size(); ++i) {
        const auto& srcMat = sourceModel->materials[i];
        auto& gpuMat = materials[i];

        if (!srcMat.baseColorTexturePath.empty()) {
            gpuMat.diffuseTexture = textureLoader.load(srcMat.baseColorTexturePath);
            if (gpuMat.diffuseTexture) {
                textureLoader.addRef(gpuMat.diffuseTexture);
            }
        }

        if (!srcMat.emissiveTexturePath.empty()) {
            gpuMat.emissiveTexture = textureLoader.load(srcMat.emissiveTexturePath);
            if (gpuMat.emissiveTexture) {
                textureLoader.addRef(gpuMat.emissiveTexture);
            }
        }

        if (!srcMat.metallicRoughnessTexturePath.empty()) {
            gpuMat.metallicRoughnessTexture = textureLoader.load(srcMat.metallicRoughnessTexturePath);
            if (gpuMat.metallicRoughnessTexture) {
                textureLoader.addRef(gpuMat.metallicRoughnessTexture);
            }
        }

        if (!srcMat.normalTexturePath.empty()) {
            gpuMat.normalTexture = textureLoader.load(srcMat.normalTexturePath);
            if (gpuMat.normalTexture) {
                textureLoader.addRef(gpuMat.normalTexture);
            }
        }
    }

    SDL_Log("Reloaded textures for ModelAsset: %s", sourceModel->path.c_str());
}

} // namespace sims3000
