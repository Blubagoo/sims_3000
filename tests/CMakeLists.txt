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

# Test executable for RenderComponent (Ticket 2-031)
add_executable(test_render_component
    ecs/test_render_component.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_render_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_render_component PRIVATE
    glm::glm
)

add_test(NAME RenderComponent COMMAND test_render_component)

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

# Test executable for GPUDevice (Ticket 2-001)
add_executable(test_gpu_device
    render/test_gpu_device.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_gpu_device PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_gpu_device PRIVATE
    SDL3::SDL3
)

# GPU tests require display - mark as requiring manual verification on headless CI
add_test(NAME GPUDevice COMMAND test_gpu_device)

# Test executable for CameraState (Ticket 2-017)
add_executable(test_camera_state
    render/test_camera_state.cpp
)

target_include_directories(test_camera_state PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_camera_state PRIVATE
    glm::glm
)

add_test(NAME CameraState COMMAND test_camera_state)

# Test executable for RenderLayer (Ticket 2-034)
add_executable(test_render_layer
    render/test_render_layer.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_render_layer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_render_layer PRIVATE
    glm::glm
)

add_test(NAME RenderLayer COMMAND test_render_layer)

# Test executable for TransformComponent (Ticket 2-032)
add_executable(test_transform_component
    ecs/test_transform_component.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_transform_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_transform_component PRIVATE
    glm::glm
)

add_test(NAME TransformComponent COMMAND test_transform_component)

# Test executable for Window (Ticket 2-002)
add_executable(test_window
    render/test_window.cpp
    ${CMAKE_SOURCE_DIR}/src/render/Window.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_window PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_window PRIVATE
    SDL3::SDL3
)

# Window/GPU tests require display - mark as requiring manual verification on headless CI
add_test(NAME Window COMMAND test_window)

# Test executable for DepthBuffer (Ticket 2-014)
add_executable(test_depth_buffer
    render/test_depth_buffer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/DepthBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_depth_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_depth_buffer PRIVATE
    SDL3::SDL3
)

# DepthBuffer/GPU tests require display - mark as requiring manual verification on headless CI
add_test(NAME DepthBuffer COMMAND test_depth_buffer)

# Test executable for DepthState (Ticket 2-015)
add_executable(test_depth_state
    render/test_depth_state.cpp
    ${CMAKE_SOURCE_DIR}/src/render/DepthState.cpp
)

target_include_directories(test_depth_state PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_depth_state PRIVATE
    SDL3::SDL3
)

# DepthState tests do NOT require GPU - pure state configuration tests
add_test(NAME DepthState COMMAND test_depth_state)

# Test executable for GPU Resources (Ticket 2-003)
add_executable(test_gpu_resources
    render/test_gpu_resources.cpp
    ${CMAKE_SOURCE_DIR}/src/render/UniformBufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/render/SamplerCache.cpp
    ${CMAKE_SOURCE_DIR}/src/render/FrameResources.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_gpu_resources PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_gpu_resources PRIVATE
    SDL3::SDL3
)

# GPU resource tests require display - mark as requiring manual verification on headless CI
add_test(NAME GPUResources COMMAND test_gpu_resources)

# Test executable for ShaderCompiler (Ticket 2-004)
add_executable(test_shader_compiler
    render/test_shader_compiler.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ShaderCompiler.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
    ${CMAKE_BINARY_DIR}/generated/EmbeddedShaders.cpp
)

target_include_directories(test_shader_compiler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_shader_compiler PRIVATE
    SDL3::SDL3
)

# Shader tests require display - mark as requiring manual verification on headless CI
add_test(NAME ShaderCompiler COMMAND test_shader_compiler)

# Test executable for ToonShader data structures (Ticket 2-005)
add_executable(test_toon_shader
    render/test_toon_shader.cpp
)

target_include_directories(test_toon_shader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_toon_shader PRIVATE
    glm::glm
)

add_test(NAME ToonShader COMMAND test_toon_shader)

# Test executable for BlendState (Ticket 2-007)
add_executable(test_blend_state
    render/test_blend_state.cpp
    ${CMAKE_SOURCE_DIR}/src/render/BlendState.cpp
)

target_include_directories(test_blend_state PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_blend_state PRIVATE
    SDL3::SDL3
)

# BlendState tests do NOT require GPU - pure state configuration tests
add_test(NAME BlendState COMMAND test_blend_state)

# Test executable for ToonPipeline (Ticket 2-007)
add_executable(test_toon_pipeline
    render/test_toon_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/render/BlendState.cpp
    ${CMAKE_SOURCE_DIR}/src/render/DepthState.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_toon_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_toon_pipeline PRIVATE
    SDL3::SDL3
    glm::glm
)

# ToonPipeline state tests do NOT require GPU
add_test(NAME ToonPipeline COMMAND test_toon_pipeline)

# Test executable for ModelLoader (Ticket 2-009)
add_executable(test_model_loader
    assets/test_model_loader.cpp
)

target_include_directories(test_model_loader PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_model_loader PRIVATE
    glm::glm
)

add_test(NAME ModelLoader COMMAND test_model_loader)

# Test executable for GPUMesh (Ticket 2-010)
add_executable(test_gpu_mesh
    render/test_gpu_mesh.cpp
)

target_include_directories(test_gpu_mesh PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_gpu_mesh PRIVATE
    glm::glm
)

add_test(NAME GPUMesh COMMAND test_gpu_mesh)

# Test executable for ViewMatrix (Ticket 2-020)
add_executable(test_view_matrix
    render/test_view_matrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
)

target_include_directories(test_view_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_view_matrix PRIVATE
    glm::glm
)

add_test(NAME ViewMatrix COMMAND test_view_matrix)

# Test executable for ProjectionMatrix (Ticket 2-021)
add_executable(test_projection_matrix
    render/test_projection_matrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_projection_matrix PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_projection_matrix PRIVATE
    glm::glm
)

add_test(NAME ProjectionMatrix COMMAND test_projection_matrix)

# Test executable for RenderCommands (Ticket 2-011)
add_executable(test_render_commands
    render/test_render_commands.cpp
    ${CMAKE_SOURCE_DIR}/src/render/RenderCommands.cpp
    ${CMAKE_SOURCE_DIR}/src/render/UniformBufferPool.cpp
)

target_include_directories(test_render_commands PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_render_commands PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME RenderCommands COMMAND test_render_commands)

# Test executable for ToonShaderConfig (Ticket 2-008)
add_executable(test_toon_shader_config
    render/test_toon_shader_config.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
)

target_include_directories(test_toon_shader_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_toon_shader_config PRIVATE
    glm::glm
)

add_test(NAME ToonShaderConfig COMMAND test_toon_shader_config)

# Test executable for CameraUniforms (Ticket 2-022)
add_executable(test_camera_uniforms
    render/test_camera_uniforms.cpp
    ${CMAKE_SOURCE_DIR}/src/render/CameraUniforms.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_camera_uniforms PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_camera_uniforms PRIVATE
    glm::glm
)

add_test(NAME CameraUniforms COMMAND test_camera_uniforms)

# Test executable for GPU Instancing (Ticket 2-012)
add_executable(test_instancing
    render/test_instancing.cpp
    ${CMAKE_SOURCE_DIR}/src/render/RenderCommands.cpp
    ${CMAKE_SOURCE_DIR}/src/render/UniformBufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/render/InstanceBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/InstancedRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_instancing PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_instancing PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME Instancing COMMAND test_instancing)

# Test executable for EmissiveMaterial (Ticket 2-037)
add_executable(test_emissive_material
    render/test_emissive_material.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
)

target_include_directories(test_emissive_material PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_emissive_material PRIVATE
    glm::glm
)

add_test(NAME EmissiveMaterial COMMAND test_emissive_material)

# Test executable for MainRenderPass (Ticket 2-018)
# Note: This test focuses on configuration structs and constants.
# GPU-dependent tests require manual verification.
add_executable(test_main_render_pass
    render/test_main_render_pass.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
)

target_include_directories(test_main_render_pass PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_main_render_pass PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME MainRenderPass COMMAND test_main_render_pass)

# Test executable for ZoomController (Ticket 2-024)
add_executable(test_zoom_controller
    input/test_zoom_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ZoomController.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ScreenToWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/input/InputSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_zoom_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_zoom_controller PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME ZoomController COMMAND test_zoom_controller)

# Test executable for ScreenToWorld (Ticket 2-029)
add_executable(test_screen_to_world
    render/test_screen_to_world.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ScreenToWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_screen_to_world PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_screen_to_world PRIVATE
    glm::glm
)

add_test(NAME ScreenToWorld COMMAND test_screen_to_world)

# Test executable for TilePicking (Ticket 2-030)
add_executable(test_tile_picking
    render/test_tile_picking.cpp
    ${CMAKE_SOURCE_DIR}/src/render/TilePicking.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ScreenToWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_tile_picking PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_tile_picking PRIVATE
    glm::glm
)

add_test(NAME TilePicking COMMAND test_tile_picking)

# Test executable for TransparentRenderQueue (Ticket 2-016)
add_executable(test_transparent_render_queue
    render/test_transparent_render_queue.cpp
    ${CMAKE_SOURCE_DIR}/src/render/TransparentRenderQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/render/BlendState.cpp
    ${CMAKE_SOURCE_DIR}/src/render/DepthState.cpp
    ${CMAKE_SOURCE_DIR}/src/render/RenderCommands.cpp
    ${CMAKE_SOURCE_DIR}/src/render/UniformBufferPool.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/render/GPUDevice.cpp
)

target_include_directories(test_transparent_render_queue PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_transparent_render_queue PRIVATE
    SDL3::SDL3
    glm::glm
)

# TransparentRenderQueue sorting tests do NOT require GPU
add_test(NAME TransparentRenderQueue COMMAND test_transparent_render_queue)

# Test executable for EdgeDetectionPass (Ticket 2-006)
add_executable(test_edge_detection_pass
    render/test_edge_detection_pass.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
)

target_include_directories(test_edge_detection_pass PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_edge_detection_pass PRIVATE
    glm::glm
)

# EdgeDetectionPass config tests do NOT require GPU
add_test(NAME EdgeDetectionPass COMMAND test_edge_detection_pass)

# Test executable for PanController (Ticket 2-023)
add_executable(test_pan_controller
    input/test_pan_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/input/PanController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/InputSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_pan_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_pan_controller PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME PanController COMMAND test_pan_controller)

# Test executable for CameraAnimator (Ticket 2-027)
add_executable(test_camera_animator
    input/test_camera_animator.cpp
    ${CMAKE_SOURCE_DIR}/src/input/CameraAnimator.cpp
)

target_include_directories(test_camera_animator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_camera_animator PRIVATE
    glm::glm
)

add_test(NAME CameraAnimator COMMAND test_camera_animator)

# Test executable for OrbitController (Ticket 2-046)
add_executable(test_orbit_controller
    input/test_orbit_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/input/OrbitController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/InputSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_orbit_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_orbit_controller PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME OrbitController COMMAND test_orbit_controller)

# Test executable for PresetSnapController (Ticket 2-047)
add_executable(test_preset_snap_controller
    input/test_preset_snap_controller.cpp
    ${CMAKE_SOURCE_DIR}/src/input/PresetSnapController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/CameraAnimator.cpp
    ${CMAKE_SOURCE_DIR}/src/input/InputSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_preset_snap_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_preset_snap_controller PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME PresetSnapController COMMAND test_preset_snap_controller)

# Test executable for CameraModeManager (Ticket 2-048)
add_executable(test_camera_mode_manager
    input/test_camera_mode_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/input/CameraModeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/input/OrbitController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/PresetSnapController.cpp
    ${CMAKE_SOURCE_DIR}/src/input/CameraAnimator.cpp
    ${CMAKE_SOURCE_DIR}/src/input/InputSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_camera_mode_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_camera_mode_manager PRIVATE
    SDL3::SDL3
    glm::glm
)

add_test(NAME CameraModeManager COMMAND test_camera_mode_manager)

# Test executable for ViewportBounds (Ticket 2-025)
add_executable(test_viewport_bounds
    render/test_viewport_bounds.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewportBounds.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ScreenToWorld.cpp
)

target_include_directories(test_viewport_bounds PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_viewport_bounds PRIVATE
    glm::glm
)

add_test(NAME ViewportBounds COMMAND test_viewport_bounds)

# Test executable for FrustumCuller (Ticket 2-026)
add_executable(test_frustum_culler
    render/test_frustum_culler.cpp
    ${CMAKE_SOURCE_DIR}/src/render/FrustumCuller.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_frustum_culler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_frustum_culler PRIVATE
    glm::glm
)

add_test(NAME FrustumCuller COMMAND test_frustum_culler)

# Test executable for BloomPass (Ticket 2-038)
# Note: test_bloom_pass requires GPU context and SDL3 linking.
# For now, BloomPass config tests are validated through MainRenderPass tests.
# TODO: Add full SDL3 linking for standalone bloom tests if needed.
#add_executable(test_bloom_pass
#    render/test_bloom_pass.cpp
#    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
#    ${CMAKE_SOURCE_DIR}/src/render/BloomPass.cpp
#)

#target_include_directories(test_bloom_pass PRIVATE
#    ${CMAKE_SOURCE_DIR}/include
#)

#target_link_libraries(test_bloom_pass PRIVATE
#    glm::glm
#)

# BloomPass config tests do NOT require GPU
#add_test(NAME BloomPass COMMAND test_bloom_pass)

# Test executable for PositionSyncSystem (Ticket 2-033)
add_executable(test_position_sync_system
    ecs/test_position_sync_system.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/PositionSyncSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_position_sync_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_position_sync_system PRIVATE
    EnTT::EnTT
    glm::glm
)

add_test(NAME PositionSyncSystem COMMAND test_position_sync_system)

# Test executable for LayerVisibility (Ticket 2-035)
add_executable(test_layer_visibility
    render/test_layer_visibility.cpp
    ${CMAKE_SOURCE_DIR}/src/render/LayerVisibility.cpp
)

