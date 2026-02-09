/**
 * @file DemandStats.cpp
 * @brief IStatQueryable implementation for demand statistics (Ticket E10-048)
 */

#include <sims3000/demand/DemandStats.h>

namespace sims3000 {
namespace demand {

float get_demand_stat(const DemandData& data, uint16_t stat_id) {
    switch (stat_id) {
        case STAT_HABITATION_DEMAND:
            return static_cast<float>(data.habitation_demand);

        case STAT_EXCHANGE_DEMAND:
            return static_cast<float>(data.exchange_demand);

        case STAT_FABRICATION_DEMAND:
            return static_cast<float>(data.fabrication_demand);

        case STAT_HABITATION_CAP:
            return static_cast<float>(data.habitation_cap);

        case STAT_EXCHANGE_CAP:
            return static_cast<float>(data.exchange_cap);

        case STAT_FABRICATION_CAP:
            return static_cast<float>(data.fabrication_cap);

        default:
            return 0.0f;
    }
}

const char* get_demand_stat_name(uint16_t stat_id) {
    switch (stat_id) {
        case STAT_HABITATION_DEMAND:
            return "Habitation Demand";

        case STAT_EXCHANGE_DEMAND:
            return "Exchange Demand";

        case STAT_FABRICATION_DEMAND:
            return "Fabrication Demand";

        case STAT_HABITATION_CAP:
            return "Habitation Cap";

        case STAT_EXCHANGE_CAP:
            return "Exchange Cap";

        case STAT_FABRICATION_CAP:
            return "Fabrication Cap";

        default:
            return "Unknown";
    }
}

bool is_valid_demand_stat(uint16_t stat_id) {
    return stat_id >= STAT_HABITATION_DEMAND && stat_id <= STAT_FABRICATION_CAP;
}

} // namespace demand
} // namespace sims3000
