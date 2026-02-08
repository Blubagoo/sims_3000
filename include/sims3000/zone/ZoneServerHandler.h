/**
 * @file ZoneServerHandler.h
 * @brief Server-authoritative validation pipeline for zone operations (Ticket 4-039)
 *
 * Processes incoming network messages for zone operations on the server side:
 * - ZonePlacementRequestMsg -> place_zones()
 * - DezoneRequestMsg -> remove_zones()
 * - RedesignateRequestMsg -> redesignate_zone()
 *
 * Each handler validates player_id < MAX_OVERSEERS before delegating
 * to ZoneSystem. Invalid requests are rejected with a reason string.
 */

#ifndef SIMS3000_ZONE_ZONESERVERHANDLER_H
#define SIMS3000_ZONE_ZONESERVERHANDLER_H

#include <sims3000/zone/ZoneNetworkMessages.h>
#include <sims3000/zone/ZoneTypes.h>
#include <cstdint>
#include <string>

// Forward declaration
namespace sims3000 {
namespace zone {
    class ZoneSystem;
} // namespace zone
} // namespace sims3000

namespace sims3000 {
namespace zone {

/**
 * @struct ZoneServerResponse
 * @brief Response from server-side zone operation handling.
 */
struct ZoneServerResponse {
    bool success;                   ///< True if operation succeeded
    uint32_t placed_count = 0;     ///< Number of zones placed (placement only)
    uint32_t removed_count = 0;    ///< Number of zones removed (dezone only)
    std::string rejection_reason;  ///< Reason for rejection (empty if success)

    ZoneServerResponse() : success(false) {}
};

/**
 * @class ZoneServerHandler
 * @brief Server-side handler for zone network messages.
 *
 * Validates and delegates zone operations from network messages to ZoneSystem.
 * Ensures player_id is valid before processing any request.
 */
class ZoneServerHandler {
public:
    /**
     * @brief Construct ZoneServerHandler with ZoneSystem dependency.
     * @param zone_system Pointer to ZoneSystem for zone operations.
     */
    explicit ZoneServerHandler(ZoneSystem* zone_system);

    /**
     * @brief Handle a zone placement request from a client.
     *
     * Creates a ZonePlacementRequest from the message, sets the player_id,
     * and delegates to zone_system->place_zones().
     *
     * @param msg The placement request message.
     * @param player_id The authenticated player ID (server-assigned).
     * @return ZoneServerResponse with success/failure and placed count.
     */
    ZoneServerResponse handle_placement_request(const ZonePlacementRequestMsg& msg, uint8_t player_id);

    /**
     * @brief Handle a dezone request from a client.
     *
     * Delegates to zone_system->remove_zones() with the message area.
     *
     * @param msg The dezone request message.
     * @param player_id The authenticated player ID (server-assigned).
     * @return ZoneServerResponse with success/failure and removed count.
     */
    ZoneServerResponse handle_dezone_request(const DezoneRequestMsg& msg, uint8_t player_id);

    /**
     * @brief Handle a zone redesignation request from a client.
     *
     * Delegates to zone_system->redesignate_zone() with the message fields.
     *
     * @param msg The redesignation request message.
     * @param player_id The authenticated player ID (server-assigned).
     * @return ZoneServerResponse with success/failure.
     */
    ZoneServerResponse handle_redesignate_request(const RedesignateRequestMsg& msg, uint8_t player_id);

private:
    ZoneSystem* m_zone_system;
};

} // namespace zone
} // namespace sims3000

#endif // SIMS3000_ZONE_ZONESERVERHANDLER_H
