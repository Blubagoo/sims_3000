/**
 * @file test_persistence_provider.cpp
 * @brief Unit tests for IPersistenceProvider implementations.
 *
 * Tests:
 * - NullPersistenceProvider behavior
 * - FilePersistenceProvider save/load
 * - EntityIdGenerator state serialization
 * - PlayerSession state serialization
 * - Error handling for corrupt/missing data
 * - Atomic file operations
 */

#include "sims3000/persistence/IPersistenceProvider.h"
#include "sims3000/persistence/NullPersistenceProvider.h"
#include "sims3000/persistence/FilePersistenceProvider.h"
#include "sims3000/sync/EntityIdGenerator.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <random>

using namespace sims3000;

// Test counter
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
            ++g_testsFailed; \
            return; \
        } \
    } while (0)

#define TEST_PASS(name) \
    do { \
        std::printf("PASS: %s\n", name); \
        ++g_testsPassed; \
    } while (0)

// =============================================================================
// Test Utilities
// =============================================================================

std::string getTestFilePath() {
    // Use temp directory with unique name
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 99999);

    std::string path = std::filesystem::temp_directory_path().string();
    path += "/sims3000_test_" + std::to_string(dis(gen)) + ".bin";
    return path;
}

void cleanupTestFile(const std::string& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path + ".bak", ec);
    std::filesystem::remove(path + ".tmp", ec);
}

PersistentPlayerSession createTestSession(std::uint8_t playerId, const std::string& name) {
    PersistentPlayerSession session;
    session.playerId = playerId;
    session.playerName = name;
    session.createdAt = 1000000 + playerId * 1000;
    session.disconnectedAt = 0;
    session.wasConnected = true;

    // Generate pseudo-random token
    for (std::size_t i = 0; i < PERSISTENCE_SESSION_TOKEN_SIZE; ++i) {
        session.token[i] = static_cast<std::uint8_t>((playerId * 17 + i * 31) & 0xFF);
    }

    return session;
}

// =============================================================================
// NullPersistenceProvider Tests
// =============================================================================

void test_NullProvider_SaveAlwaysSucceeds() {
    NullPersistenceProvider provider;

    TEST_ASSERT(provider.saveEntityIdState(100), "saveEntityIdState should succeed");

    std::vector<PersistentPlayerSession> sessions;
    sessions.push_back(createTestSession(1, "TestPlayer"));
    TEST_ASSERT(provider.savePlayerSessions(sessions), "savePlayerSessions should succeed");

    PersistentServerState state;
    state.nextEntityId = 500;
    TEST_ASSERT(provider.saveServerState(state), "saveServerState should succeed");

    TEST_PASS("test_NullProvider_SaveAlwaysSucceeds");
}

void test_NullProvider_LoadAlwaysEmpty() {
    NullPersistenceProvider provider;

    TEST_ASSERT(!provider.loadEntityIdState().has_value(), "loadEntityIdState should return empty");
    TEST_ASSERT(!provider.loadPlayerSessions().has_value(), "loadPlayerSessions should return empty");
    TEST_ASSERT(!provider.loadServerState().has_value(), "loadServerState should return empty");
    TEST_ASSERT(!provider.hasState(), "hasState should be false");

    TEST_PASS("test_NullProvider_LoadAlwaysEmpty");
}

void test_NullProvider_ClearSucceeds() {
    NullPersistenceProvider provider;

    TEST_ASSERT(provider.clearState(), "clearState should succeed");
    TEST_ASSERT(provider.getStorageLocation() == "null", "Storage location should be 'null'");

    TEST_PASS("test_NullProvider_ClearSucceeds");
}

// =============================================================================
// FilePersistenceProvider Tests
// =============================================================================

