/**
 * @file RateLimiter.h
 * @brief Token bucket rate limiting for player actions.
 *
 * Provides per-player, per-action-type rate limiting using token bucket algorithm.
 * Designed to prevent abuse while allowing normal gameplay patterns.
 *
 * Rate limits per action type (per second):
 * - Building: 10/sec
 * - Zoning: 20/sec
 * - Infrastructure: 15/sec
 * - Economy: 5/sec
 * - Default: 10/sec
 *
 * Actions exceeding the rate limit are silently dropped (per Q039).
 * Egregious abuse (100+ actions/sec) triggers security logging.
 *
 * Ownership: NetworkServer owns RateLimiter.
 * Thread safety: Not thread-safe. Call from main thread only.
 */

#ifndef SIMS3000_NET_RATELIMITER_H
#define SIMS3000_NET_RATELIMITER_H

#include "sims3000/core/types.h"
#include "sims3000/net/InputMessage.h"
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <array>

namespace sims3000 {

/**
 * @enum ActionCategory
 * @brief Categories of player actions for rate limiting.
 *
 * Different action types have different rate limits based on
 * expected normal usage patterns.
 */
enum class ActionCategory : std::uint8_t {
    Building = 0,       ///< PlaceBuilding, DemolishBuilding, UpgradeBuilding
    Zoning = 1,         ///< SetZone, ClearZone (often drag operations)
    Infrastructure = 2, ///< PlaceRoad, PlacePipe, PlacePowerLine
    Economy = 3,        ///< SetTaxRate, TakeLoan, RepayLoan
    GameControl = 4,    ///< PauseGame, SetGameSpeed
    COUNT = 5
};

/**
 * @brief Map InputType to ActionCategory for rate limiting.
 */
ActionCategory getActionCategory(InputType inputType);

/**
 * @struct TokenBucket
 * @brief Token bucket for rate limiting a single action category.
 *
 * Uses the standard token bucket algorithm:
 * - Tokens refill at a constant rate up to a maximum (burst size)
 * - Each action consumes one token
 * - Actions are allowed if tokens >= 1, otherwise rejected
 */
struct TokenBucket {
    float tokens = 0.0f;          ///< Current token count (fractional allowed)
    float maxTokens = 10.0f;      ///< Maximum token capacity (burst size)
    float refillRate = 10.0f;     ///< Tokens added per second
    std::uint64_t lastRefillMs = 0; ///< Timestamp of last refill

    /**
     * @brief Refill tokens based on elapsed time.
     * @param currentTimeMs Current timestamp in milliseconds.
     */
    void refill(std::uint64_t currentTimeMs);

    /**
     * @brief Try to consume a token for an action.
     * @param currentTimeMs Current timestamp for refill calculation.
     * @return true if token was consumed (action allowed), false if rate limited.
     */
    bool tryConsume(std::uint64_t currentTimeMs);

    /**
     * @brief Reset the bucket to full capacity.
     * @param currentTimeMs Current timestamp.
     */
    void reset(std::uint64_t currentTimeMs);
};

/**
 * @struct PlayerRateState
 * @brief Rate limiting state for a single player.
 *
 * Contains per-category token buckets and abuse detection counters.
 */
struct PlayerRateState {
    /// Token buckets for each action category
    std::array<TokenBucket, static_cast<std::size_t>(ActionCategory::COUNT)> buckets;

    /// Rolling counter for abuse detection (actions in current second)
    std::uint32_t actionsThisSecond = 0;

    /// Timestamp when current second started
    std::uint64_t secondStartMs = 0;

    /// Total actions dropped due to rate limiting
    std::uint64_t totalDropped = 0;

    /// Number of times abuse threshold was triggered
    std::uint32_t abuseCount = 0;

    /**
     * @brief Initialize rate state with default limits.
     * @param currentTimeMs Current timestamp.
     */
    void initialize(std::uint64_t currentTimeMs);

