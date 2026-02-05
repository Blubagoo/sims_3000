#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <glm/vec3.hpp>
#include <vector>
#include <cstdint>

/**
 * Vertex - Represents a single vertex with position and normal.
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

/**
 * MeshData - Contains vertex and index data for a mesh.
 *
 * Loaded from glTF/glb files using LoadModel().
 * Can represent both indexed and non-indexed meshes.
 */
struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

/**
 * Load a 3D model from a glTF or glb file.
 *
 * Extracts the first mesh's positions, normals, and indices.
 * Handles both indexed and non-indexed meshes.
 *
 * @param path Path to the .gltf or .glb file
 * @return MeshData containing vertices and indices, empty on failure
 */
MeshData LoadModel(const char* path);

#endif // MODEL_LOADER_H
