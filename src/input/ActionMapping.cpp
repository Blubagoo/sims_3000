/**
 * @file ActionMapping.cpp
 * @brief ActionMapping implementation.
 */

#include "sims3000/input/ActionMapping.h"
#include <algorithm>

namespace sims3000 {

const std::vector<SDL_Scancode> ActionMapping::s_emptyBindings;

ActionMapping::ActionMapping() {
    resetToDefaults();
}

void ActionMapping::bind(Action action, SDL_Scancode scancode, bool append) {
    if (!append) {
        m_bindings[action].clear();
    }

    auto& bindings = m_bindings[action];
    if (std::find(bindings.begin(), bindings.end(), scancode) == bindings.end()) {
        bindings.push_back(scancode);
    }
}

void ActionMapping::unbind(Action action, SDL_Scancode scancode) {
    auto it = m_bindings.find(action);
    if (it != m_bindings.end()) {
        auto& bindings = it->second;
        bindings.erase(
            std::remove(bindings.begin(), bindings.end(), scancode),
            bindings.end()
        );
    }
}

void ActionMapping::clearBindings(Action action) {
    m_bindings[action].clear();
}

const std::vector<SDL_Scancode>& ActionMapping::getBindings(Action action) const {
    auto it = m_bindings.find(action);
    return (it != m_bindings.end()) ? it->second : s_emptyBindings;
}

std::vector<Action> ActionMapping::getActionsForKey(SDL_Scancode scancode) const {
    std::vector<Action> actions;
    for (const auto& [action, bindings] : m_bindings) {
        if (std::find(bindings.begin(), bindings.end(), scancode) != bindings.end()) {
            actions.push_back(action);
        }
    }
    return actions;
}

void ActionMapping::resetToDefaults() {
    m_bindings.clear();

    // Camera movement
    bind(Action::PAN_UP, SDL_SCANCODE_W);
    bind(Action::PAN_UP, SDL_SCANCODE_UP, true);
    bind(Action::PAN_DOWN, SDL_SCANCODE_S);
    bind(Action::PAN_DOWN, SDL_SCANCODE_DOWN, true);
    bind(Action::PAN_LEFT, SDL_SCANCODE_A);
    bind(Action::PAN_LEFT, SDL_SCANCODE_LEFT, true);
    bind(Action::PAN_RIGHT, SDL_SCANCODE_D);
    bind(Action::PAN_RIGHT, SDL_SCANCODE_RIGHT, true);
    bind(Action::ZOOM_IN, SDL_SCANCODE_EQUALS);
    bind(Action::ZOOM_OUT, SDL_SCANCODE_MINUS);
    bind(Action::ROTATE_CW, SDL_SCANCODE_E);
    bind(Action::ROTATE_CCW, SDL_SCANCODE_Q);

    // Game controls
    bind(Action::PAUSE, SDL_SCANCODE_SPACE);
    bind(Action::SPEED_1, SDL_SCANCODE_1);
    bind(Action::SPEED_2, SDL_SCANCODE_2);
    bind(Action::SPEED_3, SDL_SCANCODE_3);

    // UI
    bind(Action::CONFIRM, SDL_SCANCODE_RETURN);
    bind(Action::CANCEL, SDL_SCANCODE_ESCAPE);
    bind(Action::MENU, SDL_SCANCODE_ESCAPE);

    // Debug
    bind(Action::DEBUG_TOGGLE, SDL_SCANCODE_F1);
    bind(Action::DEBUG_CAMERA, SDL_SCANCODE_F4);
    bind(Action::DEBUG_GRID, SDL_SCANCODE_G);
    bind(Action::DEBUG_WIREFRAME, SDL_SCANCODE_F);
    bind(Action::DEBUG_STATS, SDL_SCANCODE_F3);  // Render stats overlay
    bind(Action::DEBUG_BOUNDING_BOX, SDL_SCANCODE_B);  // Bounding box visualization

    // Terrain debug modes (F5-F12)
    bind(Action::DEBUG_TERRAIN_ELEVATION, SDL_SCANCODE_F5);
    bind(Action::DEBUG_TERRAIN_TYPE, SDL_SCANCODE_F6);
    bind(Action::DEBUG_TERRAIN_CHUNK, SDL_SCANCODE_F7);
    bind(Action::DEBUG_TERRAIN_LOD, SDL_SCANCODE_F8);
    bind(Action::DEBUG_TERRAIN_NORMALS, SDL_SCANCODE_F9);
    bind(Action::DEBUG_TERRAIN_WATER, SDL_SCANCODE_F10);
    bind(Action::DEBUG_TERRAIN_BUILDABILITY, SDL_SCANCODE_F12);

    // View modes
    bind(Action::TOGGLE_UNDERGROUND, SDL_SCANCODE_U);

    // Zone/Building demo controls (Epic 4)
    bind(Action::ZONE_HABITATION, SDL_SCANCODE_4);
    bind(Action::ZONE_EXCHANGE, SDL_SCANCODE_5);
    bind(Action::ZONE_FABRICATION, SDL_SCANCODE_6);
    bind(Action::ZONE_PLACE, SDL_SCANCODE_Z);
    bind(Action::ZONE_DEMOLISH, SDL_SCANCODE_X);

    // Energy demo controls (Epic 5)
    bind(Action::ENERGY_NEXUS_CARBON, SDL_SCANCODE_7);
    bind(Action::ENERGY_NEXUS_WIND, SDL_SCANCODE_8);
    bind(Action::ENERGY_NEXUS_SOLAR, SDL_SCANCODE_9);
    bind(Action::ENERGY_CONDUIT, SDL_SCANCODE_0);
    bind(Action::ENERGY_PLACE, SDL_SCANCODE_C);
    bind(Action::ENERGY_REMOVE, SDL_SCANCODE_V);
    bind(Action::ENERGY_OVERLAY, SDL_SCANCODE_G);

    // Fluid demo controls (Epic 6)
    bind(Action::FLUID_EXTRACTOR, SDL_SCANCODE_U);
    bind(Action::FLUID_RESERVOIR, SDL_SCANCODE_I);
    bind(Action::FLUID_CONDUIT, SDL_SCANCODE_O);
    bind(Action::FLUID_PLACE, SDL_SCANCODE_N);
    bind(Action::FLUID_REMOVE, SDL_SCANCODE_M);
    bind(Action::FLUID_OVERLAY, SDL_SCANCODE_H);

    // System
    bind(Action::FULLSCREEN, SDL_SCANCODE_F11);
    bind(Action::QUIT, SDL_SCANCODE_ESCAPE);
}

const char* ActionMapping::getActionName(Action action) {
    switch (action) {
        case Action::PAN_UP:       return "Pan Up";
        case Action::PAN_DOWN:     return "Pan Down";
        case Action::PAN_LEFT:     return "Pan Left";
        case Action::PAN_RIGHT:    return "Pan Right";
        case Action::ZOOM_IN:      return "Zoom In";
        case Action::ZOOM_OUT:     return "Zoom Out";
        case Action::ROTATE_CW:    return "Rotate CW";
        case Action::ROTATE_CCW:   return "Rotate CCW";
        case Action::PAUSE:        return "Pause";
        case Action::SPEED_1:      return "Speed 1";
        case Action::SPEED_2:      return "Speed 2";
        case Action::SPEED_3:      return "Speed 3";
        case Action::CONFIRM:      return "Confirm";
        case Action::CANCEL:       return "Cancel";
        case Action::MENU:         return "Menu";
        case Action::DEBUG_TOGGLE: return "Debug Toggle";
        case Action::DEBUG_CAMERA: return "Debug Camera";
        case Action::DEBUG_GRID:   return "Debug Grid";
        case Action::DEBUG_WIREFRAME: return "Debug Wireframe";
        case Action::DEBUG_STATS: return "Debug Stats";
        case Action::DEBUG_BOUNDING_BOX: return "Debug Bounding Box";
        case Action::DEBUG_TERRAIN_ELEVATION: return "Terrain Elevation Heatmap";
        case Action::DEBUG_TERRAIN_TYPE: return "Terrain Type Colormap";
        case Action::DEBUG_TERRAIN_CHUNK: return "Terrain Chunk Boundaries";
        case Action::DEBUG_TERRAIN_LOD: return "Terrain LOD Level";
        case Action::DEBUG_TERRAIN_NORMALS: return "Terrain Normals";
        case Action::DEBUG_TERRAIN_WATER: return "Terrain Water Body ID";
        case Action::DEBUG_TERRAIN_BUILDABILITY: return "Terrain Buildability";
        case Action::TOGGLE_UNDERGROUND: return "Toggle Underground";
        case Action::ZONE_HABITATION: return "Zone Habitation";
        case Action::ZONE_EXCHANGE: return "Zone Exchange";
        case Action::ZONE_FABRICATION: return "Zone Fabrication";
        case Action::ZONE_PLACE: return "Zone Place";
        case Action::ZONE_DEMOLISH: return "Zone Demolish";
        case Action::ENERGY_NEXUS_CARBON: return "Energy Nexus Carbon";
        case Action::ENERGY_NEXUS_WIND: return "Energy Nexus Wind";
        case Action::ENERGY_NEXUS_SOLAR: return "Energy Nexus Solar";
        case Action::ENERGY_CONDUIT: return "Energy Conduit";
        case Action::ENERGY_PLACE: return "Energy Place";
        case Action::ENERGY_REMOVE: return "Energy Remove";
        case Action::ENERGY_OVERLAY: return "Energy Overlay";
        case Action::FLUID_EXTRACTOR: return "Fluid Extractor";
        case Action::FLUID_RESERVOIR: return "Fluid Reservoir";
        case Action::FLUID_CONDUIT: return "Fluid Conduit";
        case Action::FLUID_PLACE: return "Fluid Place";
        case Action::FLUID_REMOVE: return "Fluid Remove";
        case Action::FLUID_OVERLAY: return "Fluid Overlay";
        case Action::FULLSCREEN:   return "Fullscreen";
        case Action::QUIT:         return "Quit";
        default:                   return "Unknown";
    }
}

} // namespace sims3000
