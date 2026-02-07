/**
 * @file ModelLoader.h
 * @brief GLTF/GLB model loading with material extraction.
 *
 * Loads glTF 2.0 models (.gltf JSON format and .glb binary format).
 * Extracts mesh data (positions, normals, UVs, indices) and material data
 * (base color texture, emissive texture, emissive factor).
 *
 * Resource ownership:
 * - ModelLoader owns all SDL_GPUBuffer instances for vertex/index data
 * - Material texture paths are references (textures loaded separately via TextureLoader)
 * - Destruction order: release model references -> clearAll/clearUnused
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
 * @struct Material
 * @brief Material data extracted from glTF.
 *
 * Contains texture references and material properties.
 * Texture paths are relative to the model file location.
 */
struct Material {
    std::string name;                          ///< Material name from glTF

    // Base color (albedo/diffuse)
    std::string baseColorTexturePath;          ///< Path to base color texture, empty if none
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};  ///< RGBA multiplier

    // Emissive
    std::string emissiveTexturePath;           ///< Path to emissive texture, empty if none
    glm::vec3 emissiveFactor{0.0f, 0.0f, 0.0f}; ///< RGB emissive strength

    // Metallic-roughness (for future PBR support)
    std::string metallicRoughnessTexturePath;  ///< Path to metallic-roughness texture
    float metallicFactor = 1.0f;               ///< Metallic multiplier (0-1)
    float roughnessFactor = 1.0f;              ///< Roughness multiplier (0-1)

    // Normal mapping (for future support)
    std::string normalTexturePath;             ///< Path to normal map texture
    float normalScale = 1.0f;                  ///< Normal map intensity

    // Alpha mode
    enum class AlphaMode { Opaque, Mask, Blend };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;                  ///< Cutoff for AlphaMode::Mask

    bool doubleSided = false;                  ///< Render both faces
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
    int materialIndex = -1;                    ///< Index into Model::materials, -1 if no material
};

/**
 * @struct Model
 * @brief Complete 3D model with meshes and materials.
 */
struct Model {
    std::vector<Mesh> meshes;
    std::vector<Material> materials;           ///< Materials referenced by meshes
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    int refCount = 0;
    std::string path;
    std::string directory;                     ///< Directory containing the model (for texture resolution)
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

    /**
     * Get the last error message.
     * @return Error string from last failed operation
     */
    const std::string& getLastError() const { return m_lastError; }

private:
    SDL_GPUBuffer* createVertexBuffer(const std::vector<Vertex>& vertices);
    SDL_GPUBuffer* createIndexBuffer(const std::vector<std::uint32_t>& indices);
    void destroyMesh(Mesh& mesh);

    /**
     * Extract directory path from a file path.
     * @param filepath Full file path
     * @return Directory containing the file
     */
    static std::string getDirectory(const std::string& filepath);

    /**
     * Resolve a URI relative to the model's directory.
     * Handles both relative paths and data URIs.
     * @param uri The URI from the glTF file
     * @param modelDirectory Directory containing the model
     * @return Resolved absolute path, or empty string for data URIs
     */
    static std::string resolveUri(const std::string& uri, const std::string& modelDirectory);

    Window& m_window;
    std::unordered_map<std::string, Model> m_cache;
    std::string m_lastError;
};

} // namespace sims3000

#endif // SIMS3000_ASSETS_MODELLOADER_H