    /**
     * @brief Update rolling counter and check for abuse.
     * @param currentTimeMs Current timestamp.
     * @return true if player is currently in abuse state (100+ actions/sec)
     */
    bool updateAbuseDetection(std::uint64_t currentTimeMs);
};

/**
 * @struct RateLimitConfig
 * @brief Configuration for rate limits per action category.
 */
struct RateLimitConfig {
    /// Rate limits per category (actions per second)
    std::array<float, static_cast<std::size_t>(ActionCategory::COUNT)> ratesPerSecond = {
        10.0f,  // Building
        20.0f,  // Zoning (higher for drag operations)
        15.0f,  // Infrastructure
        5.0f,   // Economy
        5.0f    // GameControl
    };

    /// Burst sizes per category (max tokens)
    std::array<float, static_cast<std::size_t>(ActionCategory::COUNT)> burstSizes = {
        15.0f,  // Building
        30.0f,  // Zoning
        20.0f,  // Infrastructure
        10.0f,  // Economy
        10.0f   // GameControl
    };

    /// Threshold for egregious abuse logging (actions per second)
    std::uint32_t abuseThreshold = 100;
};

/**
 * @class RateLimiter
 * @brief Per-player rate limiting for network actions.
 *
 * Example usage:
 * @code
 * RateLimiter rateLimiter;
 *
 * void handleInput(PlayerID player, const InputMessage& input) {
 *     auto result = rateLimiter.checkAction(player, input.type, currentTimeMs);
 *     if (!result.allowed) {
 *         // Silently drop the action per Q039
 *         return;
 *     }
 *     // Process the action
 * }
 * @endcode
 */
class RateLimiter {
public:
    /**
     * @struct CheckResult
     * @brief Result of checking an action against rate limits.
     */
    struct CheckResult {
        bool allowed = true;            ///< Whether the action is allowed
        bool isAbuse = false;           ///< Whether this triggered abuse detection
        std::uint64_t totalDropped = 0; ///< Total actions dropped for this player
    };

    /**
     * @brief Construct a RateLimiter with default configuration.
     */
    RateLimiter() = default;

    /**
     * @brief Construct a RateLimiter with custom configuration.
     * @param config Rate limit configuration.
     */
    explicit RateLimiter(const RateLimitConfig& config);

    /**
     * @brief Check if an action is allowed for a player.
     * @param playerId The player attempting the action.
     * @param inputType The type of action being attempted.
     * @param currentTimeMs Current timestamp in milliseconds.
     * @return CheckResult indicating if action is allowed.
     *
     * If allowed, consumes a token from the appropriate bucket.
     * If not allowed, increments the dropped counter.
     */
    CheckResult checkAction(PlayerID playerId, InputType inputType, std::uint64_t currentTimeMs);

    /**
     * @brief Register a new player for rate limiting.
     * @param playerId The player to register.
     * @param currentTimeMs Current timestamp.
     *
     * Called when a player joins the server.
     */
    void registerPlayer(PlayerID playerId, std::uint64_t currentTimeMs);

    /**
     * @brief Unregister a player from rate limiting.
     * @param playerId The player to unregister.
     *
     * Called when a player disconnects.
     */
    void unregisterPlayer(PlayerID playerId);

    /**
     * @brief Reset rate limiting state for a player.
     * @param playerId The player to reset.
     * @param currentTimeMs Current timestamp.
     *
     * Useful for testing or after reconnection.
     */
    void resetPlayer(PlayerID playerId, std::uint64_t currentTimeMs);

    /**
     * @brief Get the rate state for a player (for debugging/stats).
     * @param playerId The player to query.
     * @return Pointer to PlayerRateState, or nullptr if player not found.
     */
    const PlayerRateState* getPlayerState(PlayerID playerId) const;

    /**
     * @brief Get total actions dropped across all players.
     */
    std::uint64_t getTotalDropped() const;

    /**
     * @brief Get total abuse events detected across all players.
     */
    std::uint32_t getTotalAbuseEvents() const;

    /**
     * @brief Get the current configuration.
     */
    const RateLimitConfig& getConfig() const { return m_config; }

private:
    RateLimitConfig m_config;
    std::unordered_map<PlayerID, PlayerRateState> m_playerStates;
    std::uint64_t m_totalDropped = 0;
    std::uint32_t m_totalAbuseEvents = 0;
};

} // namespace sims3000

#endif // SIMS3000_NET_RATELIMITER_H
