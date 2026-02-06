# Tests CMakeLists.txt

# Find testing framework (using simple CTest for now)
enable_testing()

# Test executable for core types
add_executable(test_core_types
    core/test_types.cpp
)

target_include_directories(test_core_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_core_types PRIVATE
    glm::glm
)

add_test(NAME CoreTypes COMMAND test_core_types)

# Test executable for Interpolatable template
add_executable(test_interpolatable
    core/test_interpolatable.cpp
)

target_include_directories(test_interpolatable PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_interpolatable PRIVATE
    glm::glm
)

add_test(NAME Interpolatable COMMAND test_interpolatable)

# Test executable for simulation clock
add_executable(test_simulation_clock
    app/test_simulation_clock.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationClock.cpp
)

target_include_directories(test_simulation_clock PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME SimulationClock COMMAND test_simulation_clock)

# Test executable for ECS
add_executable(test_ecs
    ecs/test_registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
)

target_include_directories(test_ecs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_ecs PRIVATE
    EnTT::EnTT
)

add_test(NAME ECS COMMAND test_ecs)

# Test executable for NetworkBuffer
add_executable(test_network_buffer
    net/test_network_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_network_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME NetworkBuffer COMMAND test_network_buffer)

# Test executable for NetworkTransport (ENet and Mock)
add_executable(test_transport
    net/test_transport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
)

target_include_directories(test_transport PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_transport PRIVATE
    unofficial::enet::enet
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_transport PRIVATE ws2_32 winmm)
endif()

add_test(NAME NetworkTransport COMMAND test_transport)

# Test executable for Component Serialization (Ticket 1-007)
add_executable(test_component_serialization
    ecs/test_component_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_component_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_component_serialization PRIVATE
    glm::glm
)

add_test(NAME ComponentSerialization COMMAND test_component_serialization)

# Test executable for NetworkMessage (Ticket 1-003)
add_executable(test_network_message
    net/test_network_message.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_network_message PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_network_message PRIVATE
    SDL3::SDL3
)

add_test(NAME NetworkMessage COMMAND test_network_message)

# Test executable for NetworkThread (Ticket 1-004)
add_executable(test_network_thread
    net/test_network_thread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
)

target_include_directories(test_network_thread PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_network_thread PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_network_thread PRIVATE ws2_32 winmm)
endif()

add_test(NAME NetworkThread COMMAND test_network_thread)

# Test executable for Client Messages (Ticket 1-005)
add_executable(test_client_messages
    net/test_client_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_client_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_client_messages PRIVATE
    SDL3::SDL3
)

add_test(NAME ClientMessages COMMAND test_client_messages)

# Test executable for SyncSystem Change Detection (Ticket 1-012, 1-013)
add_executable(test_sync_system
    sync/test_sync_system.cpp
    ${CMAKE_SOURCE_DIR}/src/sync/SyncSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_sync_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_sync_system PRIVATE
    EnTT::EnTT
    glm::glm
    lz4::lz4
    SDL3::SDL3
)

add_test(NAME SyncSystem COMMAND test_sync_system)

# Test executable for Server Messages (Ticket 1-006)
add_executable(test_server_messages
    net/test_server_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_server_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_server_messages PRIVATE
    SDL3::SDL3
    lz4::lz4
)

add_test(NAME ServerMessages COMMAND test_server_messages
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Release)

# Test executable for NetworkClient (Ticket 1-009)
add_executable(test_network_client
    net/test_network_client.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkClient.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_network_client PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_network_client PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_network_client PRIVATE ws2_32 winmm)
endif()

add_test(NAME NetworkClient COMMAND test_network_client)

# Test executable for NetworkServer (Ticket 1-008)
add_executable(test_network_server
    net/test_network_server.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_network_server PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_network_server PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_network_server PRIVATE ws2_32 winmm)
endif()

add_test(NAME NetworkServer COMMAND test_network_server)

# Test executable for Session Management (Ticket 1-010)
add_executable(test_session_management
    net/test_session_management.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_session_management PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_session_management PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_session_management PRIVATE ws2_32 winmm)
endif()

add_test(NAME SessionManagement COMMAND test_session_management)

# Test executable for InputHandler (Ticket 1-011)
add_executable(test_input_handler
    net/test_input_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/net/InputHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_input_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_input_handler PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
    EnTT::EnTT
    glm::glm
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_input_handler PRIVATE ws2_32 winmm)
endif()

add_test(NAME InputHandler COMMAND test_input_handler)

# Test executable for PendingActionTracker (Ticket 1-011)
add_executable(test_pending_action_tracker
    input/test_pending_action_tracker.cpp
    ${CMAKE_SOURCE_DIR}/src/input/PendingActionTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_pending_action_tracker PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_pending_action_tracker PRIVATE
    lz4::lz4
    SDL3::SDL3
)

add_test(NAME PendingActionTracker COMMAND test_pending_action_tracker)

