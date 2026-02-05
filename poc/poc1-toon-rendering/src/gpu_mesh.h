#ifndef GPU_MESH_H
#define GPU_MESH_H

#include <SDL3/SDL.h>
#include <cstdint>

struct MeshData;

/**
 * GPUMesh - Manages GPU buffers for mesh rendering.
 *
 * Uploads vertex and index data to the GPU and provides
 * access to the buffers for rendering operations.
 */
class GPUMesh {
public:
    /**
     * Construct a GPUMesh from mesh data.
     *
     * Creates vertex and index buffers on the GPU and uploads the data.
     * For non-indexed meshes (empty indices), only vertex buffer is created.
     *
     * @param device The SDL GPU device for buffer creation
     * @param meshData The mesh data to upload
     */
    GPUMesh(SDL_GPUDevice* device, const MeshData& meshData);

    /**
     * Destructor - Releases GPU buffers.
     */
    ~GPUMesh();

    // Non-copyable
    GPUMesh(const GPUMesh&) = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;

    // Movable
    GPUMesh(GPUMesh&& other) noexcept;
    GPUMesh& operator=(GPUMesh&& other) noexcept;

    /**
     * Get the vertex buffer.
     * @return Pointer to SDL_GPUBuffer for vertices, or nullptr if not created
     */
    SDL_GPUBuffer* GetVertexBuffer() const;

    /**
     * Get the index buffer.
     * @return Pointer to SDL_GPUBuffer for indices, or nullptr for non-indexed mesh
     */
    SDL_GPUBuffer* GetIndexBuffer() const;

    /**
     * Get the number of indices.
     * @return Index count, or 0 for non-indexed meshes
     */
    uint32_t GetIndexCount() const;

    /**
     * Get the number of vertices.
     * @return Vertex count
     */
    uint32_t GetVertexCount() const;

    /**
     * Check if the mesh is indexed.
     * @return true if the mesh has an index buffer
     */
    bool IsIndexed() const;

    /**
     * Check if the GPU buffers were created successfully.
     * @return true if buffers are valid and ready for use
     */
    bool IsValid() const;

private:
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUBuffer* m_vertexBuffer = nullptr;
    SDL_GPUBuffer* m_indexBuffer = nullptr;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
};

#endif // GPU_MESH_H
