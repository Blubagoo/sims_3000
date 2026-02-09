#include <sims3000/population/AbandonmentSystem.h>

namespace sims3000::population {

    bool should_start_abandonment(float demand, float disorder_level,
                                    bool has_power, bool has_fluid) {
        // Check if demand is too low
        if (demand < MIN_DEMAND_FOR_ABANDONMENT) {
            return true;
        }

        // Check if disorder is too high
        if (disorder_level > MIN_DISORDER_FOR_ABANDONMENT) {
            return true;
        }

        // Check if building has no utilities (neither power nor fluid)
        if (!has_power && !has_fluid) {
            return true;
        }

        return false;
    }

    bool update_abandonment(AbandonmentCandidate& candidate,
                              float demand, float disorder_level,
                              bool has_power, bool has_fluid) {
        // Check if conditions are still bad
        bool conditions_bad = should_start_abandonment(demand, disorder_level, has_power, has_fluid);

        if (conditions_bad) {
            // Increment the counter
            candidate.ticks_negative++;

            // Determine the primary reason
            bool low_demand = demand < MIN_DEMAND_FOR_ABANDONMENT;
            bool high_disorder = disorder_level > MIN_DISORDER_FOR_ABANDONMENT;
            bool no_utilities = !has_power && !has_fluid;

            // Count how many conditions are true
            int condition_count = (low_demand ? 1 : 0) + (high_disorder ? 1 : 0) + (no_utilities ? 1 : 0);

            if (condition_count > 1) {
                candidate.reason = AbandonmentReason::Combined;
            } else if (low_demand) {
                candidate.reason = AbandonmentReason::LowDemand;
            } else if (high_disorder) {
                candidate.reason = AbandonmentReason::HighDisorder;
            } else if (no_utilities) {
                candidate.reason = AbandonmentReason::NoUtilities;
            } else {
                candidate.reason = AbandonmentReason::None;
            }

            // Check if threshold reached
            return candidate.ticks_negative >= ABANDONMENT_THRESHOLD_TICKS;
        } else {
            // Conditions improved, reset counter
            candidate.ticks_negative = 0;
            candidate.reason = AbandonmentReason::None;
            return false;
        }
    }

    const char* get_abandonment_reason_text(AbandonmentReason reason) {
        switch (reason) {
            case AbandonmentReason::None:
                return "None";
            case AbandonmentReason::LowDemand:
                return "Low Demand";
            case AbandonmentReason::HighDisorder:
                return "High Disorder";
            case AbandonmentReason::NoUtilities:
                return "No Utilities";
            case AbandonmentReason::Combined:
                return "Multiple Issues";
            default:
                return "Unknown";
        }
    }
}
