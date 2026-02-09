/**
 * @file PopulationNetSync.h
 * @brief Network synchronization for population data (Ticket E10-032)
 *
 * Provides serializable snapshot of population data for multiplayer sync.
 * Uses simple POD structure with memcpy-based serialization.
 */

#ifndef SIMS3000_POPULATION_POPULATIONNETSYNC_H
#define SIMS3000_POPULATION_POPULATIONNETSYNC_H

#include <cstdint>
#include <cstddef>

namespace sims3000 {
namespace population {

// Forward declarations
struct PopulationData;
struct EmploymentData;

/**
 * @struct PopulationSnapshot
 * @brief Serializable population snapshot for network sync
 *
 * POD structure containing essential population data for network transmission.
 * Size: 32 bytes (optimized for network packets)
 */
struct PopulationSnapshot {
    uint32_t total_beings;          ///< Total population count
    uint32_t max_capacity;          ///< Maximum housing capacity
    uint8_t youth_percent;          ///< Youth percentage (0-100)
    uint8_t adult_percent;          ///< Adult percentage (0-100)
    uint8_t elder_percent;          ///< Elder percentage (0-100)
    int32_t growth_rate;            ///< Growth rate per 1000 (signed)
    uint8_t harmony_index;          ///< Harmony index (0-100)
    uint8_t health_index;           ///< Health index (0-100)
    uint8_t education_index;        ///< Education index (0-100)
    uint8_t unemployment_rate;      ///< Unemployment rate (0-100)
    uint32_t employed_laborers;     ///< Total employed
    uint32_t total_jobs;            ///< Total available jobs
};

// Verify POD for safe memcpy
static_assert(sizeof(PopulationSnapshot) <= 40, "PopulationSnapshot should be compact for network");

/**
 * @brief Create a snapshot from current population state
 *
 * @param pop Population data
 * @param emp Employment data
 * @return Snapshot for network transmission
 */
PopulationSnapshot create_snapshot(const PopulationData& pop, const EmploymentData& emp);

/**
 * @brief Apply a received snapshot to local state
 *
 * Updates local PopulationData and EmploymentData from network snapshot.
 * Note: Does not overwrite all fields, only synced ones.
 *
 * @param pop Population data to update
 * @param emp Employment data to update
 * @param snapshot Received snapshot
 */
void apply_snapshot(PopulationData& pop, EmploymentData& emp, const PopulationSnapshot& snapshot);

/**
 * @brief Serialize snapshot to byte buffer
 *
 * Uses simple memcpy for POD structure.
 *
 * @param snapshot Snapshot to serialize
 * @param buffer Output buffer (must be at least sizeof(PopulationSnapshot) bytes)
 * @param buffer_size Size of output buffer
 * @return Number of bytes written, or 0 if buffer too small
 */
size_t serialize_snapshot(const PopulationSnapshot& snapshot, uint8_t* buffer, size_t buffer_size);

/**
 * @brief Deserialize snapshot from byte buffer
 *
 * Uses simple memcpy for POD structure.
 *
 * @param buffer Input buffer containing serialized snapshot
 * @param size Size of input buffer
 * @param out Output snapshot
 * @return True if deserialization succeeded, false if buffer too small
 */
bool deserialize_snapshot(const uint8_t* buffer, size_t size, PopulationSnapshot& out);

} // namespace population
} // namespace sims3000

#endif // SIMS3000_POPULATION_POPULATIONNETSYNC_H
