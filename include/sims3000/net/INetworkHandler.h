/**
 * @file INetworkHandler.h
 * @brief Interface for routing network messages to appropriate handlers.
 *
 * Provides an abstraction for message handling that allows different
 * subsystems to register interest in specific message types. The NetworkServer
 * dispatches incoming messages to registered handlers.
 *
 * Ownership: NetworkServer owns the list of handlers (weak references).
 *            Actual handler objects are owned by their respective systems.
 * Thread safety: Handler callbacks are called from the main thread only.
 */

#ifndef SIMS3000_NET_INETWORKHANDLER_H
#define SIMS3000_NET_INETWORKHANDLER_H

#include "sims3000/net/NetworkMessage.h"
#include "sims3000/net/INetworkTransport.h"
#include "sims3000/core/types.h"
#include <memory>
#include <cstdint>

namespace sims3000 {

/**
 * @interface INetworkHandler
 * @brief Interface for handling incoming network messages.
 *
 * Systems that need to process network messages implement this interface
 * and register with the NetworkServer. The server routes messages based
 * on message type to the appropriate handler.
 *
 * Example usage:
 * @code
 *   class GameInputHandler : public INetworkHandler {
 *   public:
 *       bool canHandle(MessageType type) const override {
 *           return type == MessageType::Input;
 *       }
 *
 *       void handleMessage(PeerID peer, const NetworkMessage& msg) override {
 *           const auto& inputMsg = static_cast<const NetInputMessage&>(msg);
 *           // Process input...
 *       }
 *   };
 * @endcode
 */
class INetworkHandler {
public:
    virtual ~INetworkHandler() = default;

    /**
     * @brief Check if this handler can process a given message type.
     * @param type The message type to check.
     * @return true if this handler should receive messages of this type.
     */
    virtual bool canHandle(MessageType type) const = 0;

    /**
     * @brief Handle an incoming message.
     * @param peer The peer that sent the message.
     * @param msg The deserialized message.
     *
     * Called on the main thread. The message reference is valid only
     * for the duration of this call.
     */
    virtual void handleMessage(PeerID peer, const NetworkMessage& msg) = 0;

    /**
     * @brief Called when a new client connects.
     * @param peer The newly connected peer.
     *
     * Optional: Override to receive connect notifications.
     */
    virtual void onClientConnected(PeerID /*peer*/) {}

    /**
     * @brief Called when a client disconnects.
     * @param peer The disconnected peer.
     * @param timedOut True if disconnection was due to timeout.
     *
     * Optional: Override to receive disconnect notifications.
     */
    virtual void onClientDisconnected(PeerID /*peer*/, bool /*timedOut*/) {}
};

} // namespace sims3000

#endif // SIMS3000_NET_INETWORKHANDLER_H
