/**
 * @file ShadowPass.cpp
 * @brief Shadow pass implementation.
 */

#include "sims3000/render/ShadowPass.h"
#include "sims3000/render/GPUDevice.h"
#include <SDL3/SDL_log.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace sims3000 {

// =============================================================================
// Construction / Destruction
// =============================================================================

ShadowPass::ShadowPass(GPUDevice& device, const ShadowConfig& config)
    : m_device(&device)
    , m_config(config)
{
    if (!createResources()) {
        SDL_Log("ShadowPass: Failed to create resources: %s", m_lastError.c_str());
    }
}

ShadowPass::~ShadowPass() {
    // End pass if still active
    if (m_inPass && m_renderPass) {
        SDL_EndGPURenderPass(m_renderPass);
        m_renderPass = nullptr;
        m_inPass = false;
    }

    releaseResources();
}

ShadowPass::ShadowPass(ShadowPass&& other) noexcept
    : m_device(other.m_device)
    , m_config(other.m_config)
    , m_shadowMap(other.m_shadowMap)
    , m_renderPass(other.m_renderPass)
    , m_currentCmdBuffer(other.m_currentCmdBuffer)
    , m_resolution(other.m_resolution)
    , m_lightView(other.m_lightView)
    , m_lightProjection(other.m_lightProjection)
    , m_lightViewProjection(other.m_lightViewProjection)
    , m_frustumCenter(other.m_frustumCenter)
    , m_inPass(other.m_inPass)
    , m_stats(other.m_stats)
    , m_lastError(std::move(other.m_lastError))
{
    other.m_device = nullptr;
    other.m_shadowMap = nullptr;
    other.m_renderPass = nullptr;
    other.m_currentCmdBuffer = nullptr;
    other.m_inPass = false;
}

ShadowPass& ShadowPass::operator=(ShadowPass&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        if (m_inPass && m_renderPass) {
            SDL_EndGPURenderPass(m_renderPass);
        }
        releaseResources();

        m_device = other.m_device;
        m_config = other.m_config;
        m_shadowMap = other.m_shadowMap;
        m_renderPass = other.m_renderPass;
        m_currentCmdBuffer = other.m_currentCmdBuffer;
        m_resolution = other.m_resolution;
        m_lightView = other.m_lightView;
        m_lightProjection = other.m_lightProjection;
        m_lightViewProjection = other.m_lightViewProjection;
        m_frustumCenter = other.m_frustumCenter;
        m_inPass = other.m_inPass;
        m_stats = other.m_stats;
        m_lastError = std::move(other.m_lastError);

        other.m_device = nullptr;
        other.m_shadowMap = nullptr;
        other.m_renderPass = nullptr;
        other.m_currentCmdBuffer = nullptr;
        other.m_inPass = false;
    }
    return *this;
}

// =============================================================================
// Validation
// =============================================================================

bool ShadowPass::isValid() const {
    return m_device != nullptr &&
           m_device->isValid() &&
           (m_shadowMap != nullptr || !m_config.isEnabled());
}

// =============================================================================
// Configuration
// =============================================================================

void ShadowPass::setConfig(const ShadowConfig& config) {
    bool needsRecreate = (config.getShadowMapResolution() != m_resolution) ||
                         (config.isEnabled() != m_config.isEnabled());

    m_config = config;

    if (needsRecreate) {
        releaseResources();
        if (!createResources()) {
            SDL_Log("ShadowPass: Failed to recreate resources: %s", m_lastError.c_str());
        }
    }
}

void ShadowPass::setQuality(ShadowQuality quality) {
    ShadowConfig newConfig = m_config;
    newConfig.applyQualityPreset(quality);
    setConfig(newConfig);
}

// =============================================================================
// Light Matrix Calculation
// =============================================================================

