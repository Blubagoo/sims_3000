/**
 * @file ServiceSerialization.cpp
 * @brief Implementation of service component serialization
 *        (Epic 9, ticket E9-002)
 */

#include <sims3000/services/ServiceSerialization.h>
#include <stdexcept>

namespace sims3000 {
namespace services {

// ============================================================================
// Helper functions
// ============================================================================

static void write_uint8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

static uint8_t read_uint8(const uint8_t* data, size_t& offset) {
    return data[offset++];
}

// ============================================================================
// ServiceProviderComponent serialization (Ticket E9-002)
// ============================================================================

void serialize_service_provider(const ServiceProviderComponent& comp, std::vector<uint8_t>& buffer) {
    // Version byte
    write_uint8(buffer, SERVICE_SERIALIZATION_VERSION);                    // 1 byte

    // Field-by-field serialization
    write_uint8(buffer, static_cast<uint8_t>(comp.service_type));          // 1 byte
    write_uint8(buffer, comp.tier);                                        // 1 byte
    write_uint8(buffer, comp.current_effectiveness);                       // 1 byte
    write_uint8(buffer, comp.is_active ? 1 : 0);                           // 1 byte
    // Total: 1 + 4 = 5 bytes
}

size_t deserialize_service_provider(const uint8_t* data, size_t size, ServiceProviderComponent& comp) {
    if (size < SERVICE_PROVIDER_SERIALIZED_SIZE) {
        throw std::runtime_error("ServiceProviderComponent deserialization: buffer too small");
    }

    size_t offset = 0;
    const uint8_t version = read_uint8(data, offset);

    if (version != SERVICE_SERIALIZATION_VERSION) {
        throw std::runtime_error("ServiceProviderComponent deserialization: unsupported version");
    }

    comp.service_type          = static_cast<ServiceType>(read_uint8(data, offset));
    comp.tier                  = read_uint8(data, offset);
    comp.current_effectiveness = read_uint8(data, offset);
    comp.is_active             = (read_uint8(data, offset) != 0);

    return offset; // 5 bytes consumed
}

} // namespace services
} // namespace sims3000