target_include_directories(test_layer_visibility PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# LayerVisibility tests do NOT require GPU - pure state configuration tests
add_test(NAME LayerVisibility COMMAND test_layer_visibility)

# Test executable for ViewMode (Ticket 2-036)
add_executable(test_view_mode
    render/test_view_mode.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMode.cpp
    ${CMAKE_SOURCE_DIR}/src/render/LayerVisibility.cpp
)

target_include_directories(test_view_mode PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ViewMode tests do NOT require GPU - pure state configuration tests
add_test(NAME ViewMode COMMAND test_view_mode)

# Test executable for DebugGridOverlay (Ticket 2-040)
add_executable(test_debug_grid_overlay
    render/test_debug_grid_overlay.cpp
)

target_include_directories(test_debug_grid_overlay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_debug_grid_overlay PRIVATE
    glm::glm
)

# DebugGridOverlay config tests do NOT require GPU
add_test(NAME DebugGridOverlay COMMAND test_debug_grid_overlay)

# Test executable for WireframeMode (Ticket 2-041)
add_executable(test_wireframe_mode
    render/test_wireframe_mode.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_wireframe_mode PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_wireframe_mode PRIVATE
    SDL3::SDL3
)

# WireframeMode config tests do NOT require GPU
add_test(NAME WireframeMode COMMAND test_wireframe_mode)

# Test executable for ShadowPass (Ticket 2-039)
add_executable(test_shadow_pass
    render/test_shadow_pass.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ShadowConfig.cpp
)

target_include_directories(test_shadow_pass PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_shadow_pass PRIVATE
    glm::glm
)

# ShadowPass config tests do NOT require GPU
add_test(NAME ShadowPass COMMAND test_shadow_pass)

# Test executable for StatsOverlay (Ticket 2-042)
add_executable(test_stats_overlay
    render/test_stats_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/app/FrameStats.cpp
)

target_include_directories(test_stats_overlay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_stats_overlay PRIVATE
    glm::glm
)

# StatsOverlay config tests do NOT require GPU
add_test(NAME StatsOverlay COMMAND test_stats_overlay)

# Test executable for DebugBoundingBoxOverlay (Ticket 2-043)
add_executable(test_debug_bounding_box_overlay
    render/test_debug_bounding_box_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_debug_bounding_box_overlay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_debug_bounding_box_overlay PRIVATE
    SDL3::SDL3
    glm::glm
)

# DebugBoundingBoxOverlay config tests do NOT require GPU
add_test(NAME DebugBoundingBoxOverlay COMMAND test_debug_bounding_box_overlay)

# Test executable for TransformInterpolationSystem (Ticket 2-044)
add_executable(test_transform_interpolation
    ecs/test_transform_interpolation.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/TransformInterpolationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Registry.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_transform_interpolation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_transform_interpolation PRIVATE
    EnTT::EnTT
    glm::glm
)

add_test(NAME TransformInterpolation COMMAND test_transform_interpolation)

# Test executable for CursorRenderer (Ticket 2-045)
add_executable(test_cursor_renderer
    render/test_cursor_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/CursorRenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ScreenToWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
)

target_include_directories(test_cursor_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_cursor_renderer PRIVATE
    glm::glm
)

# CursorRenderer tests do NOT require GPU - pure logic tests with mocks
add_test(NAME CursorRenderer COMMAND test_cursor_renderer)

# Test executable for LODSystem (Ticket 2-049)
add_executable(test_lod_system
    render/test_lod_system.cpp
    ${CMAKE_SOURCE_DIR}/src/render/LODSystem.cpp
)

target_include_directories(test_lod_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_lod_system PRIVATE
    glm::glm
)

# LODSystem tests do NOT require GPU - pure logic tests
add_test(NAME LODSystem COMMAND test_lod_system)

# Test executable for TerrainTypes (Ticket 3-001)
add_executable(test_terrain_types
    terrain/test_terrain_types.cpp
)

target_include_directories(test_terrain_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainTypes tests do NOT require external libraries - pure data structure tests
add_test(NAME TerrainTypes COMMAND test_terrain_types)

# Test executable for TerrainTypeInfo (Ticket 3-003)
add_executable(test_terrain_type_info
    terrain/test_terrain_type_info.cpp
)

target_include_directories(test_terrain_type_info PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainTypeInfo tests do NOT require external libraries - pure data structure tests
add_test(NAME TerrainTypeInfo COMMAND test_terrain_type_info)

# Test executable for TerrainGrid (Ticket 3-002)
add_executable(test_terrain_grid
    terrain/test_terrain_grid.cpp
)

target_include_directories(test_terrain_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainGrid tests do NOT require external libraries - pure data structure tests
add_test(NAME TerrainGrid COMMAND test_terrain_grid)

# Test executable for TerrainEvents and ChunkDirtyTracker (Ticket 3-004)
add_executable(test_terrain_events
    terrain/test_terrain_events.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terrain_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainEvents tests do NOT require external libraries - pure data structure tests
add_test(NAME TerrainEvents COMMAND test_terrain_events)

# Test executable for WaterData (Ticket 3-005)
add_executable(test_water_data
    terrain/test_water_data.cpp
)

target_include_directories(test_water_data PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# WaterData tests do NOT require external libraries - pure data structure tests
add_test(NAME WaterData COMMAND test_water_data)

# Test executable for ProceduralNoise (Ticket 3-007)
add_executable(test_procedural_noise
    terrain/test_procedural_noise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
)

target_include_directories(test_procedural_noise PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_procedural_noise PRIVATE /fp:strict)
else()
    target_compile_options(test_procedural_noise PRIVATE -ffp-contract=off)
endif()

# ProceduralNoise tests do NOT require external libraries - pure algorithm tests
add_test(NAME ProceduralNoise COMMAND test_procedural_noise)

# Test executable for ITerrainModifier interface (Ticket 3-017)
add_executable(test_terrain_modifier
    terrain/test_terrain_modifier.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terrain_modifier PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainModifier tests do NOT require external libraries - pure interface tests
add_test(NAME TerrainModifier COMMAND test_terrain_modifier)

# Test executable for WaterDistanceField (Ticket 3-006)
add_executable(test_water_distance_field
    terrain/test_water_distance_field.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_water_distance_field PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# WaterDistanceField tests do NOT require external libraries - pure data structure tests
add_test(NAME WaterDistanceField COMMAND test_water_distance_field)

# Test executable for ContaminationSourceQuery (Ticket 3-018)
add_executable(test_contamination_source_query
    terrain/test_contamination_source_query.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ContaminationSourceQuery.cpp
)

target_include_directories(test_contamination_source_query PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ContaminationSourceQuery tests do NOT require external libraries - pure data structure tests
add_test(NAME ContaminationSourceQuery COMMAND test_contamination_source_query)

# Test executable for ITerrainQueryable interface (Ticket 3-014)
add_executable(test_terrain_queryable
    terrain/test_terrain_queryable.cpp
)

target_include_directories(test_terrain_queryable PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ITerrainQueryable tests do NOT require external libraries - pure interface tests
add_test(NAME TerrainQueryable COMMAND test_terrain_queryable)

# Test executable for ElevationGenerator (Ticket 3-008)
add_executable(test_elevation_generator
    terrain/test_elevation_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
)

target_include_directories(test_elevation_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_elevation_generator PRIVATE /fp:strict)
else()
    target_compile_options(test_elevation_generator PRIVATE -ffp-contract=off)
endif()

# ElevationGenerator tests do NOT require external libraries - pure algorithm tests
add_test(NAME ElevationGenerator COMMAND test_elevation_generator)

# Test executable for BiomeGenerator (Ticket 3-010)
add_executable(test_biome_generator
    terrain/test_biome_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/BiomeGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_biome_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_biome_generator PRIVATE /fp:strict)
else()
    target_compile_options(test_biome_generator PRIVATE -ffp-contract=off)
endif()

# BiomeGenerator tests do NOT require external libraries - pure algorithm tests
add_test(NAME BiomeGenerator COMMAND test_biome_generator)

# Test executable for ITerrainRenderData interface (Ticket 3-016)
add_executable(test_terrain_render_data
    terrain/test_terrain_render_data.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terrain_render_data PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ITerrainRenderData tests do NOT require external libraries - pure interface tests
add_test(NAME TerrainRenderData COMMAND test_terrain_render_data)

# Test executable for WaterBodyGenerator (Ticket 3-009)
add_executable(test_water_body_generator
    terrain/test_water_body_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterBodyGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_water_body_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_water_body_generator PRIVATE /fp:strict)
else()
    target_compile_options(test_water_body_generator PRIVATE -ffp-contract=off)
endif()

# WaterBodyGenerator tests do NOT require external libraries - pure algorithm tests
add_test(NAME WaterBodyGenerator COMMAND test_water_body_generator)

# Test executable for MapSizeScaling (Ticket 3-011)
add_executable(test_map_size_scaling
    terrain/test_map_size_scaling.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterBodyGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/BiomeGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_map_size_scaling PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_map_size_scaling PRIVATE /fp:strict)
else()
    target_compile_options(test_map_size_scaling PRIVATE -ffp-contract=off)
endif()

# MapSizeScaling tests do NOT require external libraries - pure algorithm tests
add_test(NAME MapSizeScaling COMMAND test_map_size_scaling)

# Test executable for SpawnPointGenerator (Ticket 3-012)
add_executable(test_spawn_point_generator
    terrain/test_spawn_point_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/SpawnPointGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterBodyGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/BiomeGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_spawn_point_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_spawn_point_generator PRIVATE /fp:strict)
else()
    target_compile_options(test_spawn_point_generator PRIVATE -ffp-contract=off)
endif()

# SpawnPointGenerator tests do NOT require external libraries - pure algorithm tests
add_test(NAME SpawnPointGenerator COMMAND test_spawn_point_generator)

# Test executable for MapValidator (Ticket 3-013)
add_executable(test_map_validator
    terrain/test_map_validator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/MapValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/SpawnPointGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterBodyGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/BiomeGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
)

target_include_directories(test_map_validator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_map_validator PRIVATE /fp:strict)
else()
    target_compile_options(test_map_validator PRIVATE -ffp-contract=off)
endif()

# MapValidator tests do NOT require external libraries - pure algorithm tests
add_test(NAME MapValidator COMMAND test_map_validator)

# Test executable for TerrainModificationSystem (Ticket 3-019)
add_executable(test_terrain_modification_system
    terrain/test_terrain_modification_system.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainModificationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terrain_modification_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainModificationSystem tests do NOT require external libraries - pure data structure tests
add_test(NAME TerrainModificationSystem COMMAND test_terrain_modification_system)

# Test executable for GradeTerrainOperation (Ticket 3-020)
add_executable(test_grade_terrain
    terrain/test_grade_terrain.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/GradeTerrainOperation.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_grade_terrain PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_grade_terrain PRIVATE
    EnTT::EnTT
)

# GradeTerrainOperation tests require EnTT for ECS registry
add_test(NAME GradeTerrainOperation COMMAND test_grade_terrain)

# Test executable for TerraformOperation (Ticket 3-021)
add_executable(test_terraform_operation
    terrain/test_terraform_operation.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerraformOperation.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/GradeTerrainOperation.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terraform_operation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terraform_operation PRIVATE
    EnTT::EnTT
)

# TerraformOperation tests require EnTT for ECS registry
add_test(NAME TerraformOperation COMMAND test_terraform_operation)

# Test executable for TerrainNetworkMessages (Ticket 3-022)
add_executable(test_terrain_network
    terrain/test_terrain_network.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNetworkMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_network PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_network PRIVATE
    SDL3::SDL3
)

# TerrainNetwork tests do NOT require external libraries - pure message serialization tests
add_test(NAME TerrainNetwork COMMAND test_terrain_network)

# Test executable for TerrainVertex (Ticket 3-023)
add_executable(test_terrain_vertex
    terrain/test_terrain_vertex.cpp
)

target_include_directories(test_terrain_vertex PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_vertex PRIVATE
    SDL3::SDL3
)

# TerrainVertex tests do NOT require GPU - pure data structure tests
add_test(NAME TerrainVertex COMMAND test_terrain_vertex)

# Test executable for TerrainChunk (Ticket 3-023)
add_executable(test_terrain_chunk
    terrain/test_terrain_chunk.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
)

target_include_directories(test_terrain_chunk PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_chunk PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainChunk tests do NOT require GPU - pure data structure tests
add_test(NAME TerrainChunk COMMAND test_terrain_chunk)

# Test executable for TerrainNormals (Ticket 3-024)
add_executable(test_terrain_normals
    terrain/test_terrain_normals.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNormals.cpp
)

target_include_directories(test_terrain_normals PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_normals PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainNormals tests do NOT require GPU - pure algorithm tests
add_test(NAME TerrainNormals COMMAND test_terrain_normals)

# Test executable for TerrainVisualConfig (Ticket 3-026)
add_executable(test_terrain_visual_config
    render/test_terrain_visual_config.cpp
)

target_include_directories(test_terrain_visual_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_visual_config PRIVATE
    glm::glm
)

# TerrainVisualConfig tests do NOT require GPU - pure data structure tests
add_test(NAME TerrainVisualConfig COMMAND test_terrain_visual_config)

# Test executable for TerrainVisualConfig Integration (Ticket 3-039)
add_executable(test_terrain_visual_config_integration
    render/test_terrain_visual_config_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ToonShaderConfig.cpp
)

target_include_directories(test_terrain_visual_config_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_visual_config_integration PRIVATE
    glm::glm
)

if(nlohmann_json_FOUND)
    target_link_libraries(test_terrain_visual_config_integration PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(test_terrain_visual_config_integration PRIVATE SIMS3000_HAS_JSON=1)
endif()

# TerrainVisualConfig integration tests do NOT require GPU - pure data structure tests
add_test(NAME TerrainVisualConfigIntegration COMMAND test_terrain_visual_config_integration)

# Test executable for WaterMesh (Ticket 3-027)
add_executable(test_water_mesh
    terrain/test_water_mesh.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterMeshGenerator.cpp
)

target_include_directories(test_water_mesh PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_water_mesh PRIVATE
    SDL3::SDL3
    glm::glm
)

# WaterMesh tests do NOT require GPU - pure data structure and algorithm tests
add_test(NAME WaterMesh COMMAND test_water_mesh)

# Test executable for WaterVisualConfig (Ticket 3-028)
add_executable(test_water_visual_config
    render/test_water_visual_config.cpp
)

target_include_directories(test_water_visual_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_water_visual_config PRIVATE
    glm::glm
)

# WaterVisualConfig tests do NOT require GPU - pure data structure tests
add_test(NAME WaterVisualConfig COMMAND test_water_visual_config)

# Test executable for TerrainChunkMeshGenerator (Ticket 3-025)
add_executable(test_terrain_chunk_mesh_generator
    terrain/test_terrain_chunk_mesh_generator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainChunkMeshGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNormals.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_chunk_mesh_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_chunk_mesh_generator PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainChunkMeshGenerator tests do NOT require GPU - CPU-side mesh generation tests
add_test(NAME TerrainChunkMeshGenerator COMMAND test_terrain_chunk_mesh_generator)

# Test executable for VegetationPlacementGenerator (Ticket 3-029)
add_executable(test_vegetation_placement
    render/test_vegetation_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/render/VegetationPlacementGenerator.cpp
)

target_include_directories(test_vegetation_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_vegetation_placement PRIVATE
    glm::glm
)

# VegetationPlacement tests do NOT require GPU - pure algorithm tests
add_test(NAME VegetationPlacement COMMAND test_vegetation_placement)

# Test executable for TerrainModificationVisualPipeline (Ticket 3-031)
add_executable(test_terrain_modification_visual_pipeline
    terrain/test_terrain_modification_visual_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainModificationVisualPipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainChunkMeshGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNormals.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterMeshGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/render/VegetationPlacementGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_modification_visual_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_modification_visual_pipeline PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainModificationVisualPipeline tests do NOT require GPU - CPU-side logic tests
add_test(NAME TerrainModificationVisualPipeline COMMAND test_terrain_modification_visual_pipeline)

# Test executable for TerrainLODMesh (Ticket 3-032)
add_executable(test_terrain_lod_mesh
    terrain/test_terrain_lod_mesh.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainChunkMeshGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNormals.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_lod_mesh PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_lod_mesh PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainLODMesh tests do NOT require GPU - CPU-side mesh generation tests
add_test(NAME TerrainLODMesh COMMAND test_terrain_lod_mesh)

# Test executable for Terrain Skirt Geometry (Ticket 3-033)
add_executable(test_terrain_skirt
    terrain/test_terrain_skirt.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainChunkMeshGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNormals.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_skirt PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_skirt PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainSkirt tests do NOT require GPU - CPU-side mesh generation tests
add_test(NAME TerrainSkirt COMMAND test_terrain_skirt)

# Test executable for TerrainChunkCuller (Ticket 3-034)
add_executable(test_terrain_chunk_culler
    terrain/test_terrain_chunk_culler.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainChunkCuller.cpp
    ${CMAKE_SOURCE_DIR}/src/render/FrustumCuller.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ViewMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/render/ProjectionMatrix.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_chunk_culler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_chunk_culler PRIVATE
    SDL3::SDL3
    glm::glm
)

# TerrainChunkCuller tests do NOT require GPU - CPU-side culling tests
add_test(NAME TerrainChunkCuller COMMAND test_terrain_chunk_culler)

# Test executable for VegetationRenderer (Ticket 3-030)
add_executable(test_vegetation_renderer
    render/test_vegetation_renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)

target_include_directories(test_vegetation_renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_vegetation_renderer PRIVATE
    glm::glm
)

# VegetationRenderer tests do NOT require GPU - CPU-side data structure tests
add_test(NAME VegetationRenderer COMMAND test_vegetation_renderer)

# Test executable for TerrainGridSerializer (Ticket 3-036)
add_executable(test_terrain_grid_serialization
    terrain/test_terrain_grid_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainGridSerializer.cpp
)

target_include_directories(test_terrain_grid_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainGridSerializer tests do NOT require external libraries - pure serialization tests
add_test(NAME TerrainGridSerialization COMMAND test_terrain_grid_serialization)

# Test executable for TerrainDebugOverlay (Ticket 3-038)
# Tests config defaults and utility functions without GPU rendering
# Also tests key bindings (Criterion 8) via ActionMapping
add_executable(test_terrain_debug_overlay
    render/test_terrain_debug_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/input/ActionMapping.cpp
)

target_include_directories(test_terrain_debug_overlay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# TerrainDebugOverlay tests need ActionMapping for key binding verification
target_link_libraries(test_terrain_debug_overlay PRIVATE
    glm::glm
    SDL3::SDL3
)

# TerrainDebugOverlay config tests do NOT require GPU
add_test(NAME TerrainDebugOverlay COMMAND test_terrain_debug_overlay)

# Test executable for TerrainNetworkSync (Ticket 3-037)
add_executable(test_terrain_network_sync
    terrain/test_terrain_network_sync.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNetworkSync.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainClientHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ChunkDirtyTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ElevationGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/BiomeGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterBodyGenerator.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/WaterDistanceField.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/ProceduralNoise.cpp
    ${CMAKE_SOURCE_DIR}/src/terrain/TerrainNetworkMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkMessage.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Logger.cpp
)

target_include_directories(test_terrain_network_sync PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_terrain_network_sync PRIVATE
    SDL3::SDL3
)

# Use strict FP semantics for cross-platform determinism
if(MSVC)
    target_compile_options(test_terrain_network_sync PRIVATE /fp:strict)
else()
    target_compile_options(test_terrain_network_sync PRIVATE -ffp-contract=off)
endif()

# TerrainNetworkSync tests do NOT require GPU - pure network/sync logic tests
add_test(NAME TerrainNetworkSync COMMAND test_terrain_network_sync)

# ==============================================================================
# Epic 4 Tests - Zoning & Building System
# ==============================================================================

# Test executable for ZoneTypes (Ticket 4-001)
add_executable(test_zone_types
    zone/test_zone_types.cpp
)

target_include_directories(test_zone_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ZoneTypes tests do NOT require external libraries - pure data structure tests
add_test(NAME ZoneTypes COMMAND test_zone_types)

# Test executable for BuildingTypes (Ticket 4-002)
add_executable(test_building_types
    building/test_building_types.cpp
)

target_include_directories(test_building_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# BuildingTypes tests do NOT require external libraries - pure data structure tests
add_test(NAME BuildingTypes COMMAND test_building_types)

# Test executable for DebrisComponent (Ticket 4-005)
add_executable(test_debris_component
    building/test_debris_component.cpp
)

target_include_directories(test_debris_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# DebrisComponent tests do NOT require external libraries - pure data structure tests
add_test(NAME DebrisComponent COMMAND test_debris_component)

# Test executable for BuildingGrid (Ticket 4-007)
add_executable(test_building_grid
    building/test_building_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
)

target_include_directories(test_building_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# BuildingGrid tests do NOT require external libraries - pure data structure tests
add_test(NAME BuildingGrid COMMAND test_building_grid)

# Test executable for BuildingComponent (Ticket 4-003)
add_executable(test_building_component
    building/test_building_component.cpp
)

target_include_directories(test_building_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# BuildingComponent tests do NOT require external libraries - pure data structure tests
add_test(NAME BuildingComponent COMMAND test_building_component)

# Test executable for ConstructionComponent (Ticket 4-004)
add_executable(test_construction_component
    building/test_construction_component.cpp
)

target_include_directories(test_construction_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ConstructionComponent tests do NOT require external libraries - pure data structure tests
add_test(NAME ConstructionComponent COMMAND test_construction_component)

# Test executable for ZoneGrid (Ticket 4-006)
add_executable(test_zone_grid
    zone/test_zone_grid.cpp
)

target_include_directories(test_zone_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ZoneGrid tests do NOT require external libraries - pure data structure tests
add_test(NAME ZoneGrid COMMAND test_zone_grid)

# Test executable for ZoneEvents (Ticket 4-009)
add_executable(test_zone_events
    zone/test_zone_events.cpp
)

target_include_directories(test_zone_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ZoneEvents tests do NOT require external libraries - pure data structure tests
add_test(NAME ZoneEvents COMMAND test_zone_events)

# Test executable for BuildingEvents (Ticket 4-010)
add_executable(test_building_events
    building/test_building_events.cpp
)

target_include_directories(test_building_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# BuildingEvents tests do NOT require external libraries - pure data structure tests
add_test(NAME BuildingEvents COMMAND test_building_events)

# Test executable for ForwardDependencyInterfaces (Ticket 4-019)
add_executable(test_forward_interfaces
    building/test_forward_interfaces.cpp
)

target_include_directories(test_forward_interfaces PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Find GTest
find_package(GTest REQUIRED CONFIG)
target_link_libraries(test_forward_interfaces PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# ForwardInterfaces tests do NOT require external libraries - pure interface tests
add_test(NAME ForwardInterfaces COMMAND test_forward_interfaces)

# Test executable for BuildingTemplate (Ticket 4-021)
add_executable(test_building_template
    building/test_building_template.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
)

target_include_directories(test_building_template PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_building_template PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# BuildingTemplate tests do NOT require external libraries - pure data structure tests
add_test(NAME BuildingTemplate COMMAND test_building_template)

# Test executable for TemplateSelector (Ticket 4-022)
add_executable(test_template_selector
    building/test_template_selector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
)

target_include_directories(test_template_selector PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_template_selector PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# TemplateSelector tests do NOT require external libraries - pure algorithm tests
add_test(NAME TemplateSelector COMMAND test_template_selector)

# Test executable for ZoneSerialization (Ticket 4-041)
add_executable(test_zone_serialization
    zone/test_zone_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSerialization.cpp
)

target_include_directories(test_zone_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_zone_serialization PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# ZoneSerialization tests do NOT require external libraries - pure serialization tests
add_test(NAME ZoneSerialization COMMAND test_zone_serialization)

# Test executable for BuildingSerialization (Ticket 4-042)
add_executable(test_building_serialization
    building/test_building_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSerialization.cpp
)

target_include_directories(test_building_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_building_serialization PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# BuildingSerialization tests do NOT require external libraries - pure serialization tests
add_test(NAME BuildingSerialization COMMAND test_building_serialization)

# ==============================================================================
# Epic 4 Wave 3 Tests
# ==============================================================================

# Test executable for ZoneSystem (Ticket 4-008)
add_executable(test_zone_system
    zone/test_zone_system.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)

target_include_directories(test_zone_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_zone_system PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# ZoneSystem tests do NOT require external libraries - pure system logic tests
add_test(NAME ZoneSystem COMMAND test_zone_system)

# Test executable for ForwardDependencyStubs (Ticket 4-020)
add_executable(test_forward_stubs
    building/test_forward_stubs.cpp
)

target_include_directories(test_forward_stubs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_forward_stubs PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# ForwardStubs tests do NOT require external libraries - pure stub tests
add_test(NAME ForwardStubs COMMAND test_forward_stubs)

# Test executable for InitialTemplates (Ticket 4-023)
add_executable(test_initial_templates
    building/test_initial_templates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
)

target_include_directories(test_initial_templates PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_initial_templates PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# InitialTemplates tests do NOT require external libraries - pure data tests
add_test(NAME InitialTemplates COMMAND test_initial_templates)

# Test executable for IBuildingTemplateQuery (Ticket 4-037)
add_executable(test_building_template_query
    building/test_building_template_query.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
)

target_include_directories(test_building_template_query PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_building_template_query PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# IBuildingTemplateQuery tests do NOT require external libraries - pure interface tests
add_test(NAME BuildingTemplateQuery COMMAND test_building_template_query)

# ==============================================================================
# Epic 4 Wave 4 Tests
# ==============================================================================

# Test executable for ZonePlacementValidation (Ticket 4-011)
add_executable(test_zone_placement_validation
    zone/test_zone_placement_validation.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_placement_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_placement_validation PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZonePlacementValidation COMMAND test_zone_placement_validation)

# Test executable for ZoneStateManagement (Ticket 4-015)
add_executable(test_zone_state_management
    zone/test_zone_state_management.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_state_management PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_state_management PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneStateManagement COMMAND test_zone_state_management)

# Test executable for ZoneDemand (Ticket 4-016)
add_executable(test_zone_demand
    zone/test_zone_demand.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_demand PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_demand PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneDemand COMMAND test_zone_demand)

# Test executable for ZoneDesirability (Ticket 4-018)
add_executable(test_zone_desirability
    zone/test_zone_desirability.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_desirability PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_desirability PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneDesirability COMMAND test_zone_desirability)

# Test executable for BuildingSpawnChecker (Ticket 4-024)
add_executable(test_building_spawn_checker
    building/test_building_spawn_checker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_spawn_checker PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_spawn_checker PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingSpawnChecker COMMAND test_building_spawn_checker)

# ==============================================================================
# Epic 4 Wave 5 Tests
# ==============================================================================

# Test executable for ZonePlacementExecution (Ticket 4-012)
add_executable(test_zone_placement_execution
    zone/test_zone_placement_execution.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_placement_execution PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_placement_execution PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZonePlacementExecution COMMAND test_zone_placement_execution)

# Test executable for ZoneDezoning (Ticket 4-013)
add_executable(test_zone_dezoning
    zone/test_zone_dezoning.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_dezoning PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_dezoning PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneDezoning COMMAND test_zone_dezoning)

# Test executable for ZoneDemandProvider (Ticket 4-017)
add_executable(test_zone_demand_provider
    zone/test_zone_demand_provider.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_demand_provider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_demand_provider PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneDemandProvider COMMAND test_zone_demand_provider)

# ==============================================================================
# Epic 4 Wave 6 Tests - Building Systems
# ==============================================================================

# Test executable for BuildingFactory (Ticket 4-025)
add_executable(test_building_factory
    building/test_building_factory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_factory PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_factory PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingFactory COMMAND test_building_factory)

# Test executable for ConstructionProgressSystem (Ticket 4-027)
add_executable(test_construction_progress
    building/test_construction_progress.cpp
    ${CMAKE_SOURCE_DIR}/src/building/ConstructionProgressSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_construction_progress PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_construction_progress PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ConstructionProgress COMMAND test_construction_progress)

# Test executable for DemolitionHandler (Ticket 4-030)
add_executable(test_demolition_handler
    building/test_demolition_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DemolitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_demolition_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_demolition_handler PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME DemolitionHandler COMMAND test_demolition_handler)

# Test executable for DebrisClearSystem (Ticket 4-031)
add_executable(test_debris_clear_system
    building/test_debris_clear_system.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DebrisClearSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_debris_clear_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_debris_clear_system PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME DebrisClearSystem COMMAND test_debris_clear_system)

# ==============================================================================
# Epic 4 Wave 7 Tests - Zone Redesignation, Queryable, Network Messages
# ==============================================================================

# Test executable for ZoneRedesignation (Ticket 4-014)
add_executable(test_zone_redesignation
    zone/test_zone_redesignation.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_redesignation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_redesignation PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneRedesignation COMMAND test_zone_redesignation)

# Test executable for ZoneQueryable (Ticket 4-035)
add_executable(test_zone_queryable
    zone/test_zone_queryable.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_queryable PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_queryable PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneQueryable COMMAND test_zone_queryable)

# Test executable for ZoneNetworkMessages (Ticket 4-038)
add_executable(test_zone_network_messages
    zone/test_zone_network_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneNetworkMessages.cpp
)
target_include_directories(test_zone_network_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_network_messages PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneNetworkMessages COMMAND test_zone_network_messages)

# ==============================================================================
# Epic 4 Wave 8 Tests - Building Spawning Loop & State Transitions
# ==============================================================================

# Test executable for BuildingSpawningLoop (Ticket 4-026)
add_executable(test_building_spawning_loop
    building/test_building_spawning_loop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawningLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_spawning_loop PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_spawning_loop PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingSpawningLoop COMMAND test_building_spawning_loop)

# Test executable for BuildingStateTransitionSystem (Ticket 4-028)
add_executable(test_building_state_transitions
    building/test_building_state_transitions.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_state_transitions PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_state_transitions PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingStateTransitions COMMAND test_building_state_transitions)

# ==============================================================================
# Epic 4 Wave 9 Tests - BuildingSystem Integration
# ==============================================================================

# Test executable for BuildingSystem (Ticket 4-034)
add_executable(test_building_system
    building/test_building_system.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawningLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/ConstructionProgressSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DemolitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DebrisClearSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_system PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingSystem COMMAND test_building_system)

# ==============================================================================
# Epic 4 Wave 8 Tests - Grace Period, Upgrade, Server Handler
# ==============================================================================

# Test executable for AbandonGracePeriod (Ticket 4-029)
add_executable(test_abandon_grace_period
    building/test_abandon_grace_period.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_abandon_grace_period PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_abandon_grace_period PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME AbandonGracePeriod COMMAND test_abandon_grace_period)

# Test executable for BuildingUpgradeSystem (Ticket 4-032)
add_executable(test_building_upgrade
    building/test_building_upgrade.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingUpgradeSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_upgrade PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_upgrade PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingUpgrade COMMAND test_building_upgrade)

# Test executable for ZoneServerHandler (Ticket 4-039)
add_executable(test_zone_server_handler
    zone/test_zone_server_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneServerHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_server_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_server_handler PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneServerHandler COMMAND test_zone_server_handler)

# ==============================================================================
# Epic 4 Wave 10 Tests - Downgrade, Queryable, SimulationPipeline
# ==============================================================================

# Test executable for BuildingDowngradeSystem (Ticket 4-033)
add_executable(test_building_downgrade
    building/test_building_downgrade.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingDowngradeSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingUpgradeSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_downgrade PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_downgrade PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingDowngrade COMMAND test_building_downgrade)

# Test executable for IBuildingQueryable (Ticket 4-036)
add_executable(test_building_queryable
    building/test_building_queryable.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawningLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/ConstructionProgressSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DemolitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DebrisClearSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_queryable PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_queryable PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingQueryable COMMAND test_building_queryable)

# Test executable for SimulationPipeline (Ticket 4-048)
add_executable(test_simulation_pipeline
    sim/test_simulation_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationPipeline.cpp
)
target_include_directories(test_simulation_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_simulation_pipeline PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME SimulationPipeline COMMAND test_simulation_pipeline)

# Test executable for BuildingNetworkMessages (Ticket 4-040)
add_executable(test_building_network_messages
    building/test_building_network_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingNetworkMessages.cpp
)
target_include_directories(test_building_network_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_network_messages PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME BuildingNetworkMessages COMMAND test_building_network_messages)

# Test executable for ZoneBuildingPipeline integration (Ticket 4-047)
add_executable(test_zone_building_pipeline
    building/test_zone_building_pipeline.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawningLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/ConstructionProgressSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DemolitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DebrisClearSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_building_pipeline PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_zone_building_pipeline PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ZoneBuildingPipeline COMMAND test_zone_building_pipeline)

# ==============================================================================
# Epic 5 Tests - Energy Infrastructure
# ==============================================================================

# Test executable for EnergyEnums (Ticket 5-001)
add_executable(test_energy_enums
    energy/test_energy_enums.cpp
)
target_include_directories(test_energy_enums PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyEnums COMMAND test_energy_enums)

# Test executable for EnergyConduitComponent (Ticket 5-004)
add_executable(test_energy_conduit_component
    energy/test_energy_conduit_component.cpp
)
target_include_directories(test_energy_conduit_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyConduitComponent COMMAND test_energy_conduit_component)

# Test executable for CoverageGrid (Ticket 5-007)
add_executable(test_coverage_grid
    energy/test_coverage_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
)
target_include_directories(test_coverage_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CoverageGrid COMMAND test_coverage_grid)

# Test executable for EnergyRequirements and EnergyPriorities (Tickets 5-037, 5-038)
add_executable(test_energy_requirements
    energy/test_energy_requirements.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyRequirements.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyPriorities.cpp
)
target_include_directories(test_energy_requirements PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyRequirements COMMAND test_energy_requirements)

# Test executable for EnergyComponent (Ticket 5-002)
add_executable(test_energy_component
    energy/test_energy_component.cpp
)
target_include_directories(test_energy_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyComponent COMMAND test_energy_component)

# Test executable for EnergyProducerComponent (Ticket 5-003)
add_executable(test_energy_producer_component
    energy/test_energy_producer_component.cpp
)
target_include_directories(test_energy_producer_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyProducerComponent COMMAND test_energy_producer_component)

# Test executable for PerPlayerEnergyPool (Ticket 5-005)
add_executable(test_per_player_energy_pool
    energy/test_per_player_energy_pool.cpp
)
target_include_directories(test_per_player_energy_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PerPlayerEnergyPool COMMAND test_per_player_energy_pool)

# Test executable for EnergyEvents (Ticket 5-006)
add_executable(test_energy_events
    energy/test_energy_events.cpp
)
target_include_directories(test_energy_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergyEvents COMMAND test_energy_events)

# Test executable for NexusTypeConfig (Ticket 5-023)
add_executable(test_nexus_type_config
    energy/test_nexus_type_config.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_nexus_type_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME NexusTypeConfig COMMAND test_nexus_type_config)

# Test executable for EnergySystem (Ticket 5-008, updated for 5-009/5-010/5-014)
add_executable(test_energy_system
    energy/test_energy_system.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_energy_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_energy_system PRIVATE
    EnTT::EnTT
)
add_test(NAME EnergySystem COMMAND test_energy_system)

# Test executable for EnergySerialization (Tickets 5-034, 5-035)
add_executable(test_energy_serialization
    energy/test_energy_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyPriorities.cpp
)
target_include_directories(test_energy_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EnergySerialization COMMAND test_energy_serialization)

# Test executable for EnergyProvider (Ticket 5-009)
add_executable(test_energy_provider
    energy/test_energy_provider.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_energy_provider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_energy_provider PRIVATE
    EnTT::EnTT
)
add_test(NAME EnergyProvider COMMAND test_energy_provider)

# Test executable for NexusOutput (Ticket 5-010)
add_executable(test_nexus_output
    energy/test_nexus_output.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_nexus_output PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_nexus_output PRIVATE
    EnTT::EnTT
)
add_test(NAME NexusOutput COMMAND test_nexus_output)

# Test executable for CoverageBFS (Ticket 5-014)
add_executable(test_coverage_bfs
    energy/test_coverage_bfs.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_coverage_bfs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_coverage_bfs PRIVATE
    EnTT::EnTT
)
add_test(NAME CoverageBFS COMMAND test_coverage_bfs)

# Test executable for CoverageDirty (Tickets 5-015, 5-016)
add_executable(test_coverage_dirty
    energy/test_coverage_dirty.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_coverage_dirty PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_coverage_dirty PRIVATE
    EnTT::EnTT
)
add_test(NAME CoverageDirty COMMAND test_coverage_dirty)

# Test executable for CoverageQueries (Ticket 5-017)
add_executable(test_coverage_queries
    energy/test_coverage_queries.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_coverage_queries PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_coverage_queries PRIVATE
    EnTT::EnTT
)
add_test(NAME CoverageQueries COMMAND test_coverage_queries)

# Test executable for ConduitConnection (Ticket 5-028)
add_executable(test_conduit_connection
    energy/test_conduit_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_conduit_connection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_conduit_connection PRIVATE
    EnTT::EnTT
)
add_test(NAME ConduitConnection COMMAND test_conduit_connection)

# Test executable for ConsumerAggregation (Ticket 5-011)
add_executable(test_consumer_aggregation
    energy/test_consumer_aggregation.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_consumer_aggregation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_consumer_aggregation PRIVATE
    EnTT::EnTT
)
add_test(NAME ConsumerAggregation COMMAND test_consumer_aggregation)

# Test executable for NexusPlacement (Ticket 5-026)
add_executable(test_nexus_placement
    energy/test_nexus_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_nexus_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_nexus_placement PRIVATE
    EnTT::EnTT
)
add_test(NAME NexusPlacement COMMAND test_nexus_placement)

# Test executable for ConduitPlacement (Ticket 5-027)
add_executable(test_conduit_placement
    energy/test_conduit_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_conduit_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_conduit_placement PRIVATE
    EnTT::EnTT
)
add_test(NAME ConduitPlacement COMMAND test_conduit_placement)

# Test executable for PoolCalculation (Ticket 5-012)
add_executable(test_pool_calculation
    energy/test_pool_calculation.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_pool_calculation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_pool_calculation PRIVATE
    EnTT::EnTT
)
add_test(NAME PoolCalculation COMMAND test_pool_calculation)

# Test executable for NexusAging (Ticket 5-022)
add_executable(test_nexus_aging
    energy/test_nexus_aging.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_nexus_aging PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_nexus_aging PRIVATE
    EnTT::EnTT
)
add_test(NAME NexusAging COMMAND test_nexus_aging)

# Test executable for ConduitRemoval (Ticket 5-029)
add_executable(test_conduit_removal
    energy/test_conduit_removal.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_conduit_removal PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_conduit_removal PRIVATE
    EnTT::EnTT
)
add_test(NAME ConduitRemoval COMMAND test_conduit_removal)

# Test executable for PoolStateMachine (Ticket 5-013)
add_executable(test_pool_state_machine
    energy/test_pool_state_machine.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_pool_state_machine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_pool_state_machine PRIVATE
    EnTT::EnTT
)
add_test(NAME PoolStateMachine COMMAND test_pool_state_machine)

# Test executable for EnergyDistribution (Ticket 5-018)
add_executable(test_energy_distribution
    energy/test_energy_distribution.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_energy_distribution PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_energy_distribution PRIVATE
    EnTT::EnTT
)
add_test(NAME EnergyDistribution COMMAND test_energy_distribution)

# Test executable for ConduitActiveState (Ticket 5-030)
add_executable(test_conduit_active_state
    energy/test_conduit_active_state.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_conduit_active_state PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_conduit_active_state PRIVATE
    EnTT::EnTT
)
add_test(NAME ConduitActiveState COMMAND test_conduit_active_state)

# Test executable for BuildingEventHandler (Ticket 5-032)
add_executable(test_building_event_handler
    energy/test_building_event_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_building_event_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_event_handler PRIVATE
    EnTT::EnTT
)
add_test(NAME BuildingEventHandler COMMAND test_building_event_handler)

# Test executable for Rationing (Ticket 5-019)
add_executable(test_rationing
    energy/test_rationing.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_rationing PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_rationing PRIVATE
    EnTT::EnTT
)
add_test(NAME Rationing COMMAND test_rationing)

# Test executable for StateChangeEvents (Ticket 5-020)
add_executable(test_state_change_events
    energy/test_state_change_events.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_state_change_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_state_change_events PRIVATE
    EnTT::EnTT
)
add_test(NAME StateChangeEvents COMMAND test_state_change_events)

# Test executable for PoolTransitionEvents (Ticket 5-021)
add_executable(test_pool_transition_events
    energy/test_pool_transition_events.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_pool_transition_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_pool_transition_events PRIVATE
    EnTT::EnTT
)
add_test(NAME PoolTransitionEvents COMMAND test_pool_transition_events)

# Test executable for BuildingDeconHandler (Ticket 5-033)
add_executable(test_building_decon_handler
    energy/test_building_decon_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_building_decon_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_decon_handler PRIVATE
    EnTT::EnTT
)
add_test(NAME BuildingDeconHandler COMMAND test_building_decon_handler)

# Test executable for TerrainBonus (Ticket 5-024)
add_executable(test_terrain_bonus
    energy/test_terrain_bonus.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_terrain_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_terrain_bonus PRIVATE
    EnTT::EnTT
)
add_test(NAME TerrainBonus COMMAND test_terrain_bonus)

# Test executable for ContaminationSource (Ticket 5-025)
add_executable(test_contamination_source
    energy/test_contamination_source.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_contamination_source PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_contamination_source PRIVATE
    EnTT::EnTT
)
add_test(NAME ContaminationSource COMMAND test_contamination_source)

# Test executable for ConduitPreview (Ticket 5-031)
add_executable(test_conduit_preview
    energy/test_conduit_preview.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_conduit_preview PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_conduit_preview PRIVATE
    EnTT::EnTT
)
add_test(NAME ConduitPreview COMMAND test_conduit_preview)

# Test executable for BuildingSystem Integration (Ticket 5-036)
add_executable(test_building_integration
    energy/test_building_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingFactory.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawnChecker.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingSpawningLoop.cpp
    ${CMAKE_SOURCE_DIR}/src/building/ConstructionProgressSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingStateTransitionSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DemolitionHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/building/DebrisClearSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/building/TemplateSelector.cpp
    ${CMAKE_SOURCE_DIR}/src/building/InitialTemplates.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingTemplateRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/building/BuildingGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_building_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_building_integration PRIVATE
    EnTT::EnTT
)
add_test(NAME BuildingIntegration COMMAND test_building_integration)

# Test executable for ThresholdBalance (Ticket 5-039)
add_executable(test_threshold_balance
    energy/test_threshold_balance.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
)
target_include_directories(test_threshold_balance PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_threshold_balance PRIVATE
    EnTT::EnTT
)
add_test(NAME ThresholdBalance COMMAND test_threshold_balance)

# Test executable for MultiplayerSync (Ticket 5-042)
add_executable(test_multiplayer_sync
    energy/test_multiplayer_sync.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyPriorities.cpp
)
target_include_directories(test_multiplayer_sync PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_multiplayer_sync PRIVATE
    EnTT::EnTT
)
add_test(NAME MultiplayerSync COMMAND test_multiplayer_sync)

# Test executable for EnergyIntegration (Ticket 5-041)
add_executable(test_energy_integration
    energy/test_energy_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyPriorities.cpp
)
target_include_directories(test_energy_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_energy_integration PRIVATE
    EnTT::EnTT
)
add_test(NAME EnergyIntegration COMMAND test_energy_integration)

# Test executable for EnergyPerformance (Ticket 5-043)
add_executable(test_energy_performance
    energy/test_energy_performance.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergySystem.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/CoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/NexusTypeConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyPriorities.cpp
)
target_include_directories(test_energy_performance PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_energy_performance PRIVATE
    EnTT::EnTT
)
add_test(NAME EnergyPerformance COMMAND test_energy_performance)

# Test executable for FluidCoverageGrid (Ticket 6-008)
add_executable(test_fluid_coverage_grid
    fluid/test_fluid_coverage_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
)
target_include_directories(test_fluid_coverage_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidCoverageGrid COMMAND test_fluid_coverage_grid)

# Test executable for FluidProducerComponent (Ticket 6-003)
add_executable(test_fluid_producer_component
    fluid/test_fluid_producer_component.cpp
)
target_include_directories(test_fluid_producer_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidProducerComponent COMMAND test_fluid_producer_component)

# Test executable for PerPlayerFluidPool (Ticket 6-006)
add_executable(test_per_player_fluid_pool
    fluid/test_per_player_fluid_pool.cpp
)
target_include_directories(test_per_player_fluid_pool PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PerPlayerFluidPool COMMAND test_per_player_fluid_pool)

# Test executable for FluidEvents (Ticket 6-007)
add_executable(test_fluid_events
    fluid/test_fluid_events.cpp
)
target_include_directories(test_fluid_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidEvents COMMAND test_fluid_events)

# Test executable for FluidExtractorConfig (Ticket 6-023)
add_executable(test_fluid_extractor_config
    fluid/test_fluid_extractor_config.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
)
target_include_directories(test_fluid_extractor_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidExtractorConfig COMMAND test_fluid_extractor_config)

# Test executable for FluidReservoirConfig (Ticket 6-024)
add_executable(test_fluid_reservoir_config
    fluid/test_fluid_reservoir_config.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
)
target_include_directories(test_fluid_reservoir_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidReservoirConfig COMMAND test_fluid_reservoir_config)

# Test executable for FluidSerialization (Tickets 6-036, 6-037)
add_executable(test_fluid_serialization
    fluid/test_fluid_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSerialization.cpp
)
target_include_directories(test_fluid_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidSerialization COMMAND test_fluid_serialization)

# Test executable for FluidCoverageBFS (Ticket 6-010)
add_executable(test_fluid_coverage_bfs
    fluid/test_fluid_coverage_bfs.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
)
target_include_directories(test_fluid_coverage_bfs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_coverage_bfs PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidCoverageBFS COMMAND test_fluid_coverage_bfs)

# Test executable for FluidPlacementValidation (Tickets 6-027, 6-028)
add_executable(test_fluid_placement_validation
    fluid/test_fluid_placement_validation.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
)
target_include_directories(test_fluid_placement_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_placement_validation PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidPlacementValidation COMMAND test_fluid_placement_validation)

# Test executable for FluidSystem (Ticket 6-009)
add_executable(test_fluid_system
    fluid/test_fluid_system.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_system PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidSystem COMMAND test_fluid_system)

# Test executable for Extractor/Reservoir Registration (Tickets 6-014, 6-015)
add_executable(test_extractor_registration
    fluid/test_extractor_registration.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_extractor_registration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_extractor_registration PRIVATE
    EnTT::EnTT
)
add_test(NAME ExtractorRegistration COMMAND test_extractor_registration)

# Test executable for FluidCoverageQueries (Tickets 6-013, 6-011)
add_executable(test_fluid_coverage_queries
    fluid/test_coverage_queries.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_coverage_queries PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_coverage_queries PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidCoverageQueries COMMAND test_fluid_coverage_queries)

# Test executable for FluidCoverageDirty (Ticket 6-011)
add_executable(test_fluid_coverage_dirty
    fluid/test_coverage_dirty.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_coverage_dirty PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_coverage_dirty PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidCoverageDirty COMMAND test_fluid_coverage_dirty)

# Test executable for FluidProvider integration (Ticket 6-038)
add_executable(test_fluid_provider
    fluid/test_fluid_provider.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_provider PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_provider PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidProvider COMMAND test_fluid_provider)

# Test executable for Fluid Consumer Registration & Aggregation (Ticket 6-016)
add_executable(test_fluid_consumer_aggregation
    fluid/test_consumer_aggregation.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_consumer_aggregation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_consumer_aggregation PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConsumerAggregation COMMAND test_fluid_consumer_aggregation)

# Test executable for Water Proximity & Power Dependency (Tickets 6-025, 6-026)
add_executable(test_water_proximity
    fluid/test_water_proximity.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_water_proximity PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_water_proximity PRIVATE
    EnTT::EnTT
)
add_test(NAME WaterProximity COMMAND test_water_proximity)

# Test executable for Fluid Conduit Placement (Ticket 6-029)
add_executable(test_fluid_conduit_placement
    fluid/test_conduit_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_conduit_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_conduit_placement PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConduitPlacement COMMAND test_fluid_conduit_placement)

# Test executable for Fluid Conduit Connection Detection (Ticket 6-030)
add_executable(test_fluid_conduit_connection
    fluid/test_conduit_connection.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
)
target_include_directories(test_fluid_conduit_connection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_conduit_connection PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConduitConnection COMMAND test_fluid_conduit_connection)

# Test executable for Fluid Conduit Preview (Ticket 6-033)
add_executable(test_fluid_conduit_preview
    fluid/test_conduit_preview.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_conduit_preview PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_conduit_preview PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConduitPreview COMMAND test_fluid_conduit_preview)

# Test executable for Fluid Pool Calculation (Ticket 6-017)
add_executable(test_fluid_pool_calculation
    fluid/test_pool_calculation.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_pool_calculation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_pool_calculation PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidPoolCalculation COMMAND test_fluid_pool_calculation)

# Test executable for Fluid Conduit Removal (Ticket 6-031)
add_executable(test_fluid_conduit_removal
    fluid/test_conduit_removal.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_conduit_removal PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_conduit_removal PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConduitRemoval COMMAND test_fluid_conduit_removal)

# Test executable for Fluid BuildingConstructedEvent Handler (Ticket 6-034)
add_executable(test_fluid_building_event_handler
    fluid/test_building_event_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_building_event_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_building_event_handler PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidBuildingEventHandler COMMAND test_fluid_building_event_handler)

# Test executable for Fluid BuildingDeconstructedEvent Handler (Ticket 6-035)
add_executable(test_fluid_building_decon_handler
    fluid/test_building_decon_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_building_decon_handler PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_building_decon_handler PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidBuildingDeconHandler COMMAND test_fluid_building_decon_handler)

# Test executable for Fluid Pool State Machine and Reservoir Buffering (Ticket 6-018)
add_executable(test_fluid_pool_state_machine
    fluid/test_pool_state_machine.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_pool_state_machine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_pool_state_machine PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidPoolStateMachine COMMAND test_fluid_pool_state_machine)

# Test executable for Fluid Conduit Active State (Ticket 6-032)
add_executable(test_fluid_conduit_active_state
    fluid/test_conduit_active_state.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_conduit_active_state PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_conduit_active_state PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidConduitActiveState COMMAND test_fluid_conduit_active_state)

# Test executable for Fluid Distribution (Ticket 6-019)
add_executable(test_fluid_distribution
    fluid/test_fluid_distribution.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_distribution PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_distribution PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidDistribution COMMAND test_fluid_distribution)

# Test executable for Fluid Pool Transition Events (Ticket 6-022)
add_executable(test_fluid_pool_transition_events
    fluid/test_pool_transition_events.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_pool_transition_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_pool_transition_events PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidPoolTransitionEvents COMMAND test_fluid_pool_transition_events)

# Test executable for Fluid State Change Events (Ticket 6-021)
add_executable(test_fluid_state_change_events
    fluid/test_state_change_events.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_state_change_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_state_change_events PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidStateChangeEvents COMMAND test_fluid_state_change_events)

# Test executable for FluidSystem Integration (Ticket 6-043)
add_executable(test_fluid_integration
    fluid/test_fluid_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
)
target_include_directories(test_fluid_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_integration PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidIntegration COMMAND test_fluid_integration)

# Test executable for Fluid Multiplayer Sync (Ticket 6-044)
add_executable(test_fluid_multiplayer_sync
    fluid/test_multiplayer_sync.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidExtractorConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidReservoirConfig.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidCoverageBFS.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidPlacementValidation.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidSerialization.cpp
)
target_include_directories(test_fluid_multiplayer_sync PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_fluid_multiplayer_sync PRIVATE
    EnTT::EnTT
)
add_test(NAME FluidMultiplayerSync COMMAND test_fluid_multiplayer_sync)

# Test executable for FluidEnums (Ticket 6-001)
add_executable(test_fluid_enums
    fluid/test_fluid_enums.cpp
)
target_include_directories(test_fluid_enums PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidEnums COMMAND test_fluid_enums)

# Test executable for FluidComponent (Ticket 6-002)
add_executable(test_fluid_component
    fluid/test_fluid_component.cpp
)
target_include_directories(test_fluid_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidComponent COMMAND test_fluid_component)

# Test executable for FluidReservoirComponent (Ticket 6-004)
add_executable(test_fluid_reservoir_component
    fluid/test_fluid_reservoir_component.cpp
)
target_include_directories(test_fluid_reservoir_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidReservoirComponent COMMAND test_fluid_reservoir_component)

# Test executable for FluidConduitComponent (Ticket 6-005)
add_executable(test_fluid_conduit_component
    fluid/test_fluid_conduit_component.cpp
)
target_include_directories(test_fluid_conduit_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidConduitComponent COMMAND test_fluid_conduit_component)

# Test executable for FluidRequirements (Ticket 6-039)
add_executable(test_fluid_requirements
    fluid/test_fluid_requirements.cpp
    ${CMAKE_SOURCE_DIR}/src/fluid/FluidRequirements.cpp
)
target_include_directories(test_fluid_requirements PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FluidRequirements COMMAND test_fluid_requirements)

# ===========================================================================
# Epic 7: Transportation Network Tests
# ===========================================================================

# Test executable for TransportEnums (Ticket E7-001)
add_executable(test_transport_enums
    transport/test_transport_enums.cpp
)
target_include_directories(test_transport_enums PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportEnums COMMAND test_transport_enums)

# Test executable for TrafficComponent (Ticket E7-003)
add_executable(test_traffic_component
    transport/test_traffic_component.cpp
)
target_include_directories(test_traffic_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TrafficComponent COMMAND test_traffic_component)

# Test executable for TransportEvents (Ticket E7-004)
add_executable(test_transport_events
    transport/test_transport_events.cpp
)
target_include_directories(test_transport_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportEvents COMMAND test_transport_events)

# Test executable for RailComponent (Ticket E7-030)
add_executable(test_rail_component
    transport/test_rail_component.cpp
)
target_include_directories(test_rail_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailComponent COMMAND test_rail_component)

# Test executable for TerminalComponent (Ticket E7-031)
add_executable(test_terminal_component
    transport/test_terminal_component.cpp
)
target_include_directories(test_terminal_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TerminalComponent COMMAND test_terminal_component)

# Test executable for SubterraComponent (Ticket E7-043)
add_executable(test_subterra_component
    transport/test_subterra_component.cpp
)
target_include_directories(test_subterra_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME SubterraComponent COMMAND test_subterra_component)

# Test executable for NetworkGraph (Ticket E7-008)
add_executable(test_network_graph
    transport/test_network_graph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_network_graph PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME NetworkGraph COMMAND test_network_graph)

# Test executable for ITransportProvider Interface (Ticket E7-016)
add_executable(test_transport_provider_interface
    transport/test_transport_provider_interface.cpp
)
target_include_directories(test_transport_provider_interface PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportProviderInterface COMMAND test_transport_provider_interface)

# Test executable for RoadComponent (Ticket E7-002)
add_executable(test_road_component
    transport/test_road_component.cpp
)
target_include_directories(test_road_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RoadComponent COMMAND test_road_component)

# Test executable for TrafficContribution (Ticket E7-012)
add_executable(test_traffic_contribution
    transport/test_traffic_contribution.cpp
)
target_include_directories(test_traffic_contribution PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TrafficContribution COMMAND test_traffic_contribution)

# Test executable for PathwayTypeConfig (Ticket E7-040)
add_executable(test_pathway_type_config
    transport/test_pathway_type_config.cpp
)
target_include_directories(test_pathway_type_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PathwayTypeConfig COMMAND test_pathway_type_config)

# Test executable for DecayConfig (Ticket E7-046)
add_executable(test_decay_config
    transport/test_decay_config.cpp
)
target_include_directories(test_decay_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME DecayConfig COMMAND test_decay_config)

# Test executable for RailConfig (Ticket E7-047)
add_executable(test_rail_config
    transport/test_rail_config.cpp
)
target_include_directories(test_rail_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailConfig COMMAND test_rail_config)

# Test executable for TrafficBalanceConfig (Ticket E7-048)
add_executable(test_traffic_balance_config
    transport/test_traffic_balance_config.cpp
)
target_include_directories(test_traffic_balance_config PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TrafficBalanceConfig COMMAND test_traffic_balance_config)

# Test executable for PathwayGrid (Ticket E7-005)
add_executable(test_pathway_grid
    transport/test_pathway_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_pathway_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PathwayGrid COMMAND test_pathway_grid)

# Test executable for TransportSerialization (Tickets E7-036, E7-037)
add_executable(test_transport_serialization
    transport/test_transport_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TransportSerialization.cpp
)
target_include_directories(test_transport_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportSerialization COMMAND test_transport_serialization)

# Test executable for PathwayDecay (Ticket E7-025)
add_executable(test_pathway_decay
    transport/test_pathway_decay.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayDecay.cpp
)
target_include_directories(test_pathway_decay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PathwayDecay COMMAND test_pathway_decay)

# Test executable for MaintenanceCost (Ticket E7-021)
add_executable(test_maintenance_cost
    transport/test_maintenance_cost.cpp
)
target_include_directories(test_maintenance_cost PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME MaintenanceCost COMMAND test_maintenance_cost)

# Test executable for RailSystem (Ticket E7-032)
add_executable(test_rail_system
    transport/test_rail_system.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_rail_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailSystem COMMAND test_rail_system)

# Test executable for CapacityDegradation (Ticket E7-026)
add_executable(test_capacity_degradation
    transport/test_capacity_degradation.cpp
)
target_include_directories(test_capacity_degradation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CapacityDegradation COMMAND test_capacity_degradation)

# Test executable for MaintenanceAPI (Ticket E7-027)
add_executable(test_maintenance_api
    transport/test_maintenance_api.cpp
)
target_include_directories(test_maintenance_api PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME MaintenanceAPI COMMAND test_maintenance_api)

# Test executable for SubterraLayerManager (Ticket E7-042)
add_executable(test_subterra_layer
    transport/test_subterra_layer.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/SubterraLayerManager.cpp
)
target_include_directories(test_subterra_layer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME SubterraLayer COMMAND test_subterra_layer)

# Test executable for ProximityCache (Ticket E7-006)
add_executable(test_proximity_cache
    transport/test_proximity_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_proximity_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ProximityCache COMMAND test_proximity_cache)

# Test executable for NetworkGraph rebuild_from_grid (Ticket E7-009)
add_executable(test_graph_rebuild
    transport/test_graph_rebuild.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_graph_rebuild PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME GraphRebuild COMMAND test_graph_rebuild)

# Test executable for RailPower (Ticket E7-033)
add_executable(test_rail_power
    transport/test_rail_power.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_rail_power PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailPower COMMAND test_rail_power)

# Test executable for TerminalPlacement (Ticket E7-034)
add_executable(test_terminal_placement
    transport/test_terminal_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_terminal_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TerminalPlacement COMMAND test_terminal_placement)

# Test executable for ProximityCache BFS (Ticket E7-007)
add_executable(test_proximity_bfs
    transport/test_proximity_bfs.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_proximity_bfs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ProximityBFS COMMAND test_proximity_bfs)

# Test executable for Connected Components (Ticket E7-010)
add_executable(test_connected_components
    transport/test_connected_components.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_connected_components PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ConnectedComponents COMMAND test_connected_components)

# Test executable for Cross-Ownership Connectivity (Ticket E7-020)
add_executable(test_cross_ownership
    transport/test_cross_ownership.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_cross_ownership PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CrossOwnership COMMAND test_cross_ownership)

# Test executable for FlowDistribution (Ticket E7-013)
add_executable(test_flow_distribution
    transport/test_flow_distribution.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/FlowDistribution.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
)
target_include_directories(test_flow_distribution PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FlowDistribution COMMAND test_flow_distribution)

# Test executable for RailCoverage (Ticket E7-035)
add_executable(test_rail_coverage
    transport/test_rail_coverage.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_rail_coverage PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailCoverage COMMAND test_rail_coverage)

# Test executable for SubterraPlacement (Ticket E7-044)
add_executable(test_subterra_placement
    transport/test_subterra_placement.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/SubterraLayerManager.cpp
)
target_include_directories(test_subterra_placement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME SubterraPlacement COMMAND test_subterra_placement)

# Test executable for ConnectivityQuery (Ticket E7-011)
add_executable(test_connectivity_query
    transport/test_connectivity_query.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ConnectivityQuery.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_connectivity_query PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ConnectivityQuery COMMAND test_connectivity_query)

# Test executable for TransportProviderImpl (Tickets E7-017/E7-018)
add_executable(test_transport_provider_impl
    transport/test_transport_provider_impl.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TransportProviderImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ConnectivityQuery.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
)
target_include_directories(test_transport_provider_impl PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportProviderImpl COMMAND test_transport_provider_impl)

# Test executable for FlowPropagation (Ticket E7-014)
add_executable(test_flow_propagation
    transport/test_flow_propagation.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/FlowPropagation.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_flow_propagation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FlowPropagation COMMAND test_flow_propagation)

# Test executable for Pathfinding (Ticket E7-023)
add_executable(test_pathfinding
    transport/test_pathfinding.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/Pathfinding.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_pathfinding PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME Pathfinding COMMAND test_pathfinding)

# Test executable for BoundaryFlags (Ticket E7-028)
add_executable(test_boundary_flags
    transport/test_boundary_flags.cpp
)
target_include_directories(test_boundary_flags PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME BoundaryFlags COMMAND test_boundary_flags)

# Test executable for CongestionCalculator (Ticket E7-015)
add_executable(test_congestion_calculator
    transport/test_congestion_calculator.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/CongestionCalculator.cpp
)
target_include_directories(test_congestion_calculator PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CongestionCalculator COMMAND test_congestion_calculator)

# Test executable for EdgeCost (Ticket E7-024)
add_executable(test_edge_cost
    transport/test_edge_cost.cpp
)
target_include_directories(test_edge_cost PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EdgeCost COMMAND test_edge_cost)

# Test executable for ContaminationQuery (Ticket E7-029)
add_executable(test_contamination_query
    transport/test_contamination_query.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ContaminationQuery.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_contamination_query PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ContaminationQuery COMMAND test_contamination_query)

# Test executable for StubReplacement (Ticket E7-019)
add_executable(test_stub_replacement
    transport/test_stub_replacement.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TransportProviderImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ConnectivityQuery.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
)
target_include_directories(test_stub_replacement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME StubReplacement COMMAND test_stub_replacement)

# Test executable for RailTrafficReduction (Ticket E7-045)
add_executable(test_rail_traffic_reduction
    transport/test_rail_traffic_reduction.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_rail_traffic_reduction PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailTrafficReduction COMMAND test_rail_traffic_reduction)

# Test executable for TransportSystem (Ticket E7-022)
add_executable(test_transport_system
    transport/test_transport_system.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TransportSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ProximityCache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TransportProviderImpl.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/ConnectivityQuery.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/FlowDistribution.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/FlowPropagation.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/CongestionCalculator.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayDecay.cpp
)
target_include_directories(test_transport_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TransportSystem COMMAND test_transport_system)

# Test executable for PathCache (Ticket E7-041)
add_executable(test_path_cache
    transport/test_path_cache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathCache.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/Pathfinding.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/NetworkGraph.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
)
target_include_directories(test_path_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PathCache COMMAND test_path_cache)

# Test executable for PathwayNetworkMessages (Ticket E7-038)
add_executable(test_pathway_network_messages
    transport/test_pathway_network_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayNetworkMessages.cpp
)
target_include_directories(test_pathway_network_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PathwayNetworkMessages COMMAND test_pathway_network_messages)

# Test executable for RailNetworkMessages (Ticket E7-039)
add_executable(test_rail_network_messages
    transport/test_rail_network_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailNetworkMessages.cpp
)
target_include_directories(test_rail_network_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME RailNetworkMessages COMMAND test_rail_network_messages)

# Test executable for PortEvents (Ticket E8-028)
add_executable(test_port_events
    port/test_port_events.cpp
)
target_include_directories(test_port_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortEvents COMMAND test_port_events)

# Test executable for TradeEvents (Ticket E8-029)
add_executable(test_trade_events
    port/test_trade_events.cpp
)
target_include_directories(test_trade_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeEvents COMMAND test_trade_events)

# Test executable for PortTypes enums (Tickets E8-001, E8-004, E8-005)
add_executable(test_port_types
    port/test_port_types.cpp
)
target_include_directories(test_port_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortTypes COMMAND test_port_types)

# Test executable for ExternalConnectionComponent (Ticket E8-004)
add_executable(test_external_connection
    port/test_external_connection.cpp
)
target_include_directories(test_external_connection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ExternalConnection COMMAND test_external_connection)

# Test executable for TradeAgreementComponent (Ticket E8-005)
add_executable(test_trade_agreement
    port/test_trade_agreement.cpp
    ${CMAKE_SOURCE_DIR}/src/ecs/Components.cpp
    ${CMAKE_SOURCE_DIR}/src/net/NetworkBuffer.cpp
)
target_include_directories(test_trade_agreement PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_trade_agreement PRIVATE
    glm::glm
)
add_test(NAME TradeAgreement COMMAND test_trade_agreement)

# Test executable for PortComponent (Ticket E8-002)
add_executable(test_port_component
    port/test_port_component.cpp
)
target_include_directories(test_port_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortComponent COMMAND test_port_component)

# Test executable for PortZoneComponent (Ticket E8-003)
add_executable(test_port_zone_component
    port/test_port_zone_component.cpp
)
target_include_directories(test_port_zone_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortZoneComponent COMMAND test_port_zone_component)

# Test executable for PortSerialization (Ticket E8-002)
add_executable(test_port_serialization
    port/test_port_serialization.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSerialization.cpp
)
target_include_directories(test_port_serialization PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortSerialization COMMAND test_port_serialization)

# Test executable for Zone Type Extension for Ports (Ticket E8-031)
add_executable(test_zone_type_extension
    port/test_zone_type_extension.cpp
    ${CMAKE_SOURCE_DIR}/src/zone/ZoneSystem.cpp
)
target_include_directories(test_zone_type_extension PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ZoneTypeExtension COMMAND test_zone_type_extension)

# Test executable for Aero Port Zone Validation (Ticket E8-008)
add_executable(test_aero_port_validation
    port/test_aero_port_validation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortZoneValidation.cpp
)
target_include_directories(test_aero_port_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME AeroPortValidation COMMAND test_aero_port_validation)

# Test executable for Aqua Port Zone Validation (Ticket E8-009)
add_executable(test_aqua_port_validation
    port/test_aqua_port_validation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortZoneValidation.cpp
)
target_include_directories(test_aqua_port_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME AquaPortValidation COMMAND test_aqua_port_validation)

# Test executable for PortSystem (Tickets E8-006, E8-007)
add_executable(test_port_system
    port/test_port_system.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortSystem COMMAND test_port_system)

# Test executable for MapEdgeDetection (E8-013)
add_executable(test_map_edge_detection
    port/test_map_edge_detection.cpp
    ${CMAKE_SOURCE_DIR}/src/port/MapEdgeDetection.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/PathwayGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/RailSystem.cpp
)
target_include_directories(test_map_edge_detection PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME MapEdgeDetection COMMAND test_map_edge_detection)

# Test executable for Port Capacity Calculation (Ticket E8-010)
add_executable(test_port_capacity
    port/test_port_capacity.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortCapacity.cpp
)
target_include_directories(test_port_capacity PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortCapacity COMMAND test_port_capacity)

# Test executable for Port Operational Status (Ticket E8-011)
add_executable(test_port_operational
    port/test_port_operational.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortOperational.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortZoneValidation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortCapacity.cpp
)
target_include_directories(test_port_operational PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortOperational COMMAND test_port_operational)

# Test executable for Port Development Levels (Ticket E8-012)
add_executable(test_port_development
    port/test_port_development.cpp
)
target_include_directories(test_port_development PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortDevelopment COMMAND test_port_development)

# Test executable for External Connection Capacity (Ticket E8-014)
add_executable(test_connection_capacity
    port/test_connection_capacity.cpp
)
target_include_directories(test_connection_capacity PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ConnectionCapacity COMMAND test_connection_capacity)

# Test executable for NPC Neighbor Generation (Ticket E8-015)
add_executable(test_neighbor_generation
    port/test_neighbor_generation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/NeighborGeneration.cpp
)
target_include_directories(test_neighbor_generation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME NeighborGeneration COMMAND test_neighbor_generation)

# Test executable for Global Demand Bonus (Ticket E8-016)
add_executable(test_demand_bonus
    port/test_demand_bonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
)
target_include_directories(test_demand_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME DemandBonus COMMAND test_demand_bonus)

# Test executable for Local Demand Bonus (Ticket E8-017)
add_executable(test_local_demand_bonus
    port/test_local_demand_bonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
)
target_include_directories(test_local_demand_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME LocalDemandBonus COMMAND test_local_demand_bonus)

# Test executable for Trade Deal Negotiation (Ticket E8-022)
add_executable(test_trade_deal_negotiation
    port/test_trade_deal_negotiation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeDealNegotiation.cpp
)
target_include_directories(test_trade_deal_negotiation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_trade_deal_negotiation PRIVATE
    glm::glm
)
add_test(NAME TradeDealNegotiation COMMAND test_trade_deal_negotiation)

# Test executable for Trade Offer Manager (Ticket E8-025)
add_executable(test_trade_offer_manager
    port/test_trade_offer_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeOfferManager.cpp
)
target_include_directories(test_trade_offer_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeOfferManager COMMAND test_trade_offer_manager)

# Test executable for Migration Effects (Ticket E8-024)
add_executable(test_migration_effects
    port/test_migration_effects.cpp
    ${CMAKE_SOURCE_DIR}/src/port/MigrationEffects.cpp
)
target_include_directories(test_migration_effects PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME MigrationEffects COMMAND test_migration_effects)

# Test executable for Port Demand Integration (Ticket E8-018)
add_executable(test_port_demand_integration
    port/test_port_demand_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_demand_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortDemandIntegration COMMAND test_port_demand_integration)

# Test executable for Trade Income (Ticket E8-019)
add_executable(test_trade_income
    port/test_trade_income.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
)
target_include_directories(test_trade_income PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeIncome COMMAND test_trade_income)

# Test executable for Trade Deal Expiration (Ticket E8-023)
add_executable(test_trade_deal_expiration
    port/test_trade_deal_expiration.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeDealExpiration.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeDealNegotiation.cpp
)
target_include_directories(test_trade_deal_expiration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(test_trade_deal_expiration PRIVATE
    glm::glm
)
add_test(NAME TradeDealExpiration COMMAND test_trade_deal_expiration)

# Test executable for Trade Network Messages (Ticket E8-026)
add_executable(test_trade_network_messages
    port/test_trade_network_messages.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeNetworkMessages.cpp
)
target_include_directories(test_trade_network_messages PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeNetworkMessages COMMAND test_trade_network_messages)

# Test executable for Trade Agreement Benefits (Ticket E8-027)
add_executable(test_trade_agreement_benefits
    port/test_trade_agreement_benefits.cpp
)
target_include_directories(test_trade_agreement_benefits PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeAgreementBenefits COMMAND test_trade_agreement_benefits)

# Test executable for Port Infrastructure Upgrades (Ticket E8-032)
add_executable(test_port_upgrades
    port/test_port_upgrades.cpp
)
target_include_directories(test_port_upgrades PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortUpgrades COMMAND test_port_upgrades)

# Test executable for Port Noise/Contamination (Ticket E8-033)
add_executable(test_port_contamination
    port/test_port_contamination.cpp
)
target_include_directories(test_port_contamination PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortContamination COMMAND test_port_contamination)

# Test executable for Multiple Port Diminishing Returns (Ticket E8-035)
add_executable(test_diminishing_returns
    port/test_diminishing_returns.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
)
target_include_directories(test_diminishing_returns PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME DiminishingReturns COMMAND test_diminishing_returns)

# Test executable for PortEconomyIntegration (Ticket E8-020)
add_executable(test_port_economy_integration
    port/test_port_economy_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_economy_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortEconomyIntegration COMMAND test_port_economy_integration)

# Test executable for PortIncomeUI (Ticket E8-021)
add_executable(test_port_income_ui
    port/test_port_income_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_income_ui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortIncomeUI COMMAND test_port_income_ui)

# Test executable for PortRenderData (Ticket E8-030)
add_executable(test_port_render_data
    port/test_port_render_data.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_render_data PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortRenderData COMMAND test_port_render_data)

# Test executable for Comprehensive Port Validation (Ticket E8-036)
add_executable(test_port_validation_comprehensive
    port/test_port_validation_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortZoneValidation.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortCapacity.cpp
)
target_include_directories(test_port_validation_comprehensive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortValidationComprehensive COMMAND test_port_validation_comprehensive)

# Test executable for Comprehensive Trade Calculation (Ticket E8-037)
add_executable(test_trade_calculation_comprehensive
    port/test_trade_calculation_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
)
target_include_directories(test_trade_calculation_comprehensive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME TradeCalculationComprehensive COMMAND test_trade_calculation_comprehensive)

# Test executable for Neighbor Relationship Evolution (Ticket E8-034)
add_executable(test_neighbor_relationship
    port/test_neighbor_relationship.cpp
    ${CMAKE_SOURCE_DIR}/src/port/NeighborRelationship.cpp
)
target_include_directories(test_neighbor_relationship PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME NeighborRelationship COMMAND test_neighbor_relationship)

# Test executable for Port Integration Epic 10/11 (Ticket E8-038)
add_executable(test_port_integration_epic10_11
    port/test_port_integration_epic10_11.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_port_integration_epic10_11 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PortIntegrationEpic10_11 COMMAND test_port_integration_epic10_11)

# Test executable for Multiplayer Trade Routes (Ticket E8-039)
add_executable(test_multiplayer_trade_routes
    port/test_multiplayer_trade_routes.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeOfferManager.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeNetworkMessages.cpp
    ${CMAKE_SOURCE_DIR}/src/port/PortSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/port/DemandBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/port/TradeIncome.cpp
)
target_include_directories(test_multiplayer_trade_routes PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME MultiplayerTradeRoutes COMMAND test_multiplayer_trade_routes)

# Test executable for ServiceConfigs (Tickets E9-031, E9-032, E9-033)
add_executable(test_service_configs
    services/test_service_configs.cpp
)
target_include_directories(test_service_configs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceConfigs COMMAND test_service_configs)

# Test executable for ServiceTypes (Tickets E9-001, E9-030)
add_executable(test_service_types
    services/test_service_types.cpp
)
target_include_directories(test_service_types PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceTypes COMMAND test_service_types)

# Test executable for ServiceProviderComponent (Ticket E9-002)
add_executable(test_service_component
    services/test_service_component.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceSerialization.cpp
)
target_include_directories(test_service_component PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceComponent COMMAND test_service_component)

# Test executable for ServicesSystem (Ticket E9-003)
add_executable(test_services_system
    services/test_services_system.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServicesSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
)
target_include_directories(test_services_system PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServicesSystem COMMAND test_services_system)

# Test executable for ServiceStatistics (Ticket E9-053)
add_executable(test_service_statistics
    services/test_service_statistics.cpp
)
target_include_directories(test_service_statistics PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceStatistics COMMAND test_service_statistics)

# Test executable for IServiceQueryable interface (Ticket E9-004)
add_executable(test_service_queryable
    services/test_service_queryable.cpp
)
target_include_directories(test_service_queryable PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceQueryable COMMAND test_service_queryable)

# Test executable for FundingModifier (Ticket E9-024)
add_executable(test_funding_modifier
    services/test_funding_modifier.cpp
)
target_include_directories(test_funding_modifier PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME FundingModifier COMMAND test_funding_modifier)

# Test executable for ServiceCoverageGrid (Ticket E9-010)
add_executable(test_service_coverage_grid
    services/test_service_coverage_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_service_coverage_grid PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceCoverageGrid COMMAND test_service_coverage_grid)

# Test executable for CoverageCalculation (Ticket E9-020)
add_executable(test_coverage_calculation
    services/test_coverage_calculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_coverage_calculation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CoverageCalculation COMMAND test_coverage_calculation)

# Test executable for Dirty Flags (Ticket E9-011)
add_executable(test_dirty_flags
    services/test_dirty_flags.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServicesSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
)
target_include_directories(test_dirty_flags PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME DirtyFlags COMMAND test_dirty_flags)

# Test executable for ServiceEvents (Ticket E9-012)
add_executable(test_service_events
    services/test_service_events.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServicesSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
)
target_include_directories(test_service_events PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceEvents COMMAND test_service_events)

# Test executable for GlobalServiceAggregation (Ticket E9-023)
add_executable(test_global_service_aggregation
    services/test_global_service_aggregation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlobalServiceAggregation.cpp
)
target_include_directories(test_global_service_aggregation PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME GlobalServiceAggregation COMMAND test_global_service_aggregation)

# Test executable for ServiceCoverageOverlay (Ticket E9-043)
add_executable(test_service_coverage_overlay
    services/test_service_coverage_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_service_coverage_overlay PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME ServiceCoverageOverlay COMMAND test_service_coverage_overlay)

# Test executable for DisorderSuppression (Ticket E9-040)
add_executable(test_disorder_suppression
    services/test_disorder_suppression.cpp
)
target_include_directories(test_disorder_suppression PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME DisorderSuppression COMMAND test_disorder_suppression)

# Test executable for LongevityBonus (Ticket E9-041)
add_executable(test_longevity_bonus
    services/test_longevity_bonus.cpp
)
target_include_directories(test_longevity_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME LongevityBonus COMMAND test_longevity_bonus)

# Test executable for EducationBonus (Ticket E9-042)
add_executable(test_education_bonus
    services/test_education_bonus.cpp
)
target_include_directories(test_education_bonus PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME EducationBonus COMMAND test_education_bonus)

# Test executable for Global Services Comprehensive (Ticket E9-051)
add_executable(test_global_services_comprehensive
    services/test_global_services_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlobalServiceAggregation.cpp
)
target_include_directories(test_global_services_comprehensive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME GlobalServicesComprehensive COMMAND test_global_services_comprehensive)

# Test executable for LinearFalloff (Ticket E9-021)
add_executable(test_linear_falloff
    services/test_linear_falloff.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_linear_falloff PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME LinearFalloff COMMAND test_linear_falloff)

# Test executable for CoverageOverlap (Ticket E9-022)
add_executable(test_coverage_overlap
    services/test_coverage_overlap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_coverage_overlap PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CoverageOverlap COMMAND test_coverage_overlap)

# Test executable for CoverageComprehensive (Ticket E9-050)
add_executable(test_coverage_comprehensive
    services/test_coverage_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
)
target_include_directories(test_coverage_comprehensive PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME CoverageComprehensive COMMAND test_coverage_comprehensive)

# Test executable for Epic10 Integration (Ticket E9-052)
add_executable(test_epic10_integration
    services/test_epic10_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServicesSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CoverageCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlobalServiceAggregation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceSerialization.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ServiceCoverageOverlay.cpp
)
target_include_directories(test_epic10_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME Epic10Integration COMMAND test_epic10_integration)

# Epic 10: Demand data (E10-040)
add_executable(test_demand_data demand/test_demand_data.cpp)
target_include_directories(test_demand_data PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandData COMMAND test_demand_data)

# Epic 10: Disorder component (E10-070)
add_executable(test_disorder_component disorder/test_disorder_component.cpp)
target_include_directories(test_disorder_component PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderComponent COMMAND test_disorder_component)

# Epic 10: Contamination types (E10-071, E10-080)
add_executable(test_contamination_types contamination/test_contamination_types.cpp)
target_include_directories(test_contamination_types PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationTypes COMMAND test_contamination_types)

# Epic 10: Economy stubs (E10-113)
add_executable(test_economy_stubs economy/test_economy_stubs.cpp)
target_include_directories(test_economy_stubs PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EconomyStubs COMMAND test_economy_stubs)

# Test for SimulationCore (E10-001)
add_executable(test_simulation_core
    sim/test_simulation_core.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationCore.cpp
)
target_include_directories(test_simulation_core PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME SimulationCore COMMAND test_simulation_core)

# Test for Population Components (E10-010 through E10-013)
add_executable(test_population_components
    population/test_population_components.cpp
)
target_include_directories(test_population_components PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
add_test(NAME PopulationComponents COMMAND test_population_components)

# Epic 10: DisorderGrid (E10-060)
add_executable(test_disorder_grid
    disorder/test_disorder_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_grid PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderGrid COMMAND test_disorder_grid)

# Epic 10: ContaminationGrid (E10-061)
add_executable(test_contamination_grid
    contamination/test_contamination_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_grid PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationGrid COMMAND test_contamination_grid)

# Epic 10: ContaminationStats (E10-089)
add_executable(test_contamination_stats
    contamination/test_contamination_stats.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationStats.cpp
)
target_include_directories(test_contamination_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationStats COMMAND test_contamination_stats)

# Epic 10: ContaminationOverlay (E10-090)
add_executable(test_contamination_overlay
    contamination/test_contamination_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationOverlay.cpp
)
target_include_directories(test_contamination_overlay PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationOverlay COMMAND test_contamination_overlay)

# Epic 10: ContaminationEvents (E10-091)
add_executable(test_contamination_events
    contamination/test_contamination_events.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationEvents.cpp
)
target_include_directories(test_contamination_events PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationEvents COMMAND test_contamination_events)

# Epic 10: LandValueGrid (E10-062)
add_executable(test_landvalue_grid
    landvalue/test_landvalue_grid.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_landvalue_grid PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueGrid COMMAND test_landvalue_grid)

# Epic 10: Grid swap coordinator (E10-063)
add_executable(test_grid_swap_coordinator
    sim/test_grid_swap_coordinator.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_grid_swap_coordinator PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME GridSwapCoordinator COMMAND test_grid_swap_coordinator)

# Epic 10: Simulation speed (E10-002)
add_executable(test_simulation_speed
    sim/test_simulation_speed.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationCore.cpp
)
target_include_directories(test_simulation_speed PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SimulationSpeed COMMAND test_simulation_speed)

# Epic 10: Time progression (E10-003)
add_executable(test_time_progression
    sim/test_time_progression.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationCore.cpp
)
target_include_directories(test_time_progression PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TimeProgression COMMAND test_time_progression)

# Epic 10: Simulation events (E10-005)
add_executable(test_simulation_events
    sim/test_simulation_events.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationCore.cpp
)
target_include_directories(test_simulation_events PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SimulationEvents COMMAND test_simulation_events)

# Epic 10: IDemandProvider extended (E10-041)
add_executable(test_demand_provider_extended
    population/test_demand_provider_extended.cpp
)
target_include_directories(test_demand_provider_extended PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandProviderExtended COMMAND test_demand_provider_extended)

# Epic 10: PopulationSystem skeleton (E10-014)
add_executable(test_population_system
    population/test_population_system.cpp
    ${CMAKE_SOURCE_DIR}/src/population/PopulationSystem.cpp
)
target_include_directories(test_population_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PopulationSystem COMMAND test_population_system)

# Epic 10: Birth rate (E10-015)
add_executable(test_birth_rate
    population/test_birth_rate.cpp
    ${CMAKE_SOURCE_DIR}/src/population/BirthRateCalculation.cpp
)
target_include_directories(test_birth_rate PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME BirthRate COMMAND test_birth_rate)

# Epic 10: Death rate (E10-016)
add_executable(test_death_rate
    population/test_death_rate.cpp
    ${CMAKE_SOURCE_DIR}/src/population/DeathRateCalculation.cpp
)
target_include_directories(test_death_rate PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DeathRate COMMAND test_death_rate)

# Epic 10: Labor pool (E10-019)
add_executable(test_labor_pool
    population/test_labor_pool.cpp
    ${CMAKE_SOURCE_DIR}/src/population/LaborPoolCalculation.cpp
)
target_include_directories(test_labor_pool PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LaborPool COMMAND test_labor_pool)

# Epic 10: ContaminationSystem skeleton (E10-081)
add_executable(test_contamination_system
    contamination/test_contamination_system.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationSystem COMMAND test_contamination_system)

# Epic 10: LandValueSystem skeleton (E10-100)
add_executable(test_landvalue_system
    landvalue/test_landvalue_system.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_landvalue_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueSystem COMMAND test_landvalue_system)

# Epic 10: DemandSystem skeleton (E10-042)
add_executable(test_demand_system
    demand/test_demand_system.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandSystem.cpp
)
target_include_directories(test_demand_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandSystem COMMAND test_demand_system)

# Epic 10: DisorderSystem skeleton (E10-072)
add_executable(test_disorder_system
    disorder/test_disorder_system.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderSystem COMMAND test_disorder_system)

# Epic 10: Natural growth (E10-017)
add_executable(test_natural_growth
    population/test_natural_growth.cpp
    ${CMAKE_SOURCE_DIR}/src/population/NaturalGrowth.cpp
    ${CMAKE_SOURCE_DIR}/src/population/BirthRateCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/population/DeathRateCalculation.cpp
)
target_include_directories(test_natural_growth PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME NaturalGrowth COMMAND test_natural_growth)

# Epic 10: Job market (E10-020)
add_executable(test_job_market
    population/test_job_market.cpp
    ${CMAKE_SOURCE_DIR}/src/population/JobMarketAggregation.cpp
)
target_include_directories(test_job_market PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME JobMarket COMMAND test_job_market)

# Epic 10: Age distribution (E10-018)
add_executable(test_age_distribution
    population/test_age_distribution.cpp
    ${CMAKE_SOURCE_DIR}/src/population/AgeDistribution.cpp
)
target_include_directories(test_age_distribution PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME AgeDistribution COMMAND test_age_distribution)

# Epic 10: Building occupancy distribution (E10-022)
add_executable(test_occupancy_distribution
    population/test_occupancy_distribution.cpp
    ${CMAKE_SOURCE_DIR}/src/population/OccupancyDistribution.cpp
)
target_include_directories(test_occupancy_distribution PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME OccupancyDistribution COMMAND test_occupancy_distribution)

# Epic 10: Unemployment effects (E10-023)
add_executable(test_unemployment_effects
    population/test_unemployment_effects.cpp
    ${CMAKE_SOURCE_DIR}/src/population/UnemploymentEffects.cpp
)
target_include_directories(test_unemployment_effects PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME UnemploymentEffects COMMAND test_unemployment_effects)

# Epic 10: Attractiveness (E10-024)
add_executable(test_attractiveness
    population/test_attractiveness.cpp
    ${CMAKE_SOURCE_DIR}/src/population/AttractivenessCalculation.cpp
)
target_include_directories(test_attractiveness PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME Attractiveness COMMAND test_attractiveness)

# Epic 10: Habitation demand (E10-043)
add_executable(test_habitation_demand demand/test_habitation_demand.cpp ${CMAKE_SOURCE_DIR}/src/demand/HabitationDemand.cpp)
target_include_directories(test_habitation_demand PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME HabitationDemand COMMAND test_habitation_demand)

# Epic 10: Exchange demand (E10-044)
add_executable(test_exchange_demand demand/test_exchange_demand.cpp ${CMAKE_SOURCE_DIR}/src/demand/ExchangeDemand.cpp)
target_include_directories(test_exchange_demand PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ExchangeDemand COMMAND test_exchange_demand)

# Epic 10: Fabrication demand (E10-045)
add_executable(test_fabrication_demand demand/test_fabrication_demand.cpp ${CMAKE_SOURCE_DIR}/src/demand/FabricationDemand.cpp)
target_include_directories(test_fabrication_demand PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME FabricationDemand COMMAND test_fabrication_demand)

# Epic 10: Demand cap (E10-046)
add_executable(test_demand_cap demand/test_demand_cap.cpp ${CMAKE_SOURCE_DIR}/src/demand/DemandCap.cpp)
target_include_directories(test_demand_cap PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandCap COMMAND test_demand_cap)

# Epic 10: Disorder generation (E10-073)
add_executable(test_disorder_generation
    disorder/test_disorder_generation.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGeneration.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_generation PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderGeneration COMMAND test_disorder_generation)

# Epic 10: Contamination aggregator (E10-082)
add_executable(test_contamination_aggregator
    contamination/test_contamination_aggregator.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationAggregator.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_aggregator PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationAggregator COMMAND test_contamination_aggregator)

# Epic 10: Terrain contamination (E10-086)
add_executable(test_terrain_contamination
    contamination/test_terrain_contamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/TerrainContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_terrain_contamination PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TerrainContamination COMMAND test_terrain_contamination)

# Epic 10: Employment matching (E10-021)
add_executable(test_employment_matching population/test_employment_matching.cpp ${CMAKE_SOURCE_DIR}/src/population/EmploymentMatching.cpp)
target_include_directories(test_employment_matching PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EmploymentMatching COMMAND test_employment_matching)

# Epic 10: Migration in (E10-025)
add_executable(test_migration_in population/test_migration_in.cpp ${CMAKE_SOURCE_DIR}/src/population/MigrationIn.cpp)
target_include_directories(test_migration_in PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MigrationIn COMMAND test_migration_in)

# Epic 10: Migration out (E10-026)
add_executable(test_migration_out population/test_migration_out.cpp ${CMAKE_SOURCE_DIR}/src/population/MigrationOut.cpp)
target_include_directories(test_migration_out PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MigrationOut COMMAND test_migration_out)

# Epic 10: Terrain value factors (E10-101)
add_executable(test_terrain_value_factors
    landvalue/test_terrain_value_factors.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/TerrainValueFactors.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_terrain_value_factors PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TerrainValueFactors COMMAND test_terrain_value_factors)

# Epic 10: Road access bonus (E10-102)
add_executable(test_road_access_bonus
    landvalue/test_road_access_bonus.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/RoadAccessBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_road_access_bonus PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME RoadAccessBonus COMMAND test_road_access_bonus)

# Epic 10: Land value disorder (E10-074)
add_executable(test_land_value_disorder
    disorder/test_land_value_disorder.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/LandValueDisorderEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_land_value_disorder PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueDisorder COMMAND test_land_value_disorder)

# Epic 10: Disorder spread (E10-075)
add_executable(test_disorder_spread
    disorder/test_disorder_spread.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderSpread.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_spread PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderSpread COMMAND test_disorder_spread)

# Epic 10: Industrial contamination (E10-083)
add_executable(test_industrial_contamination
    contamination/test_industrial_contamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/IndustrialContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_industrial_contamination PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME IndustrialContamination COMMAND test_industrial_contamination)

# Epic 10: Energy contamination (E10-084)
add_executable(test_energy_contamination
    contamination/test_energy_contamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/EnergyContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_energy_contamination PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EnergyContamination COMMAND test_energy_contamination)

# Epic 10: Traffic contamination (E10-085)
add_executable(test_traffic_contamination_gen
    contamination/test_traffic_contamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/TrafficContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_traffic_contamination_gen PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TrafficContamination_Gen COMMAND test_traffic_contamination_gen)

# Epic 10: Migration application (E10-027)
add_executable(test_migration_application
    population/test_migration_application.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationIn.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationOut.cpp
)
target_include_directories(test_migration_application PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MigrationApplication COMMAND test_migration_application)

# Epic 10: Demand integration (E10-049)
add_executable(test_demand_integration
    demand/test_demand_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandIntegration.cpp
)
target_include_directories(test_demand_integration PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandIntegration COMMAND test_demand_integration)

# Epic 10: Enforcer suppression integration (E10-076)
add_executable(test_enforcer_suppression
    disorder/test_enforcer_suppression.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/EnforcerSuppression.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_enforcer_suppression PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EnforcerSuppression COMMAND test_enforcer_suppression)

# Epic 10: Demand factors for UI (E10-047)
add_executable(test_demand_factors_ui
    demand/test_demand_factors_ui.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandFactorsUI.cpp
)
target_include_directories(test_demand_factors_ui PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandFactorsUI COMMAND test_demand_factors_ui)

# Epic 10: Contamination spread (E10-087)
add_executable(test_contamination_spread
    contamination/test_contamination_spread.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationSpread.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_spread PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationSpread COMMAND test_contamination_spread)

# Epic 10: Disorder penalty (E10-103)
add_executable(test_disorder_penalty
    landvalue/test_disorder_penalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/DisorderPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_penalty PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderPenalty COMMAND test_disorder_penalty)

# Epic 10: Contamination penalty (E10-104)
add_executable(test_contamination_penalty
    landvalue/test_contamination_penalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/ContaminationPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_penalty PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationPenalty COMMAND test_contamination_penalty)

# Epic 10: Contamination decay algorithm (E10-088)
add_executable(test_contamination_decay
    contamination/test_contamination_decay.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationDecay.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_contamination_decay PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationDecay COMMAND test_contamination_decay)

# Epic 10: Full land value recalculation (E10-105)
add_executable(test_full_value_recalc
    landvalue/test_full_value_recalc.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/FullValueRecalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/TerrainValueFactors.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/RoadAccessBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/DisorderPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/ContaminationPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_full_value_recalc PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME FullValueRecalculation COMMAND test_full_value_recalc)
# Epic 10: Life expectancy calculation (E10-028)
add_executable(test_life_expectancy
    population/test_life_expectancy.cpp
    ${CMAKE_SOURCE_DIR}/src/population/LifeExpectancy.cpp
)
target_include_directories(test_life_expectancy PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LifeExpectancy COMMAND test_life_expectancy)

# Epic 10: Population stats query (E10-030)
add_executable(test_population_stats
    population/test_population_stats.cpp
    ${CMAKE_SOURCE_DIR}/src/population/PopulationStats.cpp
    ${CMAKE_SOURCE_DIR}/src/population/LifeExpectancy.cpp
)
target_include_directories(test_population_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PopulationStats COMMAND test_population_stats)

# Epic 10: Population milestones (E10-031)
add_executable(test_population_milestones
    population/test_population_milestones.cpp
    ${CMAKE_SOURCE_DIR}/src/population/PopulationMilestones.cpp
)
target_include_directories(test_population_milestones PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PopulationMilestones COMMAND test_population_milestones)

# Epic 10: Population network sync (E10-032)
add_executable(test_population_net_sync
    population/test_population_net_sync.cpp
    ${CMAKE_SOURCE_DIR}/src/population/PopulationNetSync.cpp
)
target_include_directories(test_population_net_sync PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME PopulationNetSync COMMAND test_population_net_sync)

# Epic 10: Disorder stats (E10-077)
add_executable(test_disorder_stats
    disorder/test_disorder_stats.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderStats.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderStats COMMAND test_disorder_stats)

# Epic 10: Disorder overlay (E10-078)
add_executable(test_disorder_overlay
    disorder/test_disorder_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_overlay PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderOverlay COMMAND test_disorder_overlay)

# Epic 10: Disorder events (E10-079)
add_executable(test_disorder_events
    disorder/test_disorder_events.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderEvents.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
)
target_include_directories(test_disorder_events PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderEvents COMMAND test_disorder_events)

# Epic 10: Health index system (E10-029)
add_executable(test_health_index
    population/test_health_index.cpp
    ${CMAKE_SOURCE_DIR}/src/population/HealthIndex.cpp
)
target_include_directories(test_health_index PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME HealthIndex COMMAND test_health_index)

# Epic 10: Demand stats (E10-048)
add_executable(test_demand_stats
    demand/test_demand_stats.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandStats.cpp
)
target_include_directories(test_demand_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandStats COMMAND test_demand_stats)

# Epic 10: Land value stats (E10-106)
add_executable(test_landvalue_stats
    landvalue/test_landvalue_stats.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueStats.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_landvalue_stats PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueStats COMMAND test_landvalue_stats)

# Epic 10: Land value overlay (E10-107)
add_executable(test_landvalue_overlay
    landvalue/test_landvalue_overlay.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueOverlay.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_landvalue_overlay PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueOverlay COMMAND test_landvalue_overlay)

# Epic 10: Abandonment system (E10-110)
add_executable(test_abandonment_system
    population/test_abandonment_system.cpp
    ${CMAKE_SOURCE_DIR}/src/population/AbandonmentSystem.cpp
)
target_include_directories(test_abandonment_system PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME AbandonmentSystem COMMAND test_abandonment_system)

# Epic 10: Simulation state serialization (E10-111)
add_executable(test_simulation_serializer
    sim/test_simulation_serializer.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationSerializer.cpp
)
target_include_directories(test_simulation_serializer PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SimulationSerializer COMMAND test_simulation_serializer)

# Epic 10: Energy contamination adapter (E10-114)
add_executable(test_energy_contamination_adapter
    energy/test_energy_contamination_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/energy/EnergyContaminationAdapter.cpp
)
target_include_directories(test_energy_contamination_adapter PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EnergyContaminationAdapter COMMAND test_energy_contamination_adapter)

# Epic 10: Traffic contamination adapter (E10-115)
add_executable(test_traffic_contamination_adapter
    transport/test_traffic_contamination_adapter.cpp
    ${CMAKE_SOURCE_DIR}/src/transport/TrafficContaminationAdapter.cpp
)
target_include_directories(test_traffic_contamination_adapter PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME TrafficContaminationAdapter COMMAND test_traffic_contamination_adapter)

# Epic 10: Demographics comprehensive tests (E10-120)
add_executable(test_demographics_comprehensive
    population/test_demographics_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/population/BirthRateCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/population/DeathRateCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/population/NaturalGrowth.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationIn.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationOut.cpp
    ${CMAKE_SOURCE_DIR}/src/population/MigrationApplication.cpp
    ${CMAKE_SOURCE_DIR}/src/population/AgeDistribution.cpp
)
target_include_directories(test_demographics_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemographicsComprehensive COMMAND test_demographics_comprehensive)

# Epic 10: Employment comprehensive tests (E10-121)
add_executable(test_employment_comprehensive
    population/test_employment_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/population/LaborPoolCalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/population/JobMarketAggregation.cpp
    ${CMAKE_SOURCE_DIR}/src/population/EmploymentMatching.cpp
    ${CMAKE_SOURCE_DIR}/src/population/OccupancyDistribution.cpp
    ${CMAKE_SOURCE_DIR}/src/population/UnemploymentEffects.cpp
)
target_include_directories(test_employment_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME EmploymentComprehensive COMMAND test_employment_comprehensive)

# Epic 10: Comprehensive demand tests (E10-122)
add_executable(test_demand_comprehensive
    demand/test_demand_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/HabitationDemand.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/ExchangeDemand.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/FabricationDemand.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandCap.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandFactorsUI.cpp
)
target_include_directories(test_demand_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DemandComprehensive COMMAND test_demand_comprehensive)

# Epic 10: Comprehensive disorder tests (E10-123)
add_executable(test_disorder_comprehensive
    disorder/test_disorder_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderSpread.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGeneration.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/LandValueDisorderEffect.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/EnforcerSuppression.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderStats.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_disorder_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DisorderComprehensive COMMAND test_disorder_comprehensive)


# Epic 10: Contamination comprehensive tests (E10-124)
add_executable(test_contamination_comprehensive
    contamination/test_contamination_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationSpread.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationDecay.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/TerrainContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/IndustrialContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/EnergyContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/TrafficContamination.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationStats.cpp
)
target_include_directories(test_contamination_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ContaminationComprehensive COMMAND test_contamination_comprehensive)

# Epic 10: Land value comprehensive tests (E10-125)
add_executable(test_landvalue_comprehensive
    landvalue/test_landvalue_comprehensive.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/TerrainValueFactors.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/RoadAccessBonus.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/DisorderPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/ContaminationPenalty.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/FullValueRecalculation.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueStats.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
)
target_include_directories(test_landvalue_comprehensive PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME LandValueComprehensive COMMAND test_landvalue_comprehensive)

# Epic 10: Simulation integration tests (E10-126)
add_executable(test_simulation_integration
    sim/test_simulation_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/sim/SimulationCore.cpp
    ${CMAKE_SOURCE_DIR}/src/population/PopulationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/demand/DemandSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/disorder/DisorderGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/contamination/ContaminationGrid.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/landvalue/LandValueGrid.cpp
)
target_include_directories(test_simulation_integration PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME SimulationIntegration COMMAND test_simulation_integration)
