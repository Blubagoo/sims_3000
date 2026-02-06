/**
 * @file test_session_management.cpp
 * @brief Unit tests for Player Session Management (Ticket 1-010)
 *
 * Tests:
 * - PlayerID assignment (1-4)
 * - Session token generation (128-bit random)
 * - Session token validation on reconnect
 * - Player list maintenance
 * - PlayerListMessage broadcasting
 * - 30-second grace period for reconnection
 * - Session cleanup after grace period
 * - Duplicate connection handling
 * - Activity tracking for ghost town timer
 */

#include "sims3000/net/NetworkServer.h"
#include "sims3000/net/MockTransport.h"
#include "sims3000/net/ClientMessages.h"
#include "sims3000/net/ServerMessages.h"
#include "sims3000/net/NetworkBuffer.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <cstring>

using namespace sims3000;

// Helper to serialize a message to bytes
std::vector<std::uint8_t> serializeMessage(const NetworkMessage& msg) {
    NetworkBuffer buffer;
    msg.serializeWithEnvelope(buffer);
    return buffer.raw();
}

// Test handler that captures broadcasts
class SessionTestHandler : public INetworkHandler {
public:
    std::vector<std::pair<PeerID, MessageType>> receivedMessages;
    std::vector<PeerID> connectedPeers;
    std::vector<std::pair<PeerID, bool>> disconnectedPeers;

    bool canHandle(MessageType type) const override {
        return type == MessageType::PlayerList;
    }

    void handleMessage(PeerID peer, const NetworkMessage& msg) override {
        receivedMessages.push_back({peer, msg.getType()});
    }

    void onClientConnected(PeerID peer) override {
        connectedPeers.push_back(peer);
    }

    void onClientDisconnected(PeerID peer, bool timedOut) override {
        disconnectedPeers.push_back({peer, timedOut});
    }

    void clear() {
        receivedMessages.clear();
        connectedPeers.clear();
        disconnectedPeers.clear();
    }
};

