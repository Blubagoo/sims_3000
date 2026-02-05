/**
 * @file InputMessage.h
 * @brief Input message structure for client-server communication.
 */

#ifndef SIMS3000_NET_INPUTMESSAGE_H
#define SIMS3000_NET_INPUTMESSAGE_H

#include "sims3000/core/types.h"
#include "sims3000/core/Serialization.h"
#include <cstdint>

namespace sims3000 {

/**
 * @enum InputType
 * @brief Types of player input actions.
 */
enum class InputType : std::uint8_t {
    None = 0,

    // Building placement
    PlaceBuilding,
    DemolishBuilding,
    UpgradeBuilding,

    // Zone management
    SetZone,
    ClearZone,

    // Infrastructure
    PlaceRoad,
    PlacePipe,
    PlacePowerLine,

    // Economy
    SetTaxRate,
    TakeLoan,
    RepayLoan,

    // Camera (client-only, not sent to server)
    CameraMove,
    CameraZoom,

    // Game control
    PauseGame,
    SetGameSpeed,

    COUNT
};

/**
 * @struct InputMessage
 * @brief Player input message sent from client to server.
 *
 * Each input is timestamped with the client's tick for server validation
 * and reconciliation. Server processes inputs in tick order.
 */
struct InputMessage : public ISerializable {
    SimulationTick tick = 0;      // Client tick when input was generated
    PlayerID playerId = 0;        // Source player
    InputType type = InputType::None;
    std::uint32_t sequenceNum = 0; // For acknowledgment/replay

    // Payload data (interpretation depends on type)
    GridPosition targetPos{0, 0};
    std::uint32_t param1 = 0;     // Building type, zone type, etc.
    std::uint32_t param2 = 0;     // Additional parameter
    std::int32_t value = 0;       // Signed value (tax rate, etc.)

    void serialize(WriteBuffer& buffer) const override {
        buffer.writeU64(tick);
        buffer.writeU8(playerId);
        buffer.writeU8(static_cast<std::uint8_t>(type));
        buffer.writeU32(sequenceNum);
        buffer.writeI16(targetPos.x);
        buffer.writeI16(targetPos.y);
        buffer.writeU32(param1);
        buffer.writeU32(param2);
        buffer.writeI32(value);
    }

    void deserialize(ReadBuffer& buffer) override {
        tick = buffer.readU64();
        playerId = buffer.readU8();
        type = static_cast<InputType>(buffer.readU8());
        sequenceNum = buffer.readU32();
        targetPos.x = buffer.readI16();
        targetPos.y = buffer.readI16();
        param1 = buffer.readU32();
        param2 = buffer.readU32();
        value = buffer.readI32();
    }

    static constexpr std::size_t SERIALIZED_SIZE = 8 + 1 + 1 + 4 + 2 + 2 + 4 + 4 + 4; // 30 bytes
};

/**
 * @struct InputAck
 * @brief Server acknowledgment of processed input.
 */
struct InputAck : public ISerializable {
    SimulationTick serverTick = 0;   // Server tick when processed
    std::uint32_t sequenceNum = 0;    // Matches InputMessage::sequenceNum
    bool accepted = false;            // Was input valid/accepted?
    std::uint8_t errorCode = 0;       // Error reason if rejected

    void serialize(WriteBuffer& buffer) const override {
        buffer.writeU64(serverTick);
        buffer.writeU32(sequenceNum);
        buffer.writeU8(accepted ? 1 : 0);
        buffer.writeU8(errorCode);
    }

    void deserialize(ReadBuffer& buffer) override {
        serverTick = buffer.readU64();
        sequenceNum = buffer.readU32();
        accepted = buffer.readU8() != 0;
        errorCode = buffer.readU8();
    }
};

} // namespace sims3000

#endif // SIMS3000_NET_INPUTMESSAGE_H
