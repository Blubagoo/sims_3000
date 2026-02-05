#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include "model_loader.h"
#include <SDL3/SDL.h>

/**
 * Helper to read accessor data at a given index.
 */
static glm::vec3 ReadVec3FromAccessor(const cgltf_accessor* accessor, size_t index) {
    glm::vec3 result(0.0f);

    if (accessor && index < accessor->count) {
        cgltf_accessor_read_float(accessor, index, &result.x, 3);
    }

    return result;
}

/**
 * Helper to read index from accessor.
 */
static uint32_t ReadIndexFromAccessor(const cgltf_accessor* accessor, size_t index) {
    if (!accessor || index >= accessor->count) {
        return 0;
    }

    cgltf_uint value = 0;
    cgltf_accessor_read_uint(accessor, index, &value, 1);
    return static_cast<uint32_t>(value);
}

MeshData LoadModel(const char* path) {
    MeshData meshData;

    if (!path) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "LoadModel: path is null");
        return meshData;
    }

    // Parse the glTF file
    cgltf_options options = {};
    cgltf_data* data = nullptr;

    cgltf_result parseResult = cgltf_parse_file(&options, path, &data);
    if (parseResult != cgltf_result_success) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "LoadModel: Failed to parse '%s' (error %d)", path, parseResult);
        return meshData;
    }

    // Load the binary buffer data
    cgltf_result loadResult = cgltf_load_buffers(&options, data, path);
    if (loadResult != cgltf_result_success) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "LoadModel: Failed to load buffers for '%s' (error %d)", path, loadResult);
        cgltf_free(data);
        return meshData;
    }

    if (data->meshes_count == 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "LoadModel: No meshes found in '%s'", path);
        cgltf_free(data);
        return meshData;
    }

    // Load ALL primitives from ALL meshes, concatenating vertex/index data
    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        cgltf_mesh* mesh = &data->meshes[mi];

        for (size_t pi = 0; pi < mesh->primitives_count; ++pi) {
            cgltf_primitive* primitive = &mesh->primitives[pi];

            // Find position and normal accessors
            const cgltf_accessor* positionAccessor = nullptr;
            const cgltf_accessor* normalAccessor = nullptr;

            for (size_t i = 0; i < primitive->attributes_count; ++i) {
                cgltf_attribute* attr = &primitive->attributes[i];
                if (attr->type == cgltf_attribute_type_position) {
                    positionAccessor = attr->data;
                } else if (attr->type == cgltf_attribute_type_normal) {
                    normalAccessor = attr->data;
                }
            }

            if (!positionAccessor) {
                continue; // Skip primitives without positions
            }

            // Track the base vertex index for this primitive's indices
            uint32_t baseVertex = static_cast<uint32_t>(meshData.vertices.size());

            // Extract vertex data
            size_t vertexCount = positionAccessor->count;
            for (size_t i = 0; i < vertexCount; ++i) {
                Vertex vertex;
                vertex.position = ReadVec3FromAccessor(positionAccessor, i);

                if (normalAccessor) {
                    vertex.normal = ReadVec3FromAccessor(normalAccessor, i);
                } else {
                    vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                }

                meshData.vertices.push_back(vertex);
            }

            // Extract indices, offset by baseVertex
            if (primitive->indices) {
                const cgltf_accessor* indexAccessor = primitive->indices;
                size_t indexCount = indexAccessor->count;
                for (size_t i = 0; i < indexCount; ++i) {
                    meshData.indices.push_back(baseVertex + ReadIndexFromAccessor(indexAccessor, i));
                }
            } else {
                // Non-indexed primitive: generate sequential indices
                for (uint32_t i = 0; i < vertexCount; ++i) {
                    meshData.indices.push_back(baseVertex + i);
                }
            }
        }
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "LoadModel: Loaded '%s' with %zu vertices and %zu indices (%zu meshes)",
                path, meshData.vertices.size(), meshData.indices.size(), data->meshes_count);

    cgltf_free(data);
    return meshData;
}
