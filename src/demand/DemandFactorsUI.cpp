/**
 * @file DemandFactorsUI.cpp
 * @brief Implementation of UI demand factor helper functions (Ticket E10-047).
 */

#include <sims3000/demand/DemandFactorsUI.h>
#include <cstring> // for strcmp

namespace sims3000 {
namespace demand {

const DemandFactors& get_demand_factors(const DemandData& data, uint8_t zone_type) {
    switch (zone_type) {
        case ZONE_HABITATION:
            return data.habitation_factors;
        case ZONE_EXCHANGE:
            return data.exchange_factors;
        case ZONE_FABRICATION:
            return data.fabrication_factors;
        default:
            // Invalid zone type - return habitation as fallback
            return data.habitation_factors;
    }
}

const char* get_dominant_factor_name(const DemandFactors& factors) {
    // Track the factor with the largest absolute value
    int8_t max_abs = 0;
    const char* dominant = "none";

    // Helper lambda to check and update dominant factor
    auto check_factor = [&](int8_t value, const char* name) {
        int8_t abs_value = (value < 0) ? -value : value;
        if (abs_value > max_abs) {
            max_abs = abs_value;
            dominant = name;
        }
    };

    check_factor(factors.population_factor, "population");
    check_factor(factors.employment_factor, "employment");
    check_factor(factors.services_factor, "services");
    check_factor(factors.tribute_factor, "tribute");
    check_factor(factors.transport_factor, "transport");
    check_factor(factors.contamination_factor, "contamination");

    return dominant;
}

const char* get_demand_description(int8_t demand_value) {
    if (demand_value >= 75) {
        return "Strong Growth";
    } else if (demand_value >= 25) {
        return "Growth";
    } else if (demand_value >= 10) {
        return "Weak Growth";
    } else if (demand_value > -10) {
        return "Stagnant";
    } else if (demand_value > -25) {
        return "Weak Decline";
    } else if (demand_value > -75) {
        return "Decline";
    } else {
        return "Strong Decline";
    }
}

int16_t sum_factors(const DemandFactors& factors) {
    int16_t sum = 0;
    sum += factors.population_factor;
    sum += factors.employment_factor;
    sum += factors.services_factor;
    sum += factors.tribute_factor;
    sum += factors.transport_factor;
    sum += factors.contamination_factor;
    return sum;
}

bool is_bottlenecked_by(const DemandFactors& factors, const char* factor_name) {
    if (!factor_name) {
        return false;
    }

    // Get the value of the named factor
    int8_t factor_value = 0;
    if (std::strcmp(factor_name, "population") == 0) {
        factor_value = factors.population_factor;
    } else if (std::strcmp(factor_name, "employment") == 0) {
        factor_value = factors.employment_factor;
    } else if (std::strcmp(factor_name, "services") == 0) {
        factor_value = factors.services_factor;
    } else if (std::strcmp(factor_name, "tribute") == 0) {
        factor_value = factors.tribute_factor;
    } else if (std::strcmp(factor_name, "transport") == 0) {
        factor_value = factors.transport_factor;
    } else if (std::strcmp(factor_name, "contamination") == 0) {
        factor_value = factors.contamination_factor;
    } else {
        // Unknown factor name
        return false;
    }

    // Must be negative to be a bottleneck
    if (factor_value >= 0) {
        return false;
    }

    // Check if this factor has the largest absolute value (is dominant)
    const char* dominant = get_dominant_factor_name(factors);
    return std::strcmp(dominant, factor_name) == 0;
}

} // namespace demand
} // namespace sims3000
