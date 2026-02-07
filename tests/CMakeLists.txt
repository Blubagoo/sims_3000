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
