/**
 * @file ModelLoader.cpp
 * @brief ModelLoader implementation with cgltf.
 */

#include "sims3000/assets/ModelLoader.h"
#include "sims3000/render/Window.h"

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_filesystem.h>

namespace sims3000 {

ModelLoader::ModelLoader(Window& window)
    : m_window(window)
{
}

ModelLoader::~ModelLoader() {
    clearAll();
}

ModelHandle ModelLoader::load(const std::string& path) {
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
        SDL_Log("Failed to parse GLTF: %s", path.c_str());
        return nullptr;
    }

    result = cgltf_load_buffers(&options, data, path.c_str());
    if (result != cgltf_result_success) {
        cgltf_free(data);
        SDL_Log("Failed to load GLTF buffers: %s", path.c_str());
        return nullptr;
    }

    // Create model
    Model model;
    model.path = path;
    model.refCount = 1;

    // Process meshes
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

            for (size_t a = 0; a < prim.attributes_count; ++a) {
                if (prim.attributes[a].type == cgltf_attribute_type_position) {
                    posAccessor = prim.attributes[a].data;
                } else if (prim.attributes[a].type == cgltf_attribute_type_normal) {
                    normAccessor = prim.attributes[a].data;
                } else if (prim.attributes[a].type == cgltf_attribute_type_texcoord) {
                    uvAccessor = prim.attributes[a].data;
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

                // Default color
                vert.color = glm::vec4(1.0f);
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

            if (mesh.vertexBuffer && mesh.indexBuffer) {
                model.meshes.push_back(mesh);
            } else {
                destroyMesh(mesh);
            }
        }
    }

    cgltf_free(data);

    if (model.meshes.empty()) {
        SDL_Log("No valid meshes in GLTF: %s", path.c_str());
        return nullptr;
    }

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

} // namespace sims3000
