/**
 * @file ToonShader.h
 * @brief Toon shader data structures and configuration constants.
 *
 * Defines the C++ structures that match the HLSL uniform buffer layouts
 * for the toon shader pipeline. These structures must be kept in sync
 * with the shader definitions in toon.vert.hlsl and toon.frag.hlsl.
 *
 * Shader Resource Bindings:
 * - Vertex uniform buffer 0 (space1): ViewProjectionUBO
 * - Vertex storage buffer 0 (space0): InstanceData[]
 * - Fragment uniform buffer 0 (space3): LightingUBO
 *
 * Alignment Notes:
 * - All structures use 16-byte aligned vectors (glm types)
 * - Padding is explicit to match HLSL cbuffer layout rules
 * - sizeof() assertions verify layout correctness
 */

#ifndef SIMS3000_RENDER_TOON_SHADER_H
#define SIMS3000_RENDER_TOON_SHADER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>

namespace sims3000 {

/**
 * @struct ToonViewProjectionUBO
 * @brief View-projection matrix uniform buffer for vertex shader.
 *
 * Bound to vertex uniform slot 0 (HLSL: register(b0, space1))
 * Includes light view-projection matrix for shadow mapping.
 */
struct ToonViewProjectionUBO {
    glm::mat4 viewProjection{1.0f};       ///< Camera view-projection matrix
    glm::mat4 lightViewProjection{1.0f};  ///< Light view-projection for shadow mapping
};
static_assert(sizeof(ToonViewProjectionUBO) == 128, "ToonViewProjectionUBO must be 128 bytes");

/**
 * @struct ToonInstanceData
 * @brief Per-instance rendering data for instanced toon rendering.
 *
 * Stored in structured buffer (HLSL: register(t0, space0))
 * Each instance has its own model matrix, colors, and emissive properties.
 */
struct ToonInstanceData {
    glm::mat4 model{1.0f};               // 64 bytes: Model transformation matrix
    glm::vec4 baseColor{1.0f};           // 16 bytes: Base diffuse color (RGB) + alpha
    glm::vec4 emissiveColor{0.0f};       // 16 bytes: Emissive color (RGB) + intensity
    float ambientStrength{0.0f};         //  4 bytes: Per-instance ambient override (0 = use global)
    float _padding[3]{0.0f, 0.0f, 0.0f}; // 12 bytes: Padding for 16-byte alignment
};
static_assert(sizeof(ToonInstanceData) == 112, "ToonInstanceData must be 112 bytes");

/**
 * @struct ToonLightingUBO
 * @brief Lighting parameters uniform buffer for fragment shader.
 *
 * Bound to fragment uniform slot 0 (HLSL: register(b0, space3))
 * Contains world-space sun direction, color shift parameters, and shadow settings.
 */
struct ToonLightingUBO {
    glm::vec3 sunDirection{0.408f, 0.816f, 0.408f}; // 12 bytes: Normalized light direction (default: (1,2,1) normalized)
    float globalAmbient{0.08f};                      //  4 bytes: Global ambient strength (0.05-0.1 recommended)
    glm::vec3 ambientColor{0.6f, 0.65f, 0.8f};      // 12 bytes: Ambient color (cool blue-ish for alien environment)
    float shadowEnabled{0.0f};                       //  4 bytes: 1.0 if shadows enabled, 0.0 if disabled
    glm::vec3 deepShadowColor{0.165f, 0.106f, 0.239f}; // 12 bytes: Deep shadow tint (#2A1B3D)
    float shadowIntensity{0.6f};                        //  4 bytes: Shadow darkness (0.0-1.0)
    glm::vec3 shadowTintColor{0.1f, 0.2f, 0.25f};      // 12 bytes: Shadow tint toward teal
    float shadowBias{0.0005f};                          //  4 bytes: Depth comparison bias
    glm::vec3 shadowColor{0.165f, 0.106f, 0.239f};     // 12 bytes: Color applied to shadowed areas (purple)
    float shadowSoftness{0.2f};                         //  4 bytes: Shadow edge softness (0.0 = hard for toon)
};
static_assert(sizeof(ToonLightingUBO) == 80, "ToonLightingUBO must be 80 bytes");

/**
 * @namespace ToonShaderDefaults
 * @brief Default values for toon shader configuration.
 */
namespace ToonShaderDefaults {
    // Lighting band thresholds (intensity values in range [0,1])
    constexpr float DEEP_SHADOW_THRESHOLD = 0.2f;  // Below this = deep shadow
    constexpr float SHADOW_THRESHOLD = 0.4f;       // Below this = shadow
    constexpr float MID_THRESHOLD = 0.7f;          // Below this = mid

    // Lighting band multipliers
    constexpr float DEEP_SHADOW_INTENSITY = 0.15f;
    constexpr float SHADOW_INTENSITY = 0.35f;
    constexpr float MID_INTENSITY = 0.65f;
    constexpr float LIT_INTENSITY = 1.0f;

