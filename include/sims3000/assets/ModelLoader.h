/**
 * @file ModelLoader.h
 * @brief GLTF/GLB model loading.
 */

#ifndef SIMS3000_ASSETS_MODELLOADER_H
#define SIMS3000_ASSETS_MODELLOADER_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class Window;

/**
 * @struct Vertex
 * @brief Vertex format for 3D models.
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 color;
};

/**
 * @struct Mesh
 * @brief Single mesh within a model.
 */
struct Mesh {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer = nullptr;
    std::uint32_t vertexCount = 0;
    std::uint32_t indexCount = 0;
    int materialIndex = -1;
};

/**
 * @struct Model
 * @brief Complete 3D model with meshes.
 */
struct Model {
    std::vector<Mesh> meshes;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    int refCount = 0;
    std::string path;
    std::uint64_t lastModified = 0;
};

/// Handle to a loaded model
using ModelHandle = Model*;

/**
 * @class ModelLoader
 * @brief Loads and caches 3D models from GLTF/GLB files.
 *
 * Uses cgltf for parsing. Creates GPU buffers for
 * vertex and index data.
 */
class ModelLoader {
public:
    /**
     * Create model loader.
     * @param window Window for GPU device access
     */
    explicit ModelLoader(Window& window);
    ~ModelLoader();

    // Non-copyable
    ModelLoader(const ModelLoader&) = delete;
    ModelLoader& operator=(const ModelLoader&) = delete;

    /**
     * Load model from file.
     * @param path Full path to GLTF/GLB file
     * @return Model handle, or nullptr on failure
     */
    ModelHandle load(const std::string& path);

    /**
     * Create fallback model (unit cube).
     * @return Fallback model handle
     */
    ModelHandle createFallback();

    /**
     * Increment reference count.
     * @param handle Model handle
     */
    void addRef(ModelHandle handle);

    /**
     * Decrement reference count.
     * @param handle Model handle
     */
    void release(ModelHandle handle);

    /**
     * Clear models with zero references.
     */
    void clearUnused();

    /**
     * Clear all models.
     */
    void clearAll();

    /**
     * Get cache statistics.
     * @param outCount Number of cached models
     * @param outBytes Total GPU memory used
     */
    void getStats(size_t& outCount, size_t& outBytes) const;

    /**
     * Reload a model if the file was modified.
     * @param handle Model to check
     * @return true if reloaded
     */
    bool reloadIfModified(ModelHandle handle);

private:
    SDL_GPUBuffer* createVertexBuffer(const std::vector<Vertex>& vertices);
    SDL_GPUBuffer* createIndexBuffer(const std::vector<std::uint32_t>& indices);
    void destroyMesh(Mesh& mesh);

    Window& m_window;
    std::unordered_map<std::string, Model> m_cache;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_MODELLOADER_H
