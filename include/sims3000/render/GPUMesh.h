/**
 * @file GPUMesh.h
 * @brief GPU-side mesh representation from loaded glTF data.
 *
 * Creates GPU vertex/index buffers from ModelLoader data, stores AABB for culling,
 * and manages texture references for rendering.
 *
 * Resource ownership:
 * - GPUMesh owns vertex and index buffer references (but not GPU memory - that's in ModelLoader)
 * - Texture handles are references (owned by TextureLoader)
 * - GPUMaterial references are to data within the parent ModelAsset
 * - ModelAsset aggregates multiple GPUMesh for multi-mesh models
 *
 * Usage:
 * @code
 *   // Load model via ModelLoader
 *   ModelHandle model = modelLoader.load("path/to/model.glb");
 *
 *   // Create ModelAsset with resolved textures
 *   ModelAsset asset = ModelAsset::fromModel(model, textureLoader);
 *
 *   // Access meshes for rendering
 *   for (const auto& mesh : asset.meshes) {
 *       // Bind mesh.vertexBuffer, mesh.indexBuffer
 *       // Bind mesh.material.diffuseTexture, mesh.material.emissiveTexture
 *       // Draw mesh.indexCount indices
 *   }
 * @endcode
 */

#ifndef SIMS3000_RENDER_GPU_MESH_H
#define SIMS3000_RENDER_GPU_MESH_H

#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace sims3000 {

// Forward declarations
class TextureLoader;
struct Model;
struct Material;
using ModelHandle = Model*;
struct Texture;
using TextureHandle = Texture*;

/**
 * @struct AABB
 * @brief Axis-Aligned Bounding Box for frustum culling.
 *
 * Stores the minimum and maximum corners of the bounding box
 * in model-local space. Transform by model matrix for world-space bounds.
 */
struct AABB {
    glm::vec3 min{0.0f};  ///< Minimum corner (lowest x, y, z)
    glm::vec3 max{0.0f};  ///< Maximum corner (highest x, y, z)

    /**
     * @brief Get the center point of the bounding box.
     * @return Center point in model-local space.
     */
    glm::vec3 center() const { return (min + max) * 0.5f; }

    /**
     * @brief Get the size (extent) of the bounding box.
     * @return Size vector (width, height, depth).
     */
    glm::vec3 size() const { return max - min; }

    /**
     * @brief Get the half-size (half-extents) of the bounding box.
     * @return Half-size vector.
     */
    glm::vec3 halfSize() const { return size() * 0.5f; }

    /**
     * @brief Check if the bounding box is valid (non-degenerate).
     * @return true if max >= min in all dimensions.
     */
    bool isValid() const {
        return max.x >= min.x && max.y >= min.y && max.z >= min.z;
    }

    /**
     * @brief Expand the bounding box to include a point.
     * @param point Point to include.
     */
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    /**
     * @brief Expand the bounding box to include another AABB.
     * @param other AABB to include.
     */
    void expand(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }

    /**
     * @brief Create an invalid/empty AABB for expansion.
     * @return AABB with min at +infinity and max at -infinity.
     */
    static AABB empty() {
        AABB aabb;
        aabb.min = glm::vec3(std::numeric_limits<float>::max());
        aabb.max = glm::vec3(std::numeric_limits<float>::lowest());
        return aabb;
    }
};

/**
 * @struct GPUMaterial
 * @brief GPU-ready material with resolved texture handles.
 *
 * Contains actual texture handles (not just paths) for direct binding
 * during rendering. Includes emissive properties for bioluminescent rendering.
 */
struct GPUMaterial {
    std::string name;                      ///< Material name from glTF

    // Diffuse/Base Color
    TextureHandle diffuseTexture = nullptr;  ///< Resolved diffuse/base color texture
    glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};  ///< RGBA multiplier

    // Emissive (for bioluminescent rendering)
    TextureHandle emissiveTexture = nullptr;  ///< Resolved emissive texture
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};  ///< RGB emissive strength/color

    // Metallic-Roughness (for future PBR support)
    TextureHandle metallicRoughnessTexture = nullptr;  ///< Metallic-roughness texture
    float metallicFactor = 1.0f;           ///< Metallic multiplier (0-1)
    float roughnessFactor = 1.0f;          ///< Roughness multiplier (0-1)

    // Normal mapping (for future support)
    TextureHandle normalTexture = nullptr; ///< Normal map texture
    float normalScale = 1.0f;              ///< Normal map intensity

    // Alpha mode
    enum class AlphaMode : std::uint8_t {
        Opaque,  ///< Fully opaque
        Mask,    ///< Alpha test (cutoff)
        Blend    ///< Alpha blending
    };
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;              ///< Cutoff for AlphaMode::Mask

    bool doubleSided = false;              ///< Render both faces

    /**
     * @brief Check if material has emissive properties.
     * @return true if emissive texture exists or emissive color is non-zero.
     */
    bool hasEmissive() const {
        return emissiveTexture != nullptr ||
               emissiveColor.r > 0.0f ||
               emissiveColor.g > 0.0f ||
               emissiveColor.b > 0.0f;
    }

    /**
     * @brief Check if material has diffuse texture.
     * @return true if diffuse texture exists.
     */
    bool hasDiffuseTexture() const {
        return diffuseTexture != nullptr;
    }
};

