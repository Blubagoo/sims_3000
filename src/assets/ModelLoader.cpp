/**
 * @file ModelLoader.cpp
 * @brief ModelLoader implementation with cgltf.
 *
 * Loads glTF 2.0 models (.gltf JSON and .glb binary formats).
 * Extracts mesh geometry and material properties including:
 * - Base color (albedo/diffuse) texture and factor
 * - Emissive texture and factor
 * - Metallic-roughness properties
 */

#include "sims3000/assets/ModelLoader.h"
#include "sims3000/render/Window.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_filesystem.h>
#include <algorithm>

namespace sims3000 {

// Helper to get texture path from cgltf_texture
static std::string getTexturePath(const cgltf_texture* texture,
                                   const std::string& modelDirectory) {
    if (!texture || !texture->image || !texture->image->uri) {
        return "";
    }

    const char* uri = texture->image->uri;

    // Check for data URI (embedded texture)
    if (std::strncmp(uri, "data:", 5) == 0) {
        // Data URIs are embedded - we don't support loading them here
        // The texture data would need to be extracted separately
        return "";
    }

    // Relative path - resolve against model directory
    std::string resolved = modelDirectory;
    if (!resolved.empty() && resolved.back() != '/' && resolved.back() != '\\') {
        resolved += '/';
    }
    resolved += uri;

    // Normalize path separators
    std::replace(resolved.begin(), resolved.end(), '\\', '/');

    return resolved;
}

ModelLoader::ModelLoader(Window& window)
    : m_window(window)
{
}

ModelLoader::~ModelLoader() {
    clearAll();
}

ModelHandle ModelLoader::load(const std::string& path) {
    m_lastError.clear();

    // Check cache
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        it->second.refCount++;
        return &it->second;
    }

