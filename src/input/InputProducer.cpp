/**
 * @file InputProducer.cpp
 * @brief InputProducer implementation.
 */

#include "sims3000/input/InputProducer.h"
#include "sims3000/input/InputSystem.h"
#include "sims3000/input/PendingActionTracker.h"

namespace sims3000 {

InputProducer::InputProducer(InputSystem& inputSystem, PendingActionTracker* tracker)
    : m_inputSystem(inputSystem)
    , m_tracker(tracker)
{
}

// =============================================================================
// Configuration
// =============================================================================

void InputProducer::setTool(ToolType tool, std::uint32_t subType, std::uint32_t modifier) {
    m_toolState.tool = tool;
    m_toolState.subType = subType;
    m_toolState.modifier = modifier;
}

// =============================================================================
// Update
// =============================================================================

void InputProducer::update() {
    if (!m_actionsEnabled || m_playerId == 0) {
        return;
    }

    // Handle tool action on primary click
    if (m_inputSystem.wasMouseButtonPressed(MouseButton::Left)) {
        handleToolAction();
    }

    // Handle drag for line tools (roads, pipes, power lines)
    if (m_inputSystem.isDragging()) {
        // Line tools produce inputs on drag end, not during drag
        m_dragging = true;
        if (!m_inputSystem.isMouseButtonDown(MouseButton::Left)) {
            // Drag ended - produce the line
            int endX, endY;
            m_inputSystem.getDragDelta(endX, endY);
            // TODO: Implement line drawing between drag start and end
            m_dragging = false;
        }
    }
}

void InputProducer::handleToolAction() {
    switch (m_toolState.tool) {
        case ToolType::None:
        case ToolType::Select:
        case ToolType::Query:
            // These tools don't produce server actions
            break;

        case ToolType::Bulldoze:
            // Demolish at cursor position
            // TODO: Need to select the entity at cursor position
            produceInput(InputType::DemolishBuilding, m_cursorPos, 0, 0, 0);
            break;

        case ToolType::Zone:
            produceInput(InputType::SetZone, m_cursorPos,
                         m_toolState.subType,    // Zone type
                         m_toolState.modifier);  // Density
            break;

        case ToolType::Road:
            produceInput(InputType::PlaceRoad, m_cursorPos);
            break;

        case ToolType::PowerLine:
            produceInput(InputType::PlacePowerLine, m_cursorPos);
            break;

        case ToolType::Pipe:
            produceInput(InputType::PlacePipe, m_cursorPos);
            break;

        case ToolType::Building:
            produceInput(InputType::PlaceBuilding, m_cursorPos,
                         m_toolState.subType,    // Building type
                         m_toolState.modifier);  // Rotation/variant
            break;
    }
}

InputType InputProducer::getInputTypeForTool() const {
    switch (m_toolState.tool) {
        case ToolType::Bulldoze:   return InputType::DemolishBuilding;
        case ToolType::Zone:       return InputType::SetZone;
        case ToolType::Road:       return InputType::PlaceRoad;
        case ToolType::PowerLine:  return InputType::PlacePowerLine;
        case ToolType::Pipe:       return InputType::PlacePipe;
        case ToolType::Building:   return InputType::PlaceBuilding;
        default:                   return InputType::None;
    }
}

// =============================================================================
// Output
// =============================================================================

void InputProducer::produceInput(InputType type, GridPosition pos,
                                  std::uint32_t param1, std::uint32_t param2,
                                  std::int32_t value) {
    InputMessage input;
    input.tick = m_currentTick;
    input.playerId = m_playerId;
    input.type = type;
    input.sequenceNum = nextSequence();
    input.targetPos = pos;
    input.param1 = param1;
    input.param2 = param2;
    input.value = value;

    // Track in pending action tracker for visual feedback
    if (m_tracker) {
        m_tracker->trackAction(input);
    }

    m_outputQueue.push(input);
}

std::optional<InputMessage> InputProducer::pollInput() {
    if (m_outputQueue.empty()) {
        return std::nullopt;
    }

    InputMessage input = std::move(m_outputQueue.front());
    m_outputQueue.pop();
    return input;
}

void InputProducer::clearQueue() {
    while (!m_outputQueue.empty()) {
        m_outputQueue.pop();
    }
}

} // namespace sims3000