void test_FileProvider_SaveLoadEntityId() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Initially no state
    TEST_ASSERT(!provider.hasState(), "Should have no state initially");
    TEST_ASSERT(!provider.loadEntityIdState().has_value(), "Load should return empty initially");

    // Save entity ID
    const std::uint64_t testId = 12345678901234ULL;
    TEST_ASSERT(provider.saveEntityIdState(testId), "Save should succeed");
    TEST_ASSERT(provider.hasState(), "Should have state after save");

    // Load entity ID
    auto loaded = provider.loadEntityIdState();
    TEST_ASSERT(loaded.has_value(), "Load should succeed");
    TEST_ASSERT(*loaded == testId, "Loaded ID should match saved ID");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_SaveLoadEntityId");
}

void test_FileProvider_SaveLoadSessions() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Create test sessions
    std::vector<PersistentPlayerSession> sessions;
    sessions.push_back(createTestSession(1, "Alice"));
    sessions.push_back(createTestSession(2, "Bob"));
    sessions.push_back(createTestSession(3, "Charlie"));

    // Modify one session to be disconnected
    sessions[1].disconnectedAt = 2000000;
    sessions[1].wasConnected = false;

    // Save sessions
    TEST_ASSERT(provider.savePlayerSessions(sessions), "Save sessions should succeed");

    // Load sessions
    auto loaded = provider.loadPlayerSessions();
    TEST_ASSERT(loaded.has_value(), "Load sessions should succeed");
    TEST_ASSERT(loaded->size() == 3, "Should have 3 sessions");

    // Verify session data
    TEST_ASSERT((*loaded)[0].playerId == 1, "Player 1 ID correct");
    TEST_ASSERT((*loaded)[0].playerName == "Alice", "Player 1 name correct");
    TEST_ASSERT((*loaded)[0].wasConnected == true, "Player 1 was connected");

    TEST_ASSERT((*loaded)[1].playerId == 2, "Player 2 ID correct");
    TEST_ASSERT((*loaded)[1].playerName == "Bob", "Player 2 name correct");
    TEST_ASSERT((*loaded)[1].disconnectedAt == 2000000, "Player 2 disconnect time correct");
    TEST_ASSERT((*loaded)[1].wasConnected == false, "Player 2 was not connected");

    TEST_ASSERT((*loaded)[2].playerId == 3, "Player 3 ID correct");
    TEST_ASSERT((*loaded)[2].playerName == "Charlie", "Player 3 name correct");

    // Verify tokens
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t j = 0; j < PERSISTENCE_SESSION_TOKEN_SIZE; ++j) {
            TEST_ASSERT((*loaded)[i].token[j] == sessions[i].token[j],
                        "Session token should match");
        }
    }

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_SaveLoadSessions");
}

void test_FileProvider_SaveLoadCompleteState() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Create complete state
    PersistentServerState state;
    state.version = 1;
    state.nextEntityId = 9999;
    state.savedAt = 1234567890;
    state.sessions.push_back(createTestSession(1, "Player1"));
    state.sessions.push_back(createTestSession(2, "Player2"));

    // Save
    TEST_ASSERT(provider.saveServerState(state), "Save state should succeed");

    // Load
    auto loaded = provider.loadServerState();
    TEST_ASSERT(loaded.has_value(), "Load state should succeed");
    TEST_ASSERT(loaded->version == 1, "Version should match");
    TEST_ASSERT(loaded->nextEntityId == 9999, "Next entity ID should match");
    TEST_ASSERT(loaded->sessions.size() == 2, "Session count should match");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_SaveLoadCompleteState");
}

void test_FileProvider_ClearState() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Save some state
    PersistentServerState state;
    state.nextEntityId = 100;
    TEST_ASSERT(provider.saveServerState(state), "Save should succeed");
    TEST_ASSERT(provider.hasState(), "Should have state");

    // Clear
    TEST_ASSERT(provider.clearState(), "Clear should succeed");
    TEST_ASSERT(!provider.hasState(), "Should have no state after clear");
    TEST_ASSERT(!provider.loadServerState().has_value(), "Load should return empty after clear");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_ClearState");
}