    // Parse GLTF/GLB
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
    if (result != cgltf_result_success) {
        m_lastError = "Failed to parse glTF file: " + path;
        if (result == cgltf_result_file_not_found) {
            m_lastError += " (file not found)";
        } else if (result == cgltf_result_io_error) {
            m_lastError += " (I/O error)";
        } else if (result == cgltf_result_invalid_json) {
            m_lastError += " (invalid JSON)";
        } else if (result == cgltf_result_invalid_gltf) {
            m_lastError += " (invalid glTF)";
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", m_lastError.c_str());
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        m_lastError = "Failed to load glTF buffers: " + path;
        if (result == cgltf_result_file_not_found) {
            m_lastError += " (external buffer file not found)";
        } else if (result == cgltf_result_io_error) {
            m_lastError += " (I/O error reading buffer)";
        }
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", m_lastError.c_str());
        cgltf_free(data);
        return nullptr;
    }

    // Create model
    Model model;
    model.path = path;
    model.directory = getDirectory(path);
    model.refCount = 1;

    // Get file modification time
    SDL_PathInfo info;
    if (SDL_GetPathInfo(path.c_str(), &info)) {
        model.lastModified = static_cast<std::uint64_t>(info.modify_time);
    }

    // =========================================================================
    // Extract materials
    // =========================================================================
    for (size_t mi = 0; mi < data->materials_count; ++mi) {
        cgltf_material& gltfMat = data->materials[mi];
        Material mat;

        // Material name
        if (gltfMat.name) {
            mat.name = gltfMat.name;
        }

        // PBR Metallic Roughness workflow
        if (gltfMat.has_pbr_metallic_roughness) {
            cgltf_pbr_metallic_roughness& pbr = gltfMat.pbr_metallic_roughness;

            // Base color texture
            if (pbr.base_color_texture.texture) {
                mat.baseColorTexturePath = getTexturePath(
                    pbr.base_color_texture.texture, model.directory);
            }

            // Base color factor
            mat.baseColorFactor = glm::vec4(
                pbr.base_color_factor[0],
                pbr.base_color_factor[1],
                pbr.base_color_factor[2],
                pbr.base_color_factor[3]
            );

            // Metallic-roughness texture
            if (pbr.metallic_roughness_texture.texture) {
                mat.metallicRoughnessTexturePath = getTexturePath(
                    pbr.metallic_roughness_texture.texture, model.directory);
            }

            // Metallic and roughness factors
            mat.metallicFactor = pbr.metallic_factor;
            mat.roughnessFactor = pbr.roughness_factor;
        }

        // Emissive texture
        if (gltfMat.emissive_texture.texture) {
            mat.emissiveTexturePath = getTexturePath(
                gltfMat.emissive_texture.texture, model.directory);
        }

        // Emissive factor
        mat.emissiveFactor = glm::vec3(
            gltfMat.emissive_factor[0],
            gltfMat.emissive_factor[1],
            gltfMat.emissive_factor[2]
        );

        // Normal texture
        if (gltfMat.normal_texture.texture) {
            mat.normalTexturePath = getTexturePath(
                gltfMat.normal_texture.texture, model.directory);
            mat.normalScale = gltfMat.normal_texture.scale;
        }

        // Alpha mode
        switch (gltfMat.alpha_mode) {
            case cgltf_alpha_mode_opaque:
                mat.alphaMode = Material::AlphaMode::Opaque;
                break;
            case cgltf_alpha_mode_mask:
                mat.alphaMode = Material::AlphaMode::Mask;
                mat.alphaCutoff = gltfMat.alpha_cutoff;
                break;
            case cgltf_alpha_mode_blend:
                mat.alphaMode = Material::AlphaMode::Blend;
                break;
        }

        mat.doubleSided = gltfMat.double_sided;

        model.materials.push_back(std::move(mat));
    }

    // =========================================================================
    // Process meshes
    // =========================================================================
    for (size_t m = 0; m < data->meshes_count; ++m) {
        cgltf_mesh& gltfMesh = data->meshes[m];

        for (size_t p = 0; p < gltfMesh.primitives_count; ++p) {
            cgltf_primitive& prim = gltfMesh.primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) continue;

            std::vector<Vertex> vertices;
            std::vector<std::uint32_t> indices;

            // Get accessors
            cgltf_accessor* posAccessor = nullptr;
            cgltf_accessor* normAccessor = nullptr;
            cgltf_accessor* uvAccessor = nullptr;
            cgltf_accessor* colorAccessor = nullptr;

            for (size_t a = 0; a < prim.attributes_count; ++a) {
                if (prim.attributes[a].type == cgltf_attribute_type_position) {
                    posAccessor = prim.attributes[a].data;
                } else if (prim.attributes[a].type == cgltf_attribute_type_normal) {
                    normAccessor = prim.attributes[a].data;
                } else if (prim.attributes[a].type == cgltf_attribute_type_texcoord) {
                    uvAccessor = prim.attributes[a].data;
                } else if (prim.attributes[a].type == cgltf_attribute_type_color) {
                    colorAccessor = prim.attributes[a].data;
                }
            }

            if (!posAccessor) continue;

            // Read vertices
            size_t vertexCount = posAccessor->count;
            vertices.resize(vertexCount);

            for (size_t v = 0; v < vertexCount; ++v) {
                Vertex& vert = vertices[v];

                // Position
                cgltf_accessor_read_float(posAccessor, v, &vert.position.x, 3);

                // Update bounds
                model.boundsMin = glm::min(model.boundsMin, vert.position);
                model.boundsMax = glm::max(model.boundsMax, vert.position);

                // Normal
                if (normAccessor) {
                    cgltf_accessor_read_float(normAccessor, v, &vert.normal.x, 3);
                } else {
                    vert.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                // UV
                if (uvAccessor) {
                    cgltf_accessor_read_float(uvAccessor, v, &vert.texCoord.x, 2);
                } else {
                    vert.texCoord = glm::vec2(0.0f);
                }

                // Vertex color
                if (colorAccessor) {
                    // glTF can have RGB or RGBA vertex colors
                    if (colorAccessor->type == cgltf_type_vec4) {
                        cgltf_accessor_read_float(colorAccessor, v, &vert.color.x, 4);
                    } else if (colorAccessor->type == cgltf_type_vec3) {
                        cgltf_accessor_read_float(colorAccessor, v, &vert.color.x, 3);
                        vert.color.w = 1.0f;
                    } else {
                        vert.color = glm::vec4(1.0f);
                    }
                } else {
                    vert.color = glm::vec4(1.0f);
                }
            }

            // Read indices
            if (prim.indices) {
                indices.resize(prim.indices->count);
                for (size_t i = 0; i < prim.indices->count; ++i) {
                    indices[i] = static_cast<std::uint32_t>(cgltf_accessor_read_index(prim.indices, i));
                }
            } else {
                // Generate indices
                indices.resize(vertexCount);
                for (size_t i = 0; i < vertexCount; ++i) {
                    indices[i] = static_cast<std::uint32_t>(i);
                }
            }

            // Create mesh
            Mesh mesh;
            mesh.vertexBuffer = createVertexBuffer(vertices);
            mesh.indexBuffer = createIndexBuffer(indices);
            mesh.vertexCount = static_cast<std::uint32_t>(vertices.size());
            mesh.indexCount = static_cast<std::uint32_t>(indices.size());

            // Link material
            if (prim.material) {
                // Find material index
                for (size_t mi = 0; mi < data->materials_count; ++mi) {
                    if (&data->materials[mi] == prim.material) {
                        mesh.materialIndex = static_cast<int>(mi);
                        break;
                    }
                }
            }

            if (mesh.vertexBuffer && mesh.indexBuffer) {
                model.meshes.push_back(mesh);
            } else {
                m_lastError = "Failed to create GPU buffers for mesh";
                destroyMesh(mesh);
            }
        }
    }

    cgltf_free(data);

    if (model.meshes.empty()) {
        m_lastError = "No valid meshes found in glTF file: " + path;
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", m_lastError.c_str());
        return nullptr;
    }

    SDL_Log("Loaded model: %s (%zu meshes, %zu materials)",
            path.c_str(), model.meshes.size(), model.materials.size());

    auto inserted = m_cache.emplace(path, std::move(model));
    return &inserted.first->second;
}

ModelHandle ModelLoader::createFallback() {
    // Create a unit cube
    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0}, {1, 0, 1, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1}, {1, 0, 1, 1}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1}, {1, 0, 1, 1}},
        // Back face
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0}, {1, 0, 1, 1}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0}, {1, 0, 1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1}, {1, 0, 1, 1}},
        // Top face
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1}, {1, 0, 1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1}, {1, 0, 1, 1}},
        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 0}, {1, 0, 1, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0}, {1, 0, 1, 1}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1}, {1, 0, 1, 1}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 1}, {1, 0, 1, 1}},
        // Right face
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0}, {1, 0, 1, 1}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1}, {1, 0, 1, 1}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1}, {1, 0, 1, 1}},
        // Left face
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0}, {1, 0, 1, 1}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0}, {1, 0, 1, 1}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1}, {1, 0, 1, 1}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1}, {1, 0, 1, 1}},
    };

    std::vector<std::uint32_t> indices;
    for (int face = 0; face < 6; ++face) {
        std::uint32_t base = face * 4;
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    const std::string fallbackPath = "__fallback_model__";

    Model model;
    model.path = fallbackPath;
    model.refCount = 1;
    model.boundsMin = glm::vec3(-0.5f);
    model.boundsMax = glm::vec3(0.5f);

    Mesh mesh;
    mesh.vertexBuffer = createVertexBuffer(vertices);
    mesh.indexBuffer = createIndexBuffer(indices);
    mesh.vertexCount = static_cast<std::uint32_t>(vertices.size());
    mesh.indexCount = static_cast<std::uint32_t>(indices.size());

    if (!mesh.vertexBuffer || !mesh.indexBuffer) {
        destroyMesh(mesh);
        return nullptr;
    }

    model.meshes.push_back(mesh);

    auto inserted = m_cache.emplace(fallbackPath, std::move(model));
    return &inserted.first->second;
}

