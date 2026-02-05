#pragma once

#include "instance_buffer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>

namespace poc1 {

/**
 * A contiguous range of instances that share the same model.
 * Instances are sorted by model index so each group is a contiguous slice.
 */
struct ModelGroup {
    uint32_t modelIndex;     // Index into the meshes vector
    uint32_t firstInstance;  // Offset into the instance buffer
    uint32_t instanceCount;  // Number of instances for this model
};

/**
 * Scene manager that generates and manages building instances.
 * Creates a grid of buildings for instanced rendering.
 */
class Scene {
public:
    /**
     * Creates a scene with the specified number of building instances.
     * @param instanceCount Target number of instances (actual count may vary due to grid rounding)
     * @param modelCount Number of different models available
     */
    explicit Scene(size_t instanceCount = 1000, size_t modelCount = 1);

    /**
     * Generates buildings in a grid pattern.
     * Buildings are centered around the origin with uniform spacing.
     */
    void GenerateBuildings();

    /**
     * Gets the vector of instance data for rendering.
     * Instances are sorted by model index for batched rendering.
     * @return Const reference to the instance data vector
     */
    const std::vector<InstanceData>& GetInstances() const { return m_instances; }

    /**
     * Gets the model groups for batched rendering.
     * Each group identifies a contiguous range of instances sharing the same model.
     * @return Const reference to the model groups vector
     */
    const std::vector<ModelGroup>& GetModelGroups() const { return m_modelGroups; }

    /**
     * Gets the current number of building instances.
     * @return Number of instances
     */
    size_t GetInstanceCount() const { return m_instances.size(); }

    /**
     * Updates the scene (optional animation/movement).
     * @param deltaTime Time elapsed since last update in seconds
     */
    void Update(float deltaTime);

private:
    std::vector<InstanceData> m_instances;
    std::vector<ModelGroup> m_modelGroups;
    size_t m_targetInstanceCount;
    size_t m_modelCount;
    float m_spacing = 1.2f;  // Units between buildings
    float m_time = 0.0f;     // Accumulated time for animations

    glm::vec4 GenerateBuildingColor(int x, int z) const;
    glm::vec3 HSVtoRGB(float h, float s, float v) const;
};

} // namespace poc1