void test_FileProvider_CorruptDataHandling() {
    std::string path = getTestFilePath();

    // Write corrupt data
    {
        std::ofstream file(path, std::ios::binary);
        const char* garbage = "This is not valid persistence data!";
        file.write(garbage, std::strlen(garbage));
    }

    FilePersistenceProvider provider(path);

    // Load should fail gracefully
    TEST_ASSERT(!provider.loadServerState().has_value(),
                "Load should return empty for corrupt data");
    TEST_ASSERT(!provider.loadEntityIdState().has_value(),
                "Load entity ID should return empty for corrupt data");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_CorruptDataHandling");
}

void test_FileProvider_WrongMagicBytes() {
    std::string path = getTestFilePath();

    // Write data with wrong magic
    {
        std::ofstream file(path, std::ios::binary);
        std::uint32_t wrongMagic = 0x12345678;
        file.write(reinterpret_cast<const char*>(&wrongMagic), sizeof(wrongMagic));

        // Add some padding to make file non-trivially sized
        char padding[100] = {0};
        file.write(padding, sizeof(padding));
    }

    FilePersistenceProvider provider(path);

    TEST_ASSERT(!provider.loadServerState().has_value(),
                "Load should fail for wrong magic bytes");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_WrongMagicBytes");
}

void test_FileProvider_ChecksumValidation() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Save valid state
    PersistentServerState state;
    state.nextEntityId = 42;
    TEST_ASSERT(provider.saveServerState(state), "Save should succeed");

    // Corrupt the file by modifying a byte in the middle
    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(20); // Skip past header to data section
        char corrupted = 0xFF;
        file.write(&corrupted, 1);
    }

    // Load should fail due to checksum mismatch
    TEST_ASSERT(!provider.loadServerState().has_value(),
                "Load should fail for corrupted checksum");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_ChecksumValidation");
}

void test_FileProvider_BackupCreation() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Save initial state
    PersistentServerState state1;
    state1.nextEntityId = 100;
    TEST_ASSERT(provider.saveServerState(state1), "First save should succeed");

    // Save updated state (should create backup)
    PersistentServerState state2;
    state2.nextEntityId = 200;
    TEST_ASSERT(provider.saveServerState(state2), "Second save should succeed");

    // Verify backup exists
    std::error_code ec;
    TEST_ASSERT(std::filesystem::exists(provider.getBackupPath(), ec),
                "Backup file should exist");

    // Verify main file has new data
    auto loaded = provider.loadServerState();
    TEST_ASSERT(loaded.has_value(), "Load should succeed");
    TEST_ASSERT(loaded->nextEntityId == 200, "Should have new entity ID");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_BackupCreation");
}

void test_FileProvider_StorageLocation() {
    std::string path = "/test/path/to/state.bin";
    FilePersistenceProvider provider(path);

    TEST_ASSERT(provider.getStorageLocation() == path, "Storage location should match path");
    TEST_ASSERT(provider.getBackupPath() == path + ".bak", "Backup path should be .bak");
    TEST_ASSERT(provider.getTempPath() == path + ".tmp", "Temp path should be .tmp");

    TEST_PASS("test_FileProvider_StorageLocation");
}

void test_FileProvider_EmptySessionList() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Save empty session list
    std::vector<PersistentPlayerSession> empty;
    TEST_ASSERT(provider.savePlayerSessions(empty), "Save empty sessions should succeed");

    // Load should return empty vector
    auto loaded = provider.loadPlayerSessions();
    TEST_ASSERT(loaded.has_value(), "Load should succeed");
    TEST_ASSERT(loaded->empty(), "Should have empty session list");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_EmptySessionList");
}

void test_FileProvider_LongPlayerName() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Create session with long name
    std::string longName(256, 'X'); // 256 character name
    auto session = createTestSession(1, longName);

    std::vector<PersistentPlayerSession> sessions;
    sessions.push_back(session);

    TEST_ASSERT(provider.savePlayerSessions(sessions), "Save should succeed");

    auto loaded = provider.loadPlayerSessions();
    TEST_ASSERT(loaded.has_value(), "Load should succeed");
    TEST_ASSERT(loaded->size() == 1, "Should have one session");
    TEST_ASSERT((*loaded)[0].playerName == longName, "Long name should be preserved");

    cleanupTestFile(path);
    TEST_PASS("test_FileProvider_LongPlayerName");
}