    // Ambient configuration
    constexpr float MIN_AMBIENT = 0.05f;
    constexpr float MAX_AMBIENT = 0.15f;
    constexpr float DEFAULT_AMBIENT = 0.08f;

    // Canon-specified shadow color (#2A1B3D = deep purple)
    constexpr float DEEP_SHADOW_R = 42.0f / 255.0f;   // 0.165
    constexpr float DEEP_SHADOW_G = 27.0f / 255.0f;   // 0.106
    constexpr float DEEP_SHADOW_B = 61.0f / 255.0f;   // 0.239

    // Default sun direction (normalized (1, 2, 1))
    // Points "up and to the right" for classic 3/4 lighting
    constexpr float SUN_DIR_X = 0.408248f;
    constexpr float SUN_DIR_Y = 0.816497f;
    constexpr float SUN_DIR_Z = 0.408248f;
}

/**
 * @brief Create default lighting UBO with canon-specified alien aesthetic.
 * @return ToonLightingUBO with default values
 */
inline ToonLightingUBO createDefaultLightingUBO() {
    ToonLightingUBO ubo;
    ubo.sunDirection = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));
    ubo.globalAmbient = ToonShaderDefaults::DEFAULT_AMBIENT;
    ubo.ambientColor = glm::vec3(0.6f, 0.65f, 0.8f);  // Cool blue ambient
    ubo.shadowEnabled = 0.0f;  // Shadows disabled by default
    ubo.deepShadowColor = glm::vec3(
        ToonShaderDefaults::DEEP_SHADOW_R,
        ToonShaderDefaults::DEEP_SHADOW_G,
        ToonShaderDefaults::DEEP_SHADOW_B
    );
    ubo.shadowIntensity = 0.6f;
    ubo.shadowTintColor = glm::vec3(0.1f, 0.2f, 0.25f);  // Teal shadow tint
    ubo.shadowBias = 0.0005f;
    ubo.shadowColor = glm::vec3(
        ToonShaderDefaults::DEEP_SHADOW_R,
        ToonShaderDefaults::DEEP_SHADOW_G,
        ToonShaderDefaults::DEEP_SHADOW_B
    );  // Purple shadow color
    ubo.shadowSoftness = 0.2f;  // Relatively hard edges for toon style
    return ubo;
}

/**
 * @brief Create lighting UBO with shadows enabled.
 * @param shadowIntensity Shadow darkness (0.0-1.0)
 * @param shadowSoftness Shadow edge softness (0.0 = hard, 1.0 = soft)
 * @return ToonLightingUBO with shadows enabled
 */
inline ToonLightingUBO createLightingUBOWithShadows(
    float shadowIntensity = 0.6f,
    float shadowSoftness = 0.2f)
{
    ToonLightingUBO ubo = createDefaultLightingUBO();
    ubo.shadowEnabled = 1.0f;
    ubo.shadowIntensity = shadowIntensity;
    ubo.shadowSoftness = shadowSoftness;
    return ubo;
}

/**
 * @brief Create an instance data structure with common defaults.
 * @param model Model transformation matrix
 * @param baseColor Base diffuse color with alpha
 * @param emissiveColor Emissive color (RGB) with intensity in alpha
 * @param ambientOverride Per-instance ambient override (0 = use global)
 * @return ToonInstanceData ready for upload to GPU
 */
inline ToonInstanceData createInstanceData(
    const glm::mat4& model = glm::mat4(1.0f),
    const glm::vec4& baseColor = glm::vec4(1.0f),
    const glm::vec4& emissiveColor = glm::vec4(0.0f),
    float ambientOverride = 0.0f
) {
    ToonInstanceData data;
    data.model = model;
    data.baseColor = baseColor;
    data.emissiveColor = emissiveColor;
    data.ambientStrength = ambientOverride;
    return data;
}

/**
 * @brief Resource requirements for toon shader pipeline.
 * Used when loading shaders via ShaderCompiler.
 */
namespace ToonShaderResources {
    // Vertex shader resources
    constexpr uint32_t VERTEX_UNIFORM_BUFFERS = 1;   // ViewProjection UBO (now includes light VP)
    constexpr uint32_t VERTEX_STORAGE_BUFFERS = 1;   // Instance data
    constexpr uint32_t VERTEX_SAMPLERS = 0;
    constexpr uint32_t VERTEX_STORAGE_TEXTURES = 0;

    // Fragment shader resources
    constexpr uint32_t FRAGMENT_UNIFORM_BUFFERS = 1; // Lighting UBO (now includes shadow params)
    constexpr uint32_t FRAGMENT_STORAGE_BUFFERS = 0;
    constexpr uint32_t FRAGMENT_SAMPLERS = 1;        // Shadow map comparison sampler
    constexpr uint32_t FRAGMENT_STORAGE_TEXTURES = 0;
}

} // namespace sims3000

#endif // SIMS3000_RENDER_TOON_SHADER_H
