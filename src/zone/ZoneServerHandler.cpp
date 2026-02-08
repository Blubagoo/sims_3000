/**
 * @file ZoneServerHandler.cpp
 * @brief Implementation of ZoneServerHandler (Ticket 4-039)
 */

#include <sims3000/zone/ZoneServerHandler.h>
#include <sims3000/zone/ZoneSystem.h>

namespace sims3000 {
namespace zone {

ZoneServerHandler::ZoneServerHandler(ZoneSystem* zone_system)
    : m_zone_system(zone_system)
{
}

ZoneServerResponse ZoneServerHandler::handle_placement_request(
    const ZonePlacementRequestMsg& msg, uint8_t player_id)
{
    ZoneServerResponse response;

    // Validate player_id
    if (player_id >= MAX_OVERSEERS) {
        response.success = false;
        response.rejection_reason = "Invalid player ID";
        return response;
    }

    // Validate zone_system
    if (!m_zone_system) {
        response.success = false;
        response.rejection_reason = "Zone system unavailable";
        return response;
    }

    // Validate message fields
    if (msg.width <= 0 || msg.height <= 0) {
        response.success = false;
        response.rejection_reason = "Invalid area dimensions";
        return response;
    }

    // Create ZonePlacementRequest from message
    ZonePlacementRequest request;
    request.x = msg.x;
    request.y = msg.y;
    request.width = msg.width;
    request.height = msg.height;
    request.zone_type = static_cast<ZoneType>(msg.zone_type);
    request.density = static_cast<ZoneDensity>(msg.density);
    request.player_id = player_id;

    // Delegate to ZoneSystem
    ZonePlacementResult result = m_zone_system->place_zones(request);

    response.success = result.any_placed;
    response.placed_count = result.placed_count;
    if (!result.any_placed) {
        response.rejection_reason = "No zones could be placed";
    }

    return response;
}

ZoneServerResponse ZoneServerHandler::handle_dezone_request(
    const DezoneRequestMsg& msg, uint8_t player_id)
{
    ZoneServerResponse response;

    // Validate player_id
    if (player_id >= MAX_OVERSEERS) {
        response.success = false;
        response.rejection_reason = "Invalid player ID";
        return response;
    }

    // Validate zone_system
    if (!m_zone_system) {
        response.success = false;
        response.rejection_reason = "Zone system unavailable";
        return response;
    }

    // Validate message fields
    if (msg.width <= 0 || msg.height <= 0) {
        response.success = false;
        response.rejection_reason = "Invalid area dimensions";
        return response;
    }

    // Delegate to ZoneSystem
    DezoneResult result = m_zone_system->remove_zones(
        msg.x, msg.y, msg.width, msg.height, player_id
    );

    response.success = result.any_removed || result.demolition_requested_count > 0;
    response.removed_count = result.removed_count;
    if (!response.success) {
        response.rejection_reason = "No zones could be removed";
    }

    return response;
}

ZoneServerResponse ZoneServerHandler::handle_redesignate_request(
    const RedesignateRequestMsg& msg, uint8_t player_id)
{
    ZoneServerResponse response;

    // Validate player_id
    if (player_id >= MAX_OVERSEERS) {
        response.success = false;
        response.rejection_reason = "Invalid player ID";
        return response;
    }

    // Validate zone_system
    if (!m_zone_system) {
        response.success = false;
        response.rejection_reason = "Zone system unavailable";
        return response;
    }

    // Delegate to ZoneSystem
    RedesignateResult result = m_zone_system->redesignate_zone(
        msg.x, msg.y,
        static_cast<ZoneType>(msg.new_zone_type),
        static_cast<ZoneDensity>(msg.new_density),
        player_id
    );

    response.success = result.success;
    if (!result.success) {
        switch (result.reason) {
            case RedesignateResult::Reason::NoZoneAtPosition:
                response.rejection_reason = "No zone at position";
                break;
            case RedesignateResult::Reason::NotOwned:
                response.rejection_reason = "Zone not owned by player";
                break;
            case RedesignateResult::Reason::SameTypeAndDensity:
                response.rejection_reason = "Same type and density";
                break;
            case RedesignateResult::Reason::OccupiedRequiresDemolition:
                response.rejection_reason = "Occupied zone requires demolition";
                // Note: demolition was requested, so this is partially successful
                response.success = result.demolition_requested;
                break;
            default:
                response.rejection_reason = "Unknown error";
                break;
        }
    }

    return response;
}

} // namespace zone
} // namespace sims3000
