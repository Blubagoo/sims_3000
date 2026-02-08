/**
 * @file ActionMapping.h
 * @brief Configurable action-to-key bindings.
 */

#ifndef SIMS3000_INPUT_ACTIONMAPPING_H
#define SIMS3000_INPUT_ACTIONMAPPING_H

#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>

namespace sims3000 {

/**
 * @enum Action
 * @brief Game actions that can be bound to keys.
 */
enum class Action {
    // Camera movement
    PAN_UP,
    PAN_DOWN,
    PAN_LEFT,
    PAN_RIGHT,
    ZOOM_IN,
    ZOOM_OUT,
    ROTATE_CW,
    ROTATE_CCW,

    // Game controls
    PAUSE,
    SPEED_1,
    SPEED_2,
    SPEED_3,

    // UI navigation
    CONFIRM,
    CANCEL,
    MENU,

    // Debug
    DEBUG_TOGGLE,
    DEBUG_CAMERA,
    DEBUG_GRID,
    DEBUG_WIREFRAME,
    DEBUG_STATS,  // Toggle render statistics overlay (F3)
    DEBUG_BOUNDING_BOX,  // Toggle bounding box visualization (B)

    // Terrain debug modes (F5-F12 range)
    DEBUG_TERRAIN_ELEVATION,   // Toggle elevation heatmap (F5)
    DEBUG_TERRAIN_TYPE,        // Toggle terrain type colormap (F6)
    DEBUG_TERRAIN_CHUNK,       // Toggle chunk boundary visualization (F7)
    DEBUG_TERRAIN_LOD,         // Toggle LOD level visualization (F8)
    DEBUG_TERRAIN_NORMALS,     // Toggle normals visualization (F9)
    DEBUG_TERRAIN_WATER,       // Toggle water body ID visualization (F10)
    DEBUG_TERRAIN_BUILDABILITY, // Toggle buildability overlay (F12)

    // View modes
    TOGGLE_UNDERGROUND,

    // Zone/Building demo controls (Epic 4)
    ZONE_HABITATION,    // Select habitation zone mode (1)
    ZONE_EXCHANGE,      // Select exchange zone mode (2)
    ZONE_FABRICATION,   // Select fabrication zone mode (3)
    ZONE_PLACE,         // Place zone at camera focus (Z)
    ZONE_DEMOLISH,      // Demolish zone at camera focus (X)

    // Energy demo controls (Epic 5)
    ENERGY_NEXUS_CARBON,    // Select carbon nexus mode (7)
    ENERGY_NEXUS_WIND,      // Select wind nexus mode (8)
    ENERGY_NEXUS_SOLAR,     // Select solar nexus mode (9)
    ENERGY_CONDUIT,         // Select conduit mode (0)
    ENERGY_PLACE,           // Place energy item at camera focus (C)
    ENERGY_REMOVE,          // Remove energy item (V)
    ENERGY_OVERLAY,         // Toggle energy overlay (G)

    // Fluid demo controls (Epic 6)
    FLUID_EXTRACTOR,        // Select extractor mode (U)
    FLUID_RESERVOIR,        // Select reservoir mode (I)
    FLUID_CONDUIT,          // Select fluid conduit mode (O)
    FLUID_PLACE,            // Place fluid item at camera focus (N)
    FLUID_REMOVE,           // Remove fluid item (M)
    FLUID_OVERLAY,          // Toggle fluid overlay (H)

    // Transport demo controls (Epic 7)
    TRANSPORT_BASIC,      // Select basic pathway mode (T)
    TRANSPORT_CORRIDOR,   // Select transit corridor mode (Y)
    TRANSPORT_RAIL,       // Select rail mode (G)
    TRANSPORT_TERMINAL,   // Select terminal mode (J)
    TRANSPORT_PLACE,      // Place transport item (B)
    TRANSPORT_REMOVE,     // Remove transport item (V)
    TRANSPORT_OVERLAY,    // Toggle transport overlay (P)

    // Port demo controls (Epic 8)
    PORT_AERO,          // Select aero port mode (L)
    PORT_AQUA,          // Select aqua port mode (K)
    PORT_PLACE,         // Place port at camera focus (,)
    PORT_REMOVE,        // Remove port at camera focus (.)

    // System
    FULLSCREEN,
    QUIT,

    COUNT  // Must be last
};

/**
 * @class ActionMapping
 * @brief Maps actions to keyboard/mouse inputs.
 *
 * Supports multiple keys per action and provides
 * methods to query active bindings.
 */
class ActionMapping {
public:
    ActionMapping();
    ~ActionMapping() = default;

    /**
     * Bind a key to an action.
     * @param action Action to bind
     * @param scancode Key to bind
     * @param append If true, add to existing bindings; otherwise replace
     */
    void bind(Action action, SDL_Scancode scancode, bool append = false);

    /**
     * Unbind a key from an action.
     * @param action Action to unbind from
     * @param scancode Key to unbind
     */
    void unbind(Action action, SDL_Scancode scancode);

    /**
     * Clear all bindings for an action.
     * @param action Action to clear
     */
    void clearBindings(Action action);

    /**
     * Get all keys bound to an action.
     * @param action Action to query
     * @return Vector of bound scancodes
     */
    const std::vector<SDL_Scancode>& getBindings(Action action) const;

    /**
     * Find action(s) bound to a key.
     * @param scancode Key to look up
     * @return Vector of actions bound to this key
     */
    std::vector<Action> getActionsForKey(SDL_Scancode scancode) const;

    /**
     * Reset to default bindings.
     */
    void resetToDefaults();

    /**
     * Get human-readable name for an action.
     * @param action Action to name
     * @return Action name string
     */
    static const char* getActionName(Action action);

private:
    std::unordered_map<Action, std::vector<SDL_Scancode>> m_bindings;
    static const std::vector<SDL_Scancode> s_emptyBindings;
};

} // namespace sims3000

#endif // SIMS3000_INPUT_ACTIONMAPPING_H