void ShadowPass::updateLightMatrices(
    const glm::mat4& cameraView,
    const glm::mat4& cameraProjection,
    const glm::vec3& cameraPosition)
{
    if (!m_config.isEnabled()) {
        return;
    }

    // Calculate inverse view-projection to get frustum corners
    glm::mat4 inverseViewProjection = glm::inverse(cameraProjection * cameraView);

    // Get frustum corners in world space
    glm::vec3 frustumCorners[8];
    calculateFrustumCorners(inverseViewProjection, frustumCorners);

    // Calculate frustum center
    m_frustumCenter = glm::vec3(0.0f);
    for (int i = 0; i < 8; ++i) {
        m_frustumCenter += frustumCorners[i];
    }
    m_frustumCenter /= 8.0f;

    // Build light view matrix
    // Light looks at the frustum center from the direction of the light
    glm::vec3 lightDir = glm::normalize(m_config.lightDirection);
    glm::vec3 lightUp = glm::abs(lightDir.y) > 0.99f ?
                        glm::vec3(0.0f, 0.0f, 1.0f) :
                        glm::vec3(0.0f, 1.0f, 0.0f);

    // Position light far enough back to encompass the scene
    float radius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        radius = std::max(radius, glm::length(frustumCorners[i] - m_frustumCenter));
    }
    radius = std::clamp(radius, m_config.minFrustumSize * 0.5f, m_config.maxFrustumSize * 0.5f);
    radius += m_config.frustumPadding;

    glm::vec3 lightPosition = m_frustumCenter + lightDir * radius * 2.0f;

    m_lightView = glm::lookAt(lightPosition, m_frustumCenter, lightUp);

    // Calculate light-space bounds
    glm::vec3 minBounds, maxBounds;
    calculateLightSpaceBounds(frustumCorners, minBounds, maxBounds);

    // Expand bounds for safety
    float padding = m_config.frustumPadding;
    minBounds -= glm::vec3(padding);
    maxBounds += glm::vec3(padding);

    // Clamp to configured limits
    float halfSize = std::clamp((maxBounds.x - minBounds.x) * 0.5f,
                                m_config.minFrustumSize * 0.5f,
                                m_config.maxFrustumSize * 0.5f);

    // Build orthographic projection
    float nearPlane = 0.1f;
    float farPlane = radius * 4.0f;  // Far enough to capture all geometry

    m_lightProjection = glm::ortho(
        -halfSize, halfSize,
        -halfSize, halfSize,
        nearPlane, farPlane
    );

    // Apply texel snapping if enabled
    if (m_config.texelSnapping) {
        applyTexelSnapping();
    }

    // Compute combined matrix
    m_lightViewProjection = m_lightProjection * m_lightView;

    // Update stats
    m_stats.resolution = m_resolution;
}

void ShadowPass::calculateFrustumCorners(
    const glm::mat4& inverseViewProjection,
    glm::vec3 corners[8]) const
{
    // NDC corners (clip space before perspective divide)
    static const glm::vec4 ndcCorners[8] = {
        { -1.0f, -1.0f, 0.0f, 1.0f },  // Near bottom-left
        {  1.0f, -1.0f, 0.0f, 1.0f },  // Near bottom-right
        { -1.0f,  1.0f, 0.0f, 1.0f },  // Near top-left
        {  1.0f,  1.0f, 0.0f, 1.0f },  // Near top-right
        { -1.0f, -1.0f, 1.0f, 1.0f },  // Far bottom-left
        {  1.0f, -1.0f, 1.0f, 1.0f },  // Far bottom-right
        { -1.0f,  1.0f, 1.0f, 1.0f },  // Far top-left
        {  1.0f,  1.0f, 1.0f, 1.0f },  // Far top-right
    };

    for (int i = 0; i < 8; ++i) {
        glm::vec4 worldPos = inverseViewProjection * ndcCorners[i];
        corners[i] = glm::vec3(worldPos) / worldPos.w;
    }
}

void ShadowPass::calculateLightSpaceBounds(
    const glm::vec3 corners[8],
    glm::vec3& minBounds,
    glm::vec3& maxBounds) const
{
    minBounds = glm::vec3(std::numeric_limits<float>::max());
    maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    for (int i = 0; i < 8; ++i) {
        glm::vec4 lightSpacePos = m_lightView * glm::vec4(corners[i], 1.0f);
        glm::vec3 pos = glm::vec3(lightSpacePos);

        minBounds = glm::min(minBounds, pos);
        maxBounds = glm::max(maxBounds, pos);
    }
}