void ModelLoader::addRef(ModelHandle handle) {
    if (handle) {
        handle->refCount++;
    }
}

void ModelLoader::release(ModelHandle handle) {
    if (handle && handle->refCount > 0) {
        handle->refCount--;
    }
}

void ModelLoader::clearUnused() {
    for (auto it = m_cache.begin(); it != m_cache.end(); ) {
        if (it->second.refCount == 0) {
            for (auto& mesh : it->second.meshes) {
                destroyMesh(mesh);
            }
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

void ModelLoader::clearAll() {
    for (auto& [path, model] : m_cache) {
        for (auto& mesh : model.meshes) {
            destroyMesh(mesh);
        }
    }
    m_cache.clear();
}

void ModelLoader::getStats(size_t& outCount, size_t& outBytes) const {
    outCount = m_cache.size();
    outBytes = 0;
    for (const auto& [path, model] : m_cache) {
        for (const auto& mesh : model.meshes) {
            outBytes += mesh.vertexCount * sizeof(Vertex);
            outBytes += mesh.indexCount * sizeof(std::uint32_t);
        }
    }
}

bool ModelLoader::reloadIfModified(ModelHandle handle) {
    if (!handle) return false;

    SDL_PathInfo info;
    if (!SDL_GetPathInfo(handle->path.c_str(), &info)) {
        return false;
    }

    if (static_cast<std::uint64_t>(info.modify_time) <= handle->lastModified) {
        return false;
    }

    // Store state
    std::string path = handle->path;
    int refCount = handle->refCount;

    // Remove old model
    for (auto& mesh : handle->meshes) {
        destroyMesh(mesh);
    }
    m_cache.erase(path);

    // Reload
    ModelHandle newHandle = load(path);
    if (newHandle) {
        newHandle->refCount = refCount;
        SDL_Log("Hot-reloaded model: %s", path.c_str());
        return true;
    }

    return false;
}

SDL_GPUBuffer* ModelLoader::createVertexBuffer(const std::vector<Vertex>& vertices) {
    auto device = m_window.getDevice();
    Uint32 size = static_cast<Uint32>(vertices.size() * sizeof(Vertex));

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = size;

    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!buffer) return nullptr;

    // Upload via transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, buffer);
        return nullptr;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped) {
        memcpy(mapped, vertices.data(), size);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = size;

    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return buffer;
}

SDL_GPUBuffer* ModelLoader::createIndexBuffer(const std::vector<std::uint32_t>& indices) {
    auto device = m_window.getDevice();
    Uint32 size = static_cast<Uint32>(indices.size() * sizeof(std::uint32_t));

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    bufferInfo.size = size;

    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &bufferInfo);
    if (!buffer) return nullptr;

    // Upload via transfer buffer
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = size;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, buffer);
        return nullptr;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped) {
        memcpy(mapped, indices.data(), size);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    src.offset = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer = buffer;
    dst.offset = 0;
    dst.size = size;

    SDL_UploadToGPUBuffer(copyPass, &src, &dst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return buffer;
}

void ModelLoader::destroyMesh(Mesh& mesh) {
    auto device = m_window.getDevice();
    if (mesh.vertexBuffer) {
        SDL_ReleaseGPUBuffer(device, mesh.vertexBuffer);
        mesh.vertexBuffer = nullptr;
    }
    if (mesh.indexBuffer) {
        SDL_ReleaseGPUBuffer(device, mesh.indexBuffer);
        mesh.indexBuffer = nullptr;
    }
}

std::string ModelLoader::getDirectory(const std::string& filepath) {
    // Find last separator
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return ".";
    }
    return filepath.substr(0, lastSlash);
}

std::string ModelLoader::resolveUri(const std::string& uri, const std::string& modelDirectory) {
    // Check for data URI
    if (uri.compare(0, 5, "data:") == 0) {
        return "";  // Data URIs not supported as file paths
    }

    // Check for absolute path
    if (!uri.empty() && (uri[0] == '/' || (uri.length() > 1 && uri[1] == ':'))) {
        return uri;
    }

    // Relative path - combine with model directory
    std::string resolved = modelDirectory;
    if (!resolved.empty() && resolved.back() != '/' && resolved.back() != '\\') {
        resolved += '/';
    }
    resolved += uri;

    // Normalize path separators
    std::replace(resolved.begin(), resolved.end(), '\\', '/');

    return resolved;
}

} // namespace sims3000
