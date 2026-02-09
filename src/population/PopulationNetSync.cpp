/**
 * @file PopulationNetSync.cpp
 * @brief Implementation of population network synchronization (Ticket E10-032)
 */

#include "sims3000/population/PopulationNetSync.h"
#include "sims3000/population/PopulationData.h"
#include "sims3000/population/EmploymentData.h"
#include <cstring>

namespace sims3000 {
namespace population {

PopulationSnapshot create_snapshot(const PopulationData& pop, const EmploymentData& emp) {
    PopulationSnapshot snapshot{};

    // Population data
    snapshot.total_beings = pop.total_beings;
    snapshot.max_capacity = pop.max_capacity;
    snapshot.youth_percent = pop.youth_percent;
    snapshot.adult_percent = pop.adult_percent;
    snapshot.elder_percent = pop.elder_percent;

    // Growth rate: convert float to int per 1000 for network transmission
    // growth_rate is a float representing fraction, convert to per-1000
    snapshot.growth_rate = static_cast<int32_t>(pop.growth_rate * 1000.0f);

    // Quality-of-life indices
    snapshot.harmony_index = pop.harmony_index;
    snapshot.health_index = pop.health_index;
    snapshot.education_index = pop.education_index;

    // Employment data
    snapshot.unemployment_rate = emp.unemployment_rate;
    snapshot.employed_laborers = emp.employed_laborers;
    snapshot.total_jobs = emp.total_jobs;

    return snapshot;
}

void apply_snapshot(PopulationData& pop, EmploymentData& emp, const PopulationSnapshot& snapshot) {
    // Apply population data
    pop.total_beings = snapshot.total_beings;
    pop.max_capacity = snapshot.max_capacity;
    pop.youth_percent = snapshot.youth_percent;
    pop.adult_percent = snapshot.adult_percent;
    pop.elder_percent = snapshot.elder_percent;

    // Convert growth rate back to float
    pop.growth_rate = snapshot.growth_rate / 1000.0f;

    // Quality-of-life indices
    pop.harmony_index = snapshot.harmony_index;
    pop.health_index = snapshot.health_index;
    pop.education_index = snapshot.education_index;

    // Employment data
    emp.unemployment_rate = snapshot.unemployment_rate;
    emp.employed_laborers = snapshot.employed_laborers;
    emp.total_jobs = snapshot.total_jobs;
}

size_t serialize_snapshot(const PopulationSnapshot& snapshot, uint8_t* buffer, size_t buffer_size) {
    size_t snapshot_size = sizeof(PopulationSnapshot);

    // Check if buffer is large enough
    if (buffer_size < snapshot_size) {
        return 0;
    }

    // Simple memcpy for POD structure
    std::memcpy(buffer, &snapshot, snapshot_size);

    return snapshot_size;
}

bool deserialize_snapshot(const uint8_t* buffer, size_t size, PopulationSnapshot& out) {
    size_t snapshot_size = sizeof(PopulationSnapshot);

    // Check if buffer is large enough
    if (size < snapshot_size) {
        return false;
    }

    // Simple memcpy for POD structure
    std::memcpy(&out, buffer, snapshot_size);

    return true;
}

} // namespace population
} // namespace sims3000