# Test executable for Game Loop Integration (Ticket 1-017)
add_executable(test_game_loop_integration
    app/test_game_loop_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/app/SimulationClock.cpp
    ${CMAKE_SOURCE_DIR}/src/sync/SyncSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_game_loop_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_game_loop_integration PRIVATE
    EnTT::EnTT
    glm::glm
    lz4::lz4
    SDL3::SDL3
)

add_test(NAME GameLoopIntegration COMMAND test_game_loop_integration)

# Test executable for Testing Infrastructure (Ticket 1-019)
add_executable(test_testing_infrastructure
    net/test_testing_infrastructure.cpp
    ${CMAKE_SOURCE_DIR}/src/test/MockSocket.cpp
    ${CMAKE_SOURCE_DIR}/src/test/StateDiffer.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestServer.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestClient.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestHarness.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkClient.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_testing_infrastructure PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_testing_infrastructure PRIVATE
    EnTT::EnTT
    glm::glm
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_testing_infrastructure PRIVATE ws2_32 winmm)
endif()

add_test(NAME TestingInfrastructure COMMAND test_testing_infrastructure)

# Test executable for Connection Error Handling (Ticket 1-018)
add_executable(test_connection_error_handling
    net/test_connection_error_handling.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_connection_error_handling PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_connection_error_handling PRIVATE
    SDL3::SDL3
)

add_test(NAME ConnectionErrorHandling COMMAND test_connection_error_handling)

# ==============================================================================
# Integration Tests - Multi-Client Scenarios (Ticket 1-021)
# ==============================================================================

# Common source files for integration tests
set(INTEGRATION_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/test/MockSocket.cpp
    ${CMAKE_SOURCE_DIR}/src/test/StateDiffer.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestServer.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestClient.cpp
    ${CMAKE_SOURCE_DIR}/src/test/TestHarness.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkClient.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/InputHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

# Common libraries for integration tests
set(INTEGRATION_TEST_LIBS
    EnTT::EnTT
    glm::glm
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Smoke tests - run on every commit (target: under 30 seconds)
add_executable(test_multi_client_smoke
    integration/test_multi_client_smoke.cpp
    ${INTEGRATION_TEST_SOURCES}
)

target_include_directories(test_multi_client_smoke PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_multi_client_smoke PRIVATE
    ${INTEGRATION_TEST_LIBS}
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_multi_client_smoke PRIVATE ws2_32 winmm)
endif()

add_test(NAME MultiClientSmoke COMMAND test_multi_client_smoke)

# Scenario tests - run on PR merge (target: under 5 minutes)
add_executable(test_multi_client_scenarios
    integration/test_multi_client_scenarios.cpp
    ${INTEGRATION_TEST_SOURCES}
)

target_include_directories(test_multi_client_scenarios PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_multi_client_scenarios PRIVATE
    ${INTEGRATION_TEST_LIBS}
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_multi_client_scenarios PRIVATE ws2_32 winmm)
endif()

add_test(NAME MultiClientScenarios COMMAND test_multi_client_scenarios)

# Network edge cases tests - adverse conditions (Ticket 1-022)
add_executable(test_network_edge_cases
    integration/test_network_edge_cases.cpp
    ${INTEGRATION_TEST_SOURCES}
)

target_include_directories(test_network_edge_cases PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_network_edge_cases PRIVATE
    ${INTEGRATION_TEST_LIBS}
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_network_edge_cases PRIVATE ws2_32 winmm)
endif()

add_test(NAME NetworkEdgeCases COMMAND test_network_edge_cases)

# Test executable for EntityIdGenerator (Ticket 1-015)
add_executable(test_entity_id_generator
    sync/test_entity_id_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/sync/EntityIdGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_entity_id_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_entity_id_generator PRIVATE
    EnTT::EnTT
    glm::glm
)

add_test(NAME EntityIdGenerator COMMAND test_entity_id_generator)

# Test executable for IPersistenceProvider (Ticket 1-016)
add_executable(test_persistence_provider
    persistence/test_persistence_provider.cpp
    ${CMAKE_SOURCE_DIR}/src/persistence/FilePersistenceProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/sync/EntityIdGenerator.cpp
)

target_include_directories(test_persistence_provider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME PersistenceProvider COMMAND test_persistence_provider)

# Test executable for Serialization and State Machine (Ticket 1-020)
add_executable(test_serialization
    net/test_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ClientMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ServerMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkClient.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkServer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkThread.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ENetTransport.cpp
    ${CMAKE_SOURCE_DIR}/src/net/RateLimiter.cpp
    ${CMAKE_SOURCE_DIR}/src/net/ConnectionValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_serialization PRIVATE
    unofficial::enet::enet
    readerwriterqueue::readerwriterqueue
    lz4::lz4
    SDL3::SDL3
)

# Windows needs ws2_32 for networking
if(WIN32)
    target_link_libraries(test_serialization PRIVATE ws2_32 winmm)
endif()

add_test(NAME Serialization COMMAND test_serialization)
