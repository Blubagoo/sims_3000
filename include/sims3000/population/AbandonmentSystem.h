#pragma once

#include <cstdint>

namespace sims3000::population {
    constexpr uint32_t ABANDONMENT_THRESHOLD_TICKS = 200;  // ~10 seconds of negative conditions
    constexpr float MIN_DEMAND_FOR_ABANDONMENT = -50.0f;   // Demand below this triggers countdown
    constexpr float MIN_DISORDER_FOR_ABANDONMENT = 200.0f; // High disorder triggers countdown

    enum class AbandonmentReason : uint8_t {
        None = 0,
        LowDemand = 1,
        HighDisorder = 2,
        NoUtilities = 3,
        Combined = 4
    };

    struct AbandonmentCandidate {
        uint32_t building_id;
        uint32_t ticks_negative;   // How long conditions have been bad
        AbandonmentReason reason;
    };

    struct AbandonmentResult {
        uint32_t building_id;
        AbandonmentReason reason;
        uint32_t duration_ticks;
    };

    // Check if a building should start/continue abandonment countdown
    bool should_start_abandonment(float demand, float disorder_level,
                                    bool has_power, bool has_fluid);

    // Update an abandonment candidate's counter
    // Returns true if building should be abandoned (threshold reached)
    bool update_abandonment(AbandonmentCandidate& candidate,
                              float demand, float disorder_level,
                              bool has_power, bool has_fluid);

    // Get reason text
    const char* get_abandonment_reason_text(AbandonmentReason reason);
}
