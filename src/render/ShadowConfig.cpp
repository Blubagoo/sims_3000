/**
 * @file ShadowConfig.cpp
 * @brief Shadow configuration implementation.
 */

#include "sims3000/render/ShadowConfig.h"

namespace sims3000 {

std::uint32_t ShadowConfig::getShadowMapResolution() const {
    using namespace ShadowConfigDefaults;

    switch (quality) {
        case ShadowQuality::Disabled:
            return 0;
        case ShadowQuality::Low:
            return RESOLUTION_LOW;
        case ShadowQuality::Medium:
            return RESOLUTION_MEDIUM;
        case ShadowQuality::High:
            return RESOLUTION_HIGH;
        case ShadowQuality::Ultra:
            return RESOLUTION_ULTRA;
        default:
            return RESOLUTION_HIGH;
    }
}

std::uint32_t ShadowConfig::getPCFSampleCount() const {
    using namespace ShadowConfigDefaults;

    switch (quality) {
        case ShadowQuality::Disabled:
            return 0;
        case ShadowQuality::Low:
            return PCF_SAMPLES_LOW;
        case ShadowQuality::Medium:
            return PCF_SAMPLES_MEDIUM;
        case ShadowQuality::High:
            return PCF_SAMPLES_HIGH;
        case ShadowQuality::Ultra:
            return PCF_SAMPLES_ULTRA;
        default:
            return PCF_SAMPLES_HIGH;
    }
}

void ShadowConfig::applyQualityPreset(ShadowQuality q) {
    quality = q;

    switch (q) {
        case ShadowQuality::Disabled:
            enabled = false;
            break;

        case ShadowQuality::Low:
            enabled = true;
            shadowSoftness = 0.0f;  // Hard edges for performance
            depthBias = 0.001f;     // Higher bias for lower resolution
            slopeBias = 0.003f;
            normalBias = 0.03f;
            texelSnapping = true;
            break;

        case ShadowQuality::Medium:
            enabled = true;
            shadowSoftness = 0.15f;
            depthBias = 0.0007f;
            slopeBias = 0.0025f;
            normalBias = 0.025f;
            texelSnapping = true;
            break;

        case ShadowQuality::High:
            enabled = true;
            shadowSoftness = 0.2f;
            depthBias = 0.0005f;
            slopeBias = 0.002f;
            normalBias = 0.02f;
            texelSnapping = true;
            break;

        case ShadowQuality::Ultra:
            enabled = true;
            shadowSoftness = 0.25f;
            depthBias = 0.0003f;
            slopeBias = 0.0015f;
            normalBias = 0.015f;
            texelSnapping = true;
            break;
    }
}

void ShadowConfig::resetToDefaults() {
    using namespace ShadowConfigDefaults;

    quality = ShadowQuality::High;
    enabled = true;

    lightDirection = glm::vec3(LIGHT_DIR_X, LIGHT_DIR_Y, LIGHT_DIR_Z);

    shadowColor = glm::vec3(SHADOW_COLOR_R, SHADOW_COLOR_G, SHADOW_COLOR_B);
    shadowIntensity = SHADOW_INTENSITY;
    shadowSoftness = SHADOW_SOFTNESS;

    depthBias = DEPTH_BIAS;
    slopeBias = SLOPE_BIAS;
    normalBias = NORMAL_BIAS;

    frustumPadding = FRUSTUM_PADDING;
    minFrustumSize = MIN_FRUSTUM_SIZE;
    maxFrustumSize = MAX_FRUSTUM_SIZE;

    texelSnapping = true;
}

const char* getShadowQualityName(ShadowQuality quality) {
    switch (quality) {
        case ShadowQuality::Disabled: return "Disabled";
        case ShadowQuality::Low:      return "Low";
        case ShadowQuality::Medium:   return "Medium";
        case ShadowQuality::High:     return "High";
        case ShadowQuality::Ultra:    return "Ultra";
        default:                      return "Unknown";
    }
}

} // namespace sims3000
