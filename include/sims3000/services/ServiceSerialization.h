/**
 * @file ServiceSerialization.h
 * @brief Service component serialization/deserialization
 *        (Epic 9, ticket E9-002)
 *
 * Provides network serialization for:
 * - ServiceProviderComponent: 5-byte field-by-field LE serialization (1 version + 4 data)
 *
 * @see ServiceProviderComponent.h
 * @see ServiceTypes.h
 */

#pragma once

#include <sims3000/services/ServiceProviderComponent.h>
#include <cstdint>
#include <vector>

namespace sims3000 {
namespace services {

// ============================================================================
// Serialization version
// ============================================================================

/// Current serialization version for service data
constexpr uint8_t SERVICE_SERIALIZATION_VERSION = 1;

// ============================================================================
// ServiceProviderComponent serialization (Ticket E9-002)
// ============================================================================

/// Serialized size of ServiceProviderComponent on the wire
/// (1 version + 1 service_type + 1 tier + 1 current_effectiveness
///  + 1 is_active = 5 bytes)
constexpr size_t SERVICE_PROVIDER_SERIALIZED_SIZE = 5;

/**
 * @brief Serialize a ServiceProviderComponent to a byte buffer.
 *
 * Uses field-by-field encoding for cross-platform safety.
 * Total serialized size: 5 bytes (1 version + 4 component fields).
 *
 * @param comp The ServiceProviderComponent to serialize.
 * @param buffer Output byte buffer (appended to).
 */
void serialize_service_provider(const ServiceProviderComponent& comp, std::vector<uint8_t>& buffer);

/**
 * @brief Deserialize a ServiceProviderComponent from a byte buffer at offset.
 *
 * @param data Pointer to serialized data.
 * @param size Size of the data buffer in bytes.
 * @param comp Output ServiceProviderComponent to populate.
 * @return Number of bytes consumed.
 * @throws std::runtime_error if buffer is too small or version mismatch.
 */
size_t deserialize_service_provider(const uint8_t* data, size_t size, ServiceProviderComponent& comp);

} // namespace services
} // namespace sims3000