// =============================================================================
// EntityIdGenerator Integration Tests
// =============================================================================

void test_EntityIdGenerator_PersistRestore() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Use EntityIdGenerator
    EntityIdGenerator generator;

    // Generate some IDs
    for (int i = 0; i < 100; ++i) {
        generator.next();
    }

    // Save state
    std::uint64_t nextId = generator.getNextId();
    TEST_ASSERT(nextId == 101, "Next ID should be 101 after 100 generations");
    TEST_ASSERT(provider.saveEntityIdState(nextId), "Save should succeed");

    // Create new generator and restore
    EntityIdGenerator restored;
    auto loadedId = provider.loadEntityIdState();
    TEST_ASSERT(loadedId.has_value(), "Load should succeed");
    restored.restore(*loadedId);

    // Verify restored generator continues correctly
    TEST_ASSERT(restored.getNextId() == 101, "Restored generator should continue from 101");
    EntityID newId = restored.next();
    TEST_ASSERT(newId == 101, "First ID from restored should be 101");
    TEST_ASSERT(restored.getNextId() == 102, "Next ID should be 102");

    cleanupTestFile(path);
    TEST_PASS("test_EntityIdGenerator_PersistRestore");
}

void test_EntityIdGenerator_ZeroIdHandling() {
    std::string path = getTestFilePath();
    FilePersistenceProvider provider(path);

    // Save zero (invalid)
    TEST_ASSERT(provider.saveEntityIdState(0), "Save should succeed even for 0");

    // Load
    auto loaded = provider.loadEntityIdState();
    TEST_ASSERT(loaded.has_value(), "Load should succeed");
    TEST_ASSERT(*loaded == 0, "Should load 0 as saved");

    // EntityIdGenerator::restore should handle 0
    EntityIdGenerator generator;
    generator.restore(0);
    // restore(0) should set to 1 per EntityIdGenerator contract
    TEST_ASSERT(generator.getNextId() == 1, "Generator should use 1 for invalid 0");

    cleanupTestFile(path);
    TEST_PASS("test_EntityIdGenerator_ZeroIdHandling");
}

// =============================================================================
// Main
// =============================================================================

int main() {
    std::printf("=== IPersistenceProvider Tests ===\n\n");

    // NullPersistenceProvider tests
    std::printf("--- NullPersistenceProvider Tests ---\n");
    test_NullProvider_SaveAlwaysSucceeds();
    test_NullProvider_LoadAlwaysEmpty();
    test_NullProvider_ClearSucceeds();

    // FilePersistenceProvider tests
    std::printf("\n--- FilePersistenceProvider Tests ---\n");
    test_FileProvider_SaveLoadEntityId();
    test_FileProvider_SaveLoadSessions();
    test_FileProvider_SaveLoadCompleteState();
    test_FileProvider_ClearState();
    test_FileProvider_CorruptDataHandling();
    test_FileProvider_WrongMagicBytes();
    test_FileProvider_ChecksumValidation();
    test_FileProvider_BackupCreation();
    test_FileProvider_StorageLocation();
    test_FileProvider_EmptySessionList();
    test_FileProvider_LongPlayerName();

    // EntityIdGenerator integration tests
    std::printf("\n--- EntityIdGenerator Integration Tests ---\n");
    test_EntityIdGenerator_PersistRestore();
    test_EntityIdGenerator_ZeroIdHandling();

    // Summary
    std::printf("\n=== Test Summary ===\n");
    std::printf("Passed: %d\n", g_testsPassed);
    std::printf("Failed: %d\n", g_testsFailed);

    return g_testsFailed == 0 ? 0 : 1;
}
