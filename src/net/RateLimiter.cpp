/**
 * @file RateLimiter.cpp
 * @brief Implementation of token bucket rate limiting.
 */

#include "sims3000/net/RateLimiter.h"
#include <SDL3/SDL_log.h>
#include <algorithm>

namespace sims3000 {

// =============================================================================
// ActionCategory Mapping
// =============================================================================

ActionCategory getActionCategory(InputType inputType) {
    switch (inputType) {
        case InputType::PlaceBuilding:
        case InputType::DemolishBuilding:
        case InputType::UpgradeBuilding:
            return ActionCategory::Building;

        case InputType::SetZone:
        case InputType::ClearZone:
            return ActionCategory::Zoning;

        case InputType::PlaceRoad:
        case InputType::PlacePipe:
        case InputType::PlacePowerLine:
            return ActionCategory::Infrastructure;

        case InputType::SetTaxRate:
        case InputType::TakeLoan:
        case InputType::RepayLoan:
            return ActionCategory::Economy;

        case InputType::PauseGame:
        case InputType::SetGameSpeed:
            return ActionCategory::GameControl;

        default:
            // Default to Building category for unknown types
            return ActionCategory::Building;
    }
}

// =============================================================================
// TokenBucket Implementation
// =============================================================================

void TokenBucket::refill(std::uint64_t currentTimeMs) {
    if (lastRefillMs == 0) {
        lastRefillMs = currentTimeMs;
        tokens = maxTokens;  // Start full
        return;
    }

    std::uint64_t elapsed = currentTimeMs - lastRefillMs;
    if (elapsed > 0) {
        float secondsElapsed = static_cast<float>(elapsed) / 1000.0f;
        tokens = std::min(tokens + (refillRate * secondsElapsed), maxTokens);
        lastRefillMs = currentTimeMs;
    }
}

bool TokenBucket::tryConsume(std::uint64_t currentTimeMs) {
    refill(currentTimeMs);

    if (tokens >= 1.0f) {
        tokens -= 1.0f;
        return true;
    }
    return false;
}

void TokenBucket::reset(std::uint64_t currentTimeMs) {
    tokens = maxTokens;
    lastRefillMs = currentTimeMs;
}

// =============================================================================
// PlayerRateState Implementation
// =============================================================================

void PlayerRateState::initialize(std::uint64_t currentTimeMs) {
    // Initialize all buckets with default values
    // (actual rates will be set by RateLimiter based on config)
    for (auto& bucket : buckets) {
        bucket.reset(currentTimeMs);
    }

    actionsThisSecond = 0;
    secondStartMs = currentTimeMs;
    totalDropped = 0;
    abuseCount = 0;
}

bool PlayerRateState::updateAbuseDetection(std::uint64_t currentTimeMs) {
    // Check if we've moved to a new second
    if (currentTimeMs - secondStartMs >= 1000) {
        // Reset counter for new second
        actionsThisSecond = 0;
        secondStartMs = currentTimeMs;
    }

    // Increment counter
    actionsThisSecond++;

    // Check for abuse threshold (100+ actions/sec)
    return actionsThisSecond >= 100;
}

// =============================================================================
// RateLimiter Implementation
// =============================================================================

RateLimiter::RateLimiter(const RateLimitConfig& config)
    : m_config(config)
{
}

RateLimiter::CheckResult RateLimiter::checkAction(PlayerID playerId,
                                                   InputType inputType,
                                                   std::uint64_t currentTimeMs) {
    CheckResult result;
    result.allowed = true;
    result.isAbuse = false;
    result.totalDropped = 0;

    // Skip rate limiting for invalid players or camera actions (client-only)
    if (playerId == 0 ||
        inputType == InputType::CameraMove ||
        inputType == InputType::CameraZoom ||
        inputType == InputType::None) {
        return result;
    }

    // Find or create player state
    auto it = m_playerStates.find(playerId);
    if (it == m_playerStates.end()) {
        // Auto-register player if not found
        registerPlayer(playerId, currentTimeMs);
        it = m_playerStates.find(playerId);
    }

    PlayerRateState& state = it->second;

    // Check for abuse (100+ actions/sec)
    bool wasAbuse = state.updateAbuseDetection(currentTimeMs);
    if (wasAbuse && state.actionsThisSecond == 100) {
        // First time hitting abuse threshold this second
        state.abuseCount++;
        m_totalAbuseEvents++;
        result.isAbuse = true;

        // Log egregious abuse
        SDL_Log("SECURITY: Player %u exceeded abuse threshold (%u actions/sec, abuse count: %u)",
                playerId, state.actionsThisSecond, state.abuseCount);
    }

    // Get the appropriate bucket for this action type
    ActionCategory category = getActionCategory(inputType);
    TokenBucket& bucket = state.buckets[static_cast<std::size_t>(category)];

    // Try to consume a token
    if (!bucket.tryConsume(currentTimeMs)) {
        // Rate limited - silently drop per Q039
        state.totalDropped++;
        m_totalDropped++;
        result.allowed = false;
        result.totalDropped = state.totalDropped;
        return result;
    }

    result.totalDropped = state.totalDropped;
    return result;
}

void RateLimiter::registerPlayer(PlayerID playerId, std::uint64_t currentTimeMs) {
    if (playerId == 0) {
        return;  // Don't register invalid player
    }

    auto& state = m_playerStates[playerId];
    state.initialize(currentTimeMs);

    // Apply configuration to buckets
    for (std::size_t i = 0; i < static_cast<std::size_t>(ActionCategory::COUNT); ++i) {
        state.buckets[i].refillRate = m_config.ratesPerSecond[i];
        state.buckets[i].maxTokens = m_config.burstSizes[i];
        state.buckets[i].tokens = m_config.burstSizes[i];  // Start full
    }
}

void RateLimiter::unregisterPlayer(PlayerID playerId) {
    m_playerStates.erase(playerId);
}

void RateLimiter::resetPlayer(PlayerID playerId, std::uint64_t currentTimeMs) {
    auto it = m_playerStates.find(playerId);
    if (it != m_playerStates.end()) {
        it->second.initialize(currentTimeMs);

        // Re-apply configuration
        for (std::size_t i = 0; i < static_cast<std::size_t>(ActionCategory::COUNT); ++i) {
            it->second.buckets[i].refillRate = m_config.ratesPerSecond[i];
            it->second.buckets[i].maxTokens = m_config.burstSizes[i];
            it->second.buckets[i].tokens = m_config.burstSizes[i];
        }
    }
}

const PlayerRateState* RateLimiter::getPlayerState(PlayerID playerId) const {
    auto it = m_playerStates.find(playerId);
    if (it != m_playerStates.end()) {
        return &it->second;
    }
    return nullptr;
}

std::uint64_t RateLimiter::getTotalDropped() const {
    return m_totalDropped;
}

std::uint32_t RateLimiter::getTotalAbuseEvents() const {
    return m_totalAbuseEvents;
}

} // namespace sims3000