/**
 * @struct GPUMesh
 * @brief GPU-side mesh with vertex/index buffers and material.
 *
 * Represents a single renderable mesh primitive with:
 * - Vertex buffer (positions, normals, UVs, colors)
 * - Index buffer (triangle indices)
 * - Material reference (within parent ModelAsset)
 * - Per-mesh AABB for fine-grained culling
 */
struct GPUMesh {
    // GPU Buffers (references to buffers owned by ModelLoader)
    SDL_GPUBuffer* vertexBuffer = nullptr;   ///< Vertex buffer handle
    SDL_GPUBuffer* indexBuffer = nullptr;    ///< Index buffer handle

    // Counts
    std::uint32_t vertexCount = 0;           ///< Number of vertices
    std::uint32_t indexCount = 0;            ///< Number of indices (triangles * 3)

    // Material (index into parent ModelAsset::materials)
    std::int32_t materialIndex = -1;         ///< Material index, -1 if no material

    // Per-mesh bounding box
    AABB bounds;                             ///< Axis-aligned bounding box

    /**
     * @brief Check if mesh has valid GPU buffers.
     * @return true if both vertex and index buffers are valid.
     */
    bool isValid() const {
        return vertexBuffer != nullptr && indexBuffer != nullptr && indexCount > 0;
    }

    /**
     * @brief Check if mesh has a material assigned.
     * @return true if materialIndex is valid (>= 0).
     */
    bool hasMaterial() const {
        return materialIndex >= 0;
    }
};

/**
 * @struct ModelAsset
 * @brief Complete model with multiple meshes and resolved materials.
 *
 * Aggregates multiple GPUMesh objects for multi-mesh models (e.g., a building
 * with separate window, wall, and roof meshes). Materials are resolved to
 * actual texture handles rather than paths.
 *
 * ModelAsset does NOT own the underlying GPU resources - those are owned by
 * ModelLoader and TextureLoader. ModelAsset is a view/reference structure
 * for convenient rendering access.
 */
struct ModelAsset {
    // Meshes (one per primitive in the source model)
    std::vector<GPUMesh> meshes;             ///< All mesh primitives

    // Materials (resolved from Material paths to TextureHandles)
    std::vector<GPUMaterial> materials;      ///< All materials with resolved textures

    // Overall bounding box (union of all mesh bounds)
    AABB bounds;                             ///< Combined AABB for all meshes

    // Source reference
    ModelHandle sourceModel = nullptr;       ///< Reference to source Model in ModelLoader

    /**
     * @brief Get total index count across all meshes.
     * @return Sum of indexCount for all meshes.
     */
    std::uint32_t getTotalIndexCount() const {
        std::uint32_t total = 0;
        for (const auto& mesh : meshes) {
            total += mesh.indexCount;
        }
        return total;
    }

    /**
     * @brief Get total vertex count across all meshes.
     * @return Sum of vertexCount for all meshes.
     */
    std::uint32_t getTotalVertexCount() const {
        std::uint32_t total = 0;
        for (const auto& mesh : meshes) {
            total += mesh.vertexCount;
        }
        return total;
    }

    /**
     * @brief Check if model asset is valid and renderable.
     * @return true if at least one mesh is valid.
     */
    bool isValid() const {
        if (meshes.empty()) return false;
        for (const auto& mesh : meshes) {
            if (mesh.isValid()) return true;
        }
        return false;
    }

    /**
     * @brief Get material for a mesh by index.
     * @param meshIndex Index of the mesh.
     * @return Pointer to GPUMaterial, or nullptr if mesh has no material.
     */
    const GPUMaterial* getMeshMaterial(std::size_t meshIndex) const {
        if (meshIndex >= meshes.size()) return nullptr;
        int matIdx = meshes[meshIndex].materialIndex;
        if (matIdx < 0 || static_cast<std::size_t>(matIdx) >= materials.size()) {
            return nullptr;
        }
        return &materials[matIdx];
    }

    /**
     * @brief Create ModelAsset from a loaded Model with resolved textures.
     *
     * Loads all referenced textures via TextureLoader and creates GPUMaterial
     * instances with resolved TextureHandle references.
     *
     * @param model Source model from ModelLoader.
     * @param textureLoader TextureLoader for resolving texture paths.
     * @return Populated ModelAsset.
     */
    static ModelAsset fromModel(ModelHandle model, TextureLoader& textureLoader);

    /**
     * @brief Create ModelAsset from a loaded Model without loading textures.
     *
     * Creates GPUMesh and GPUMaterial structures but leaves texture handles
     * as nullptr. Useful for deferred texture loading or when textures are
     * managed separately.
     *
     * @param model Source model from ModelLoader.
     * @return ModelAsset with null texture handles.
     */
    static ModelAsset fromModelNoTextures(ModelHandle model);

    /**
     * @brief Release texture references.
     *
     * Decrements reference counts on all loaded textures via TextureLoader.
     * Call this when the ModelAsset is no longer needed.
     *
     * @param textureLoader TextureLoader that owns the textures.
     */
    void releaseTextures(TextureLoader& textureLoader);

    /**
     * @brief Reload textures from source material paths.
     *
     * Useful for hot-reload when texture files change on disk.
     *
     * @param textureLoader TextureLoader for loading textures.
     */
    void reloadTextures(TextureLoader& textureLoader);
};

} // namespace sims3000

#endif // SIMS3000_RENDER_GPU_MESH_H
