#include "scene.h"
#include <algorithm>

namespace poc1 {

Scene::Scene(size_t instanceCount, size_t modelCount)
    : m_targetInstanceCount(instanceCount)
    , m_modelCount(modelCount < 1 ? 1 : modelCount)
{
    GenerateBuildings();
}

void Scene::GenerateBuildings() {
    m_instances.clear();
    m_modelGroups.clear();

    // Calculate grid size to achieve approximately the target instance count
    int gridSize = static_cast<int>(std::sqrt(static_cast<float>(m_targetInstanceCount)));
    if (gridSize < 1) {
        gridSize = 1;
    }

    // Temporary struct to hold instance + model index for sorting
    struct InstanceWithModel {
        InstanceData data;
        uint32_t modelIndex;
    };

    std::vector<InstanceWithModel> tempInstances;
    tempInstances.reserve(static_cast<size_t>(gridSize * gridSize));

    // Generate buildings in a grid pattern centered around origin
    float halfGrid = static_cast<float>(gridSize) / 2.0f;

    for (int z = 0; z < gridSize; z++) {
        for (int x = 0; x < gridSize; x++) {
            float posX = (static_cast<float>(x) - halfGrid + 0.5f) * m_spacing;
            float posY = 0.0f;
            float posZ = (static_cast<float>(z) - halfGrid + 0.5f) * m_spacing;

            InstanceWithModel inst;

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
            inst.data.modelMatrix = translation;
            inst.data.color = GenerateBuildingColor(x, z);

            // Deterministic model assignment based on position hash
            int hash = (x * 73856093) ^ (z * 19349663);
            hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
            inst.modelIndex = static_cast<uint32_t>(std::abs(hash) % static_cast<int>(m_modelCount));

            tempInstances.push_back(inst);
        }
    }

    // Sort by model index for contiguous batched draws
    std::sort(tempInstances.begin(), tempInstances.end(),
        [](const InstanceWithModel& a, const InstanceWithModel& b) {
            return a.modelIndex < b.modelIndex;
        });

    // Extract sorted instance data and build model groups
    m_instances.reserve(tempInstances.size());
    uint32_t currentModel = tempInstances.empty() ? 0 : tempInstances[0].modelIndex;
    uint32_t groupStart = 0;

    for (size_t i = 0; i < tempInstances.size(); ++i) {
        m_instances.push_back(tempInstances[i].data);

        // Check if we hit a new model group or last element
        bool isLast = (i == tempInstances.size() - 1);
        bool modelChanged = !isLast && tempInstances[i + 1].modelIndex != currentModel;

        if (isLast || modelChanged) {
            ModelGroup group;
            group.modelIndex = currentModel;
            group.firstInstance = groupStart;
            group.instanceCount = static_cast<uint32_t>(i) - groupStart + 1;
            m_modelGroups.push_back(group);

            if (!isLast) {
                currentModel = tempInstances[i + 1].modelIndex;
                groupStart = static_cast<uint32_t>(i + 1);
            }
        }
    }
}

void Scene::Update(float deltaTime) {
    m_time += deltaTime;
}

glm::vec4 Scene::GenerateBuildingColor(int x, int z) const {
    int hash = (x * 73856093) ^ (z * 19349663);
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
    hash = (hash >> 16) ^ hash;

    float normalizedHash = static_cast<float>(hash & 0xFFFF) / 65535.0f;

    float hue, saturation, value;

    int colorFamily = hash % 3;
    switch (colorFamily) {
        case 0:
            hue = 0.55f + normalizedHash * 0.1f;
            saturation = 0.2f + normalizedHash * 0.15f;
            value = 0.5f + normalizedHash * 0.3f;
            break;
        case 1:
            hue = normalizedHash * 0.1f;
            saturation = 0.05f + normalizedHash * 0.1f;
            value = 0.55f + normalizedHash * 0.25f;
            break;
        case 2:
        default:
            hue = 0.08f + normalizedHash * 0.04f;
            saturation = 0.25f + normalizedHash * 0.15f;
            value = 0.55f + normalizedHash * 0.25f;
            break;
    }

    glm::vec3 rgb = HSVtoRGB(hue, saturation, value);
    return glm::vec4(rgb, 1.0f);
}

glm::vec3 Scene::HSVtoRGB(float h, float s, float v) const {
    if (s <= 0.0f) {
        return glm::vec3(v, v, v);
    }

    float hh = h * 6.0f;
    if (hh >= 6.0f) hh = 0.0f;

    int i = static_cast<int>(hh);
    float ff = hh - static_cast<float>(i);
    float p = v * (1.0f - s);
    float q = v * (1.0f - (s * ff));
    float t = v * (1.0f - (s * (1.0f - ff)));

    switch (i) {
        case 0: return glm::vec3(v, t, p);
        case 1: return glm::vec3(q, v, p);
        case 2: return glm::vec3(p, v, t);
        case 3: return glm::vec3(p, q, v);
        case 4: return glm::vec3(t, p, v);
        case 5:
        default: return glm::vec3(v, p, q);
    }
}

} // namespace poc1