void ShadowPass::applyTexelSnapping() {
    if (m_resolution == 0) {
        return;
    }

    // Get the size of one shadow map texel in world units
    float shadowMapSize = 2.0f;  // Ortho projection range [-1, 1]
    float texelSize = shadowMapSize / static_cast<float>(m_resolution);

    // Snap the light's position to texel boundaries
    // This is done by quantizing the frustum center's light-space position
    glm::vec4 originInLightSpace = m_lightView * glm::vec4(m_frustumCenter, 1.0f);

    // Quantize to texel grid
    originInLightSpace.x = std::floor(originInLightSpace.x / texelSize) * texelSize;
    originInLightSpace.y = std::floor(originInLightSpace.y / texelSize) * texelSize;

    // Create offset for the view matrix
    glm::vec4 snappedOrigin = m_lightView * glm::vec4(m_frustumCenter, 1.0f);
    glm::vec2 offset(
        std::fmod(snappedOrigin.x, texelSize),
        std::fmod(snappedOrigin.y, texelSize)
    );

    // Apply offset to projection matrix
    glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f),
        glm::vec3(-offset.x, -offset.y, 0.0f));
    m_lightProjection = offsetMatrix * m_lightProjection;
}

// =============================================================================
// Render Pass Execution
// =============================================================================

bool ShadowPass::begin(SDL_GPUCommandBuffer* cmdBuffer) {
    if (!m_config.isEnabled()) {
        return true;  // Shadows disabled, nothing to do
    }

    if (!cmdBuffer) {
        m_lastError = "Null command buffer";
        return false;
    }

    if (m_inPass) {
        m_lastError = "Already in shadow pass - call end() first";
        return false;
    }

    if (!m_shadowMap) {
        m_lastError = "Shadow map not created";
        return false;
    }

    m_currentCmdBuffer = cmdBuffer;
    m_stats.reset();

    // Configure depth target (depth-only pass, no color)
    SDL_GPUDepthStencilTargetInfo depthTarget = {};
    depthTarget.texture = m_shadowMap;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_STORE;  // Need to read shadow map later
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.clear_depth = 1.0f;
    depthTarget.cycle = false;

    // Begin depth-only render pass (no color targets)
    m_renderPass = SDL_BeginGPURenderPass(cmdBuffer, nullptr, 0, &depthTarget);
    if (!m_renderPass) {
        m_lastError = std::string("Failed to begin shadow render pass: ") + SDL_GetError();
        return false;
    }

    // Set viewport to shadow map size
    SDL_GPUViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.w = static_cast<float>(m_resolution);
    viewport.h = static_cast<float>(m_resolution);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    SDL_SetGPUViewport(m_renderPass, &viewport);

    // Set scissor rect
    SDL_Rect scissor = {};
    scissor.x = 0;
    scissor.y = 0;
    scissor.w = static_cast<int>(m_resolution);
    scissor.h = static_cast<int>(m_resolution);
    SDL_SetGPUScissor(m_renderPass, &scissor);

    m_inPass = true;
    return true;
}

void ShadowPass::end() {
    if (!m_inPass || !m_renderPass) {
        return;
    }

    SDL_EndGPURenderPass(m_renderPass);
    m_renderPass = nullptr;
    m_currentCmdBuffer = nullptr;
    m_inPass = false;
    m_stats.executed = true;
}

// =============================================================================
// Resource Management
// =============================================================================

bool ShadowPass::createResources() {
    if (!m_device || !m_device->isValid()) {
        m_lastError = "Invalid GPU device";
        return false;
    }

    if (!m_config.isEnabled()) {
        // Shadows disabled, no resources needed
        m_resolution = 0;
        return true;
    }

    m_resolution = m_config.getShadowMapResolution();
    if (m_resolution == 0) {
        m_lastError = "Invalid shadow map resolution";
        return false;
    }

    // Create shadow map depth texture
    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    texInfo.width = m_resolution;
    texInfo.height = m_resolution;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    texInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
                    SDL_GPU_TEXTUREUSAGE_SAMPLER;

    m_shadowMap = SDL_CreateGPUTexture(m_device->getHandle(), &texInfo);
    if (!m_shadowMap) {
        m_lastError = std::string("Failed to create shadow map: ") + SDL_GetError();
        return false;
    }

    SDL_Log("ShadowPass: Created %ux%u shadow map (quality: %s)",
            m_resolution, m_resolution, getShadowQualityName(m_config.quality));

    return true;
}

void ShadowPass::releaseResources() {
    if (m_shadowMap && m_device && m_device->isValid()) {
        SDL_ReleaseGPUTexture(m_device->getHandle(), m_shadowMap);
        m_shadowMap = nullptr;
    }
    m_resolution = 0;
}

} // namespace sims3000
