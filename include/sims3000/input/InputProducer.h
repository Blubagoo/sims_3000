/**
 * @file InputProducer.h
 * @brief Converts raw input events to network InputMessage.
 *
 * InputProducer bridges between the local InputSystem and the network layer:
 * - Reads current input state from InputSystem
 * - Interprets context (selected tool, cursor position)
 * - Produces InputMessage for network transmission
 * - Tracks action sequence numbers
 *
 * This is the client-side component that creates InputMessage objects
 * from player actions. It integrates with PendingActionTracker for
 * optimistic UI hints.
 *
 * Ownership: Application owns InputProducer.
 * Thread safety: All methods called from main thread only.
 */

#ifndef SIMS3000_INPUT_INPUTPRODUCER_H
#define SIMS3000_INPUT_INPUTPRODUCER_H

#include "sims3000/net/InputMessage.h"
#include "sims3000/core/types.h"

#include <queue>
#include <functional>
#include <optional>
#include <cstdint>

namespace sims3000 {

// Forward declarations
class InputSystem;
class PendingActionTracker;

/**
 * @enum ToolType
 * @brief Currently selected player tool.
 */
enum class ToolType : std::uint8_t {
    None = 0,
    Select,
    Bulldoze,
    Zone,
    Road,
    PowerLine,
    Pipe,
    Building,
    Query
};

/**
 * @struct ToolState
 * @brief Current tool selection and parameters.
 */
struct ToolState {
    ToolType tool = ToolType::None;
    std::uint32_t subType = 0;       ///< Zone type, building type, etc.
    std::uint32_t modifier = 0;      ///< Density, rotation, etc.
};

/**
 * @class InputProducer
 * @brief Produces InputMessage from player actions.
 *
 * Example usage:
 * @code
 *   InputSystem input;
 *   NetworkClient client(...);
 *   PendingActionTracker tracker;
 *   InputProducer producer(input, tracker);
 *
 *   // In game loop
 *   producer.setPlayerId(client.getPlayerId());
 *   producer.setCurrentTick(simClock.getCurrentTick());
 *
 *   // Set tool from UI
 *   producer.setTool(ToolType::Building, buildingType);
 *
 *   // Update cursor position from mouse
 *   producer.setCursorPosition({mouseGridX, mouseGridY});
 *
 *   // Process player actions each frame
 *   producer.update();
 *
 *   // Send produced inputs to server
 *   while (auto input = producer.pollInput()) {
 *       client.queueInput(*input);
 *   }
 * @endcode
 */
class InputProducer {
public:
    /**
     * @brief Construct an InputProducer.
     * @param inputSystem Reference to the input system for reading input state.
     * @param tracker Reference to pending action tracker (optional, for tracking).
     */
    explicit InputProducer(InputSystem& inputSystem, PendingActionTracker* tracker = nullptr);
    ~InputProducer() = default;

    // Non-copyable
    InputProducer(const InputProducer&) = delete;
    InputProducer& operator=(const InputProducer&) = delete;

    // =========================================================================
    // Configuration
    // =========================================================================

    /**
     * @brief Set the player ID for generated inputs.
     * @param playerId Player ID from NetworkClient.
     */
    void setPlayerId(PlayerID playerId) { m_playerId = playerId; }

    /**
     * @brief Set the current simulation tick.
     * @param tick Current tick for timestamping inputs.
     */
    void setCurrentTick(SimulationTick tick) { m_currentTick = tick; }

    /**
     * @brief Set the current tool.
     * @param tool Tool type.
     * @param subType Tool sub-type (zone type, building type, etc.)
     * @param modifier Tool modifier (density, rotation, etc.)
     */
    void setTool(ToolType tool, std::uint32_t subType = 0, std::uint32_t modifier = 0);

    /**
     * @brief Set cursor grid position.
     * @param pos Grid position of cursor.
     */
    void setCursorPosition(GridPosition pos) { m_cursorPos = pos; }

    /**
     * @brief Set whether player can perform actions.
     * @param enabled true if actions are allowed.
     */
    void setActionsEnabled(bool enabled) { m_actionsEnabled = enabled; }

    // =========================================================================
    // Update
    // =========================================================================

    /**
     * @brief Process input state and produce actions.
     *
     * Call once per frame after InputSystem::beginFrame() and before
     * polling for input messages.
     */
    void update();

    // =========================================================================
    // Output
    // =========================================================================

    /**
     * @brief Poll for the next produced input message.
     * @return Input message if available, nullopt otherwise.
     */
    std::optional<InputMessage> pollInput();

    /**
     * @brief Get the number of queued input messages.
     */
    std::size_t getQueuedCount() const { return m_outputQueue.size(); }

    /**
     * @brief Clear all queued inputs.
     */
    void clearQueue();

    // =========================================================================
    // State Access
    // =========================================================================

    /**
     * @brief Get current tool state.
     */
    const ToolState& getToolState() const { return m_toolState; }

    /**
     * @brief Get cursor grid position.
     */
    GridPosition getCursorPosition() const { return m_cursorPos; }

    /**
     * @brief Get next sequence number (for preview purposes).
     */
    std::uint32_t peekNextSequence() const { return m_sequenceNum + 1; }

private:
    /**
     * @brief Allocate the next sequence number.
     */
    std::uint32_t nextSequence() { return ++m_sequenceNum; }

    /**
     * @brief Create and queue an InputMessage.
     */
    void produceInput(InputType type, GridPosition pos,
                      std::uint32_t param1 = 0, std::uint32_t param2 = 0,
                      std::int32_t value = 0);

    /**
     * @brief Handle tool action (click/drag).
     */
    void handleToolAction();

    /**
     * @brief Map tool type to InputType for the current action.
     */
    InputType getInputTypeForTool() const;

    // =========================================================================
    // State
    // =========================================================================

    InputSystem& m_inputSystem;
    PendingActionTracker* m_tracker;  // Optional, may be nullptr

    PlayerID m_playerId = 0;
    SimulationTick m_currentTick = 0;
    ToolState m_toolState;
    GridPosition m_cursorPos{0, 0};
    bool m_actionsEnabled = true;

    std::uint32_t m_sequenceNum = 0;
    std::queue<InputMessage> m_outputQueue;

    // Drag tracking for line tools
    bool m_dragging = false;
    GridPosition m_dragStart{0, 0};
};

} // namespace sims3000

#endif // SIMS3000_INPUT_INPUTPRODUCER_H