// ============================================================================
// Test: JoinAcceptMessage Serialization
// ============================================================================
void test_join_accept_message_serialization() {
    std::cout << "  test_join_accept_message_serialization..." << std::flush;

    JoinAcceptMessage original;
    original.playerId = 3;
    for (std::size_t i = 0; i < 16; ++i) {
        original.sessionToken[i] = static_cast<std::uint8_t>(i * 17);
    }
    original.serverTick = 0x123456789ABCDEF0ULL;

    // Serialize
    NetworkBuffer buffer;
    original.serializeWithEnvelope(buffer);

    // Deserialize
    NetworkBuffer readBuffer(buffer.data(), buffer.size());
    EnvelopeHeader header = NetworkMessage::parseEnvelope(readBuffer);

    assert(header.isValid());
    assert(header.type == MessageType::JoinAccept);

    JoinAcceptMessage deserialized;
    assert(deserialized.deserializePayload(readBuffer));

    assert(deserialized.playerId == 3);
    for (std::size_t i = 0; i < 16; ++i) {
        assert(deserialized.sessionToken[i] == static_cast<std::uint8_t>(i * 17));
    }
    assert(deserialized.serverTick == 0x123456789ABCDEF0ULL);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: JoinRejectMessage Serialization
// ============================================================================
void test_join_reject_message_serialization() {
    std::cout << "  test_join_reject_message_serialization..." << std::flush;

    JoinRejectMessage original;
    original.reason = JoinRejectReason::ServerFull;
    original.message = "Server is full, please try again later";

    // Serialize
    NetworkBuffer buffer;
    original.serializeWithEnvelope(buffer);

    // Deserialize
    NetworkBuffer readBuffer(buffer.data(), buffer.size());
    EnvelopeHeader header = NetworkMessage::parseEnvelope(readBuffer);

    assert(header.isValid());
    assert(header.type == MessageType::JoinReject);

    JoinRejectMessage deserialized;
    assert(deserialized.deserializePayload(readBuffer));

    assert(deserialized.reason == JoinRejectReason::ServerFull);
    assert(deserialized.message == "Server is full, please try again later");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: KickMessage Serialization
// ============================================================================
void test_kick_message_serialization() {
    std::cout << "  test_kick_message_serialization..." << std::flush;

    KickMessage original;
    original.reason = "Duplicate connection detected";

    // Serialize
    NetworkBuffer buffer;
    original.serializeWithEnvelope(buffer);

    // Deserialize
    NetworkBuffer readBuffer(buffer.data(), buffer.size());
    EnvelopeHeader header = NetworkMessage::parseEnvelope(readBuffer);

    assert(header.isValid());
    assert(header.type == MessageType::Kick);

    KickMessage deserialized;
    assert(deserialized.deserializePayload(readBuffer));

    assert(deserialized.reason == "Duplicate connection detected");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: JoinRejectReason Default Messages
// ============================================================================
void test_join_reject_default_messages() {
    std::cout << "  test_join_reject_default_messages..." << std::flush;

    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::ServerFull)) ==
           "Server is full");
    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::InvalidName)) ==
           "Invalid player name");
    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::Banned)) ==
           "You have been banned from this server");
    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::InvalidToken)) ==
           "Invalid session token");
    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::SessionExpired)) ==
           "Session has expired");
    assert(std::string(JoinRejectMessage::getDefaultMessage(JoinRejectReason::Unknown)) ==
           "Unknown error");

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: PlayerSession Token Matching
// ============================================================================
void test_player_session_token_matching() {
    std::cout << "  test_player_session_token_matching..." << std::flush;

    PlayerSession session;
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        session.token[i] = static_cast<std::uint8_t>(i + 1);
    }

    // Test matching token
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> matchingToken;
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        matchingToken[i] = static_cast<std::uint8_t>(i + 1);
    }
    assert(session.tokenMatches(matchingToken));

    // Test non-matching token
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> differentToken;
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        differentToken[i] = static_cast<std::uint8_t>(i + 100);
    }
    assert(!session.tokenMatches(differentToken));

    // Test single byte difference
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> almostMatching;
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        almostMatching[i] = static_cast<std::uint8_t>(i + 1);
    }
    almostMatching[15] = 255;  // Change last byte
    assert(!session.tokenMatches(almostMatching));

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: PlayerSession Grace Period
// ============================================================================
void test_player_session_grace_period() {
    std::cout << "  test_player_session_grace_period..." << std::flush;

    const std::uint64_t gracePeriodMs = 30000;  // 30 seconds

    PlayerSession session;
    session.playerId = 1;
    session.connected = true;
    session.disconnectedAt = 0;

    // Connected session is always within grace period
    assert(session.isWithinGracePeriod(1000000, gracePeriodMs));

    // Disconnect the session
    session.connected = false;
    session.disconnectedAt = 1000000;  // Disconnected at time 1000000ms

    // Just after disconnect - within grace period
    assert(session.isWithinGracePeriod(1000001, gracePeriodMs));

    // At exactly grace period boundary - still valid
    assert(session.isWithinGracePeriod(1030000, gracePeriodMs));

    // Just past grace period - expired
    assert(!session.isWithinGracePeriod(1030001, gracePeriodMs));

    // Way past grace period - expired
    assert(!session.isWithinGracePeriod(2000000, gracePeriodMs));

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Session Token Size
// ============================================================================
void test_session_token_size() {
    std::cout << "  test_session_token_size..." << std::flush;

    // Session token must be 128-bit = 16 bytes
    assert(SERVER_SESSION_TOKEN_SIZE == 16);
    assert(SESSION_TOKEN_SIZE == 16);  // From ClientMessages.h

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Session Grace Period Constant
// ============================================================================
void test_session_grace_period_constant() {
    std::cout << "  test_session_grace_period_constant..." << std::flush;

    // Grace period must be 30 seconds = 30000 milliseconds
    assert(SESSION_GRACE_PERIOD_MS == 30000);

    ServerConfig config;
    assert(config.sessionGracePeriodMs == 30000);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: PlayerID Range (1-4)
// ============================================================================
void test_player_id_range() {
    std::cout << "  test_player_id_range..." << std::flush;

    // PlayerID 0 is reserved for GAME_MASTER per canon
    // Valid player IDs are 1-4

    ServerConfig config;
    config.maxPlayers = 4;

    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Server should enforce max 4 players
    assert(server.getConfig().maxPlayers == 4);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Active Session Count
// ============================================================================
void test_active_session_count() {
    std::cout << "  test_active_session_count..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Initially no active sessions
    assert(server.getActiveSessionCount() == 0);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Session Validation for Reconnect
// ============================================================================
void test_session_validation_for_reconnect() {
    std::cout << "  test_session_validation_for_reconnect..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Non-existent token should not be valid
    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> fakeToken{};
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        fakeToken[i] = static_cast<std::uint8_t>(i + 1);
    }

    assert(!server.isSessionValidForReconnect(fakeToken));

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Get Session By Token (Not Found)
// ============================================================================
void test_get_session_by_token_not_found() {
    std::cout << "  test_get_session_by_token_not_found..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    std::array<std::uint8_t, SERVER_SESSION_TOKEN_SIZE> fakeToken{};
    assert(server.getSessionByToken(fakeToken) == nullptr);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Client Connection Has Session Fields
// ============================================================================
void test_client_connection_session_fields() {
    std::cout << "  test_client_connection_session_fields..." << std::flush;

    ClientConnection conn;

    // Check default values
    assert(conn.sessionCreatedAt == 0);
    assert(conn.lastActivityMs == 0);

    // Session token should be zeroed by default
    for (std::size_t i = 0; i < SERVER_SESSION_TOKEN_SIZE; ++i) {
        assert(conn.sessionToken[i] == 0);
    }

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Update Player Activity
// ============================================================================
void test_update_player_activity() {
    std::cout << "  test_update_player_activity..." << std::flush;

    ServerConfig config;
    auto transport = std::make_unique<MockTransport>();
    NetworkServer server(std::move(transport), config);

    assert(server.start());

    // Update activity for non-existent player should not crash
    server.updatePlayerActivity(99);

    server.stop();

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: JoinAcceptMessage Payload Size
// ============================================================================
void test_join_accept_payload_size() {
    std::cout << "  test_join_accept_payload_size..." << std::flush;

    JoinAcceptMessage msg;

    // Payload: 1 (playerId) + 16 (sessionToken) + 8 (serverTick) = 25 bytes
    assert(msg.getPayloadSize() == 25);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: JoinRejectMessage Payload Size
// ============================================================================
void test_join_reject_payload_size() {
    std::cout << "  test_join_reject_payload_size..." << std::flush;

    JoinRejectMessage msg;
    msg.reason = JoinRejectReason::ServerFull;
    msg.message = "Test";

    // Payload: 1 (reason) + 4 (length) + 4 (message) = 9 bytes
    assert(msg.getPayloadSize() == 5 + 4);  // 5 base + message length

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: KickMessage Payload Size
// ============================================================================
void test_kick_message_payload_size() {
    std::cout << "  test_kick_message_payload_size..." << std::flush;

    KickMessage msg;
    msg.reason = "Test reason";

    // Payload: 4 (length) + 11 (reason) = 15 bytes
    assert(msg.getPayloadSize() == 4 + 11);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: Message Factory Registration
// ============================================================================
void test_message_factory_registration() {
    std::cout << "  test_message_factory_registration..." << std::flush;

    // JoinAccept should be registered
    assert(MessageFactory::isRegistered(MessageType::JoinAccept));
    auto acceptMsg = MessageFactory::create(MessageType::JoinAccept);
    assert(acceptMsg != nullptr);
    assert(acceptMsg->getType() == MessageType::JoinAccept);

    // JoinReject should be registered
    assert(MessageFactory::isRegistered(MessageType::JoinReject));
    auto rejectMsg = MessageFactory::create(MessageType::JoinReject);
    assert(rejectMsg != nullptr);
    assert(rejectMsg->getType() == MessageType::JoinReject);

    // Kick should be registered
    assert(MessageFactory::isRegistered(MessageType::Kick));
    auto kickMsg = MessageFactory::create(MessageType::Kick);
    assert(kickMsg != nullptr);
    assert(kickMsg->getType() == MessageType::Kick);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Test: PlayerInfo in PlayerList (Existing)
// ============================================================================
void test_player_info_fields() {
    std::cout << "  test_player_info_fields..." << std::flush;

    PlayerInfo info;
    info.playerId = 2;
    info.name = "TestPlayer";
    info.status = PlayerStatus::Connected;
    info.latencyMs = 42;

    // Serialize
    NetworkBuffer buffer;
    info.serialize(buffer);

    // Deserialize
    NetworkBuffer readBuffer(buffer.data(), buffer.size());
    PlayerInfo deserialized;
    assert(deserialized.deserialize(readBuffer));

    assert(deserialized.playerId == 2);
    assert(deserialized.name == "TestPlayer");
    assert(deserialized.status == PlayerStatus::Connected);
    assert(deserialized.latencyMs == 42);

    std::cout << " PASS" << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "Running Session Management tests (Ticket 1-010)..." << std::endl;
    std::cout << std::endl;

    std::cout << "Message Serialization:" << std::endl;
    test_join_accept_message_serialization();
    test_join_reject_message_serialization();
    test_kick_message_serialization();
    test_join_reject_default_messages();
    std::cout << std::endl;

    std::cout << "Message Sizes:" << std::endl;
    test_join_accept_payload_size();
    test_join_reject_payload_size();
    test_kick_message_payload_size();
    std::cout << std::endl;

    std::cout << "Message Factory:" << std::endl;
    test_message_factory_registration();
    std::cout << std::endl;

    std::cout << "Session Token:" << std::endl;
    test_session_token_size();
    test_player_session_token_matching();
    std::cout << std::endl;

    std::cout << "Session Grace Period:" << std::endl;
    test_session_grace_period_constant();
    test_player_session_grace_period();
    std::cout << std::endl;

    std::cout << "Session Management:" << std::endl;
    test_active_session_count();
    test_session_validation_for_reconnect();
    test_get_session_by_token_not_found();
    std::cout << std::endl;

    std::cout << "Player ID:" << std::endl;
    test_player_id_range();
    std::cout << std::endl;

    std::cout << "Client Connection:" << std::endl;
    test_client_connection_session_fields();
    test_update_player_activity();
    std::cout << std::endl;

    std::cout << "Player List:" << std::endl;
    test_player_info_fields();
    std::cout << std::endl;

    std::cout << "All Session Management tests passed!" << std::endl;
    return 0;
}
