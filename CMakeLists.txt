cmake_minimum_required(VERSION 3.21)

project(sims_3000
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# POC build options
option(BUILD_POC1 "Build POC-1: Toon Rendering" OFF)
option(BUILD_POC2 "Build POC-2: ENet Multiplayer Snapshot Sync" OFF)
option(BUILD_POC3 "Build POC-3: Dense Grid Performance" OFF)
option(BUILD_POC4 "Build POC-4: ECS Simulation Loop" OFF)
option(BUILD_POC6 "Build POC-6: SDL_GPU UI Overlay" OFF)
option(BUILD_TESTS "Build unit tests" OFF)

# Find dependencies
find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(EnTT REQUIRED CONFIG)
find_package(lz4 REQUIRED CONFIG)
find_package(unofficial-enet CONFIG REQUIRED)
find_package(readerwriterqueue CONFIG REQUIRED)

# Find cgltf (header-only)
find_path(CGLTF_INCLUDE_DIR cgltf.h)
if(NOT CGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "cgltf.h not found")
endif()

# nlohmann/json for configuration (add to vcpkg.json if not present)
find_package(nlohmann_json QUIET CONFIG)

# Find stb_image (header-only)
find_path(STB_INCLUDE_DIR stb_image.h)
if(NOT STB_INCLUDE_DIR)
    message(FATAL_ERROR "stb_image.h not found")
endif()

# =============================================================================
# Shader Compilation Pipeline
# =============================================================================

# Include shader compilation module
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ShaderCompilation.cmake)

# Find DXC compiler (required for shader compilation)
find_dxc_compiler()

# Compile fallback shaders and generate embedded shader source
set(EMBEDDED_SHADERS_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedShaders.cpp")
generate_embedded_shaders(
    OUTPUT_FILE "${EMBEDDED_SHADERS_OUTPUT}"
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders"
    SHADERS
        "fallback.vert:vs_6_0"
        "fallback.frag:ps_6_0"
)

# Compile toon shaders to output directory (loaded at runtime)
set(TOON_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY "${TOON_SHADER_OUTPUT_DIR}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/toon.vert.hlsl"
    NAME "toon.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TOON_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(TOON_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/toon.frag.hlsl"
    NAME "toon.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TOON_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(TOON_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for toon shaders
add_custom_target(toon_shaders DEPENDS
    "${TOON_VERT_DXIL}"
    "${TOON_VERT_SPIRV}"
    "${TOON_FRAG_DXIL}"
    "${TOON_FRAG_SPIRV}"
)

# Compile edge detection shaders (fullscreen triangle + Sobel edge detection)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/fullscreen.vert.hlsl"
    NAME "fullscreen.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(FULLSCREEN_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(FULLSCREEN_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/edge_detect.frag.hlsl"
    NAME "edge_detect.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(EDGE_DETECT_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(EDGE_DETECT_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for edge detection shaders
add_custom_target(edge_shaders DEPENDS
    "${FULLSCREEN_VERT_DXIL}"
    "${FULLSCREEN_VERT_SPIRV}"
    "${EDGE_DETECT_FRAG_DXIL}"
    "${EDGE_DETECT_FRAG_SPIRV}"
)

# Compile bloom shaders (extraction, blur, composite)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/bloom_extract.frag.hlsl"
    NAME "bloom_extract.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(BLOOM_EXTRACT_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(BLOOM_EXTRACT_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/bloom_blur.frag.hlsl"
    NAME "bloom_blur.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(BLOOM_BLUR_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(BLOOM_BLUR_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/bloom_composite.frag.hlsl"
    NAME "bloom_composite.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(BLOOM_COMPOSITE_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(BLOOM_COMPOSITE_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for bloom shaders
add_custom_target(bloom_shaders DEPENDS
    "${BLOOM_EXTRACT_FRAG_DXIL}"
    "${BLOOM_EXTRACT_FRAG_SPIRV}"
    "${BLOOM_BLUR_FRAG_DXIL}"
    "${BLOOM_BLUR_FRAG_SPIRV}"
    "${BLOOM_COMPOSITE_FRAG_DXIL}"
    "${BLOOM_COMPOSITE_FRAG_SPIRV}"
)

# Compile debug grid shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/debug_grid.vert.hlsl"
    NAME "debug_grid.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(DEBUG_GRID_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(DEBUG_GRID_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/debug_grid.frag.hlsl"
    NAME "debug_grid.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(DEBUG_GRID_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(DEBUG_GRID_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for debug grid shaders
add_custom_target(debug_grid_shaders DEPENDS
    "${DEBUG_GRID_VERT_DXIL}"
    "${DEBUG_GRID_VERT_SPIRV}"
    "${DEBUG_GRID_FRAG_DXIL}"
    "${DEBUG_GRID_FRAG_SPIRV}"
)

# Compile stats overlay shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/stats_overlay.vert.hlsl"
    NAME "stats_overlay.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(STATS_OVERLAY_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(STATS_OVERLAY_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/stats_overlay.frag.hlsl"
    NAME "stats_overlay.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(STATS_OVERLAY_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(STATS_OVERLAY_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for stats overlay shaders
add_custom_target(stats_overlay_shaders DEPENDS
    "${STATS_OVERLAY_VERT_DXIL}"
    "${STATS_OVERLAY_VERT_SPIRV}"
    "${STATS_OVERLAY_FRAG_DXIL}"
    "${STATS_OVERLAY_FRAG_SPIRV}"
)

# Compile debug bounding box shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/debug_bbox.vert.hlsl"
    NAME "debug_bbox.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(DEBUG_BBOX_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(DEBUG_BBOX_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/debug_bbox.frag.hlsl"
    NAME "debug_bbox.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(DEBUG_BBOX_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(DEBUG_BBOX_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for debug bounding box shaders
add_custom_target(debug_bbox_shaders DEPENDS
    "${DEBUG_BBOX_VERT_DXIL}"
    "${DEBUG_BBOX_VERT_SPIRV}"
    "${DEBUG_BBOX_FRAG_DXIL}"
    "${DEBUG_BBOX_FRAG_SPIRV}"
)

# Compile terrain shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/terrain.vert.hlsl"
    NAME "terrain.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TERRAIN_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(TERRAIN_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/terrain.frag.hlsl"
    NAME "terrain.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TERRAIN_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(TERRAIN_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for terrain shaders
add_custom_target(terrain_shaders DEPENDS
    "${TERRAIN_VERT_DXIL}"
    "${TERRAIN_VERT_SPIRV}"
    "${TERRAIN_FRAG_DXIL}"
    "${TERRAIN_FRAG_SPIRV}"
)

# Compile water shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/water.vert.hlsl"
    NAME "water.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(WATER_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(WATER_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/water.frag.hlsl"
    NAME "water.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(WATER_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(WATER_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for water shaders
add_custom_target(water_shaders DEPENDS
    "${WATER_VERT_DXIL}"
    "${WATER_VERT_SPIRV}"
    "${WATER_FRAG_DXIL}"
    "${WATER_FRAG_SPIRV}"
)

# Compile terrain debug shaders (vertex + fragment)
compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/terrain_debug.vert.hlsl"
    NAME "terrain_debug.vert"
    PROFILE "vs_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TERRAIN_DEBUG_VERT_DXIL "${SHADER_DXIL_OUTPUT}")
set(TERRAIN_DEBUG_VERT_SPIRV "${SHADER_SPIRV_OUTPUT}")

compile_shader(
    SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/terrain_debug.frag.hlsl"
    NAME "terrain_debug.frag"
    PROFILE "ps_6_0"
    OUTPUT_DIR "${TOON_SHADER_OUTPUT_DIR}"
)
set(TERRAIN_DEBUG_FRAG_DXIL "${SHADER_DXIL_OUTPUT}")
set(TERRAIN_DEBUG_FRAG_SPIRV "${SHADER_SPIRV_OUTPUT}")

# Create custom target for terrain debug shaders
add_custom_target(terrain_debug_shaders DEPENDS
    "${TERRAIN_DEBUG_VERT_DXIL}"
    "${TERRAIN_DEBUG_VERT_SPIRV}"
    "${TERRAIN_DEBUG_FRAG_DXIL}"
    "${TERRAIN_DEBUG_FRAG_SPIRV}"
)

# Define source files
set(SIMS3000_SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/SimulationClock.cpp
    src/app/FrameStats.cpp
    src/app/Config.cpp
    src/app/DebugConsole.cpp
    src/app/ServerCLI.cpp
    src/core/Random.cpp
    src/core/Logger.cpp
    src/render/Window.cpp
    src/render/GPUDevice.cpp
    src/render/ShaderCompiler.cpp
    src/render/DepthBuffer.cpp
    src/render/DepthState.cpp
    src/render/UniformBufferPool.cpp
    src/render/SamplerCache.cpp
    src/render/FrameResources.cpp
    src/render/GPUMesh.cpp
    src/render/ViewMatrix.cpp
    src/render/ProjectionMatrix.cpp
    src/render/BlendState.cpp
    src/render/ToonPipeline.cpp
    src/render/ToonShaderConfig.cpp
    src/render/RenderCommands.cpp
    src/render/CameraUniforms.cpp
    src/render/InstanceBuffer.cpp
    src/render/InstancedRenderer.cpp
    src/render/ScreenToWorld.cpp
    src/render/TilePicking.cpp
    src/render/BloomPass.cpp
    src/render/MainRenderPass.cpp
    src/render/TransparentRenderQueue.cpp
    src/render/NormalBuffer.cpp
    src/render/EdgeDetectionPass.cpp
    src/render/ViewportBounds.cpp
    src/render/FrustumCuller.cpp
    src/render/ShadowConfig.cpp
    src/render/ShadowPass.cpp
    src/render/LayerVisibility.cpp
    src/render/ViewMode.cpp
    src/render/DebugGridOverlay.cpp
    src/render/StatsOverlay.cpp
    src/render/DebugBoundingBoxOverlay.cpp
    src/render/CursorRenderer.cpp
    src/render/LODSystem.cpp
    src/render/VegetationPlacementGenerator.cpp
    src/render/VegetationRenderer.cpp
    src/render/TerrainDebugOverlay.cpp
    ${EMBEDDED_SHADERS_OUTPUT}
    src/input/InputSystem.cpp
    src/input/ActionMapping.cpp
    src/input/CameraController.cpp
    src/input/ZoomController.cpp
    src/input/PanController.cpp
    src/input/OrbitController.cpp
    src/input/CameraAnimator.cpp
    src/input/PresetSnapController.cpp
    src/input/CameraModeManager.cpp
    src/input/InputContext.cpp
    src/input/InputProducer.cpp
    src/input/PendingActionTracker.cpp
    src/assets/AssetManager.cpp
    src/assets/TextureLoader.cpp
    src/assets/ModelLoader.cpp
    src/assets/AsyncLoader.cpp
    src/assets/AudioLoader.cpp
    src/ecs/Registry.cpp
    src/ecs/SystemManager.cpp
    src/ecs/Components.cpp
    src/ecs/PositionSyncSystem.cpp
    src/ecs/TransformInterpolationSystem.cpp
    src/net/NetworkBuffer.cpp
    src/net/ENetTransport.cpp
    src/net/NetworkThread.cpp
    src/net/NetworkMessage.cpp
    src/net/ClientMessages.cpp
    src/net/ServerMessages.cpp
    src/net/NetworkClient.cpp
    src/net/NetworkServer.cpp
    src/net/InputHandler.cpp
    src/net/RateLimiter.cpp
    src/net/ConnectionValidator.cpp
    src/sync/SyncSystem.cpp
    src/sync/EntityIdGenerator.cpp
    src/persistence/FilePersistenceProvider.cpp
    src/terrain/ChunkDirtyTracker.cpp
    src/terrain/WaterDistanceField.cpp
    src/terrain/ProceduralNoise.cpp
    src/terrain/ContaminationSourceQuery.cpp
    src/terrain/ElevationGenerator.cpp
    src/terrain/BiomeGenerator.cpp
    src/terrain/WaterBodyGenerator.cpp
    src/terrain/SpawnPointGenerator.cpp
    src/terrain/MapValidator.cpp
    src/terrain/TerrainModificationSystem.cpp
    src/terrain/GradeTerrainOperation.cpp
    src/terrain/TerraformOperation.cpp
    src/terrain/TerrainNetworkMessages.cpp
    src/terrain/TerrainNetworkHandler.cpp
    src/terrain/TerrainClientHandler.cpp
    src/terrain/TerrainNormals.cpp
    src/terrain/WaterMeshGenerator.cpp
    src/terrain/TerrainChunkMeshGenerator.cpp
    src/terrain/TerrainModificationVisualPipeline.cpp
    src/terrain/TerrainChunkCuller.cpp
    src/terrain/TerrainGridSerializer.cpp
    src/terrain/TerrainNetworkSync.cpp
)

# Define header files (for IDE integration)
set(SIMS3000_HEADERS
    include/sims3000/core/types.h
    include/sims3000/core/ISimulationTime.h
    include/sims3000/core/ISimulatable.h
    include/sims3000/core/Interpolatable.h
    include/sims3000/core/Serialization.h
    include/sims3000/core/Logger.h
    include/sims3000/app/Application.h
    include/sims3000/app/AppState.h
    include/sims3000/app/Config.h
    include/sims3000/app/SimulationClock.h
    include/sims3000/app/FrameStats.h
    include/sims3000/render/Window.h
    include/sims3000/render/GPUDevice.h
    include/sims3000/render/ShaderCompiler.h
    include/sims3000/render/CameraState.h
    include/sims3000/render/RenderLayer.h
    include/sims3000/render/DepthBuffer.h
    include/sims3000/render/DepthState.h
    include/sims3000/render/UniformBufferPool.h
    include/sims3000/render/SamplerCache.h
    include/sims3000/render/FrameResources.h
    include/sims3000/render/ToonShader.h
    include/sims3000/render/ToonShaderConfig.h
    include/sims3000/render/BlendState.h
    include/sims3000/render/ToonPipeline.h
    include/sims3000/render/GPUMesh.h
    include/sims3000/render/ViewMatrix.h
    include/sims3000/render/ProjectionMatrix.h
    include/sims3000/render/RenderCommands.h
    include/sims3000/render/CameraUniforms.h
    include/sims3000/render/InstanceBuffer.h
    include/sims3000/render/InstancedRenderer.h
    include/sims3000/render/EmissiveMaterial.h
    include/sims3000/render/ScreenToWorld.h
    include/sims3000/render/TilePicking.h
    include/sims3000/render/BloomPass.h
    include/sims3000/render/MainRenderPass.h
    include/sims3000/render/TransparentRenderQueue.h
    include/sims3000/render/NormalBuffer.h
    include/sims3000/render/EdgeDetectionPass.h
    include/sims3000/render/ViewportBounds.h
    include/sims3000/render/FrustumCuller.h
    include/sims3000/render/ShadowConfig.h
    include/sims3000/render/ShadowPass.h
    include/sims3000/render/LayerVisibility.h
    include/sims3000/render/ViewMode.h
    include/sims3000/render/DebugGridOverlay.h
    include/sims3000/render/StatsOverlay.h
    include/sims3000/render/DebugBoundingBoxOverlay.h
    include/sims3000/render/PlayerCursor.h
    include/sims3000/render/CursorRenderer.h
    include/sims3000/render/LODSystem.h
    include/sims3000/render/TerrainVisualConfig.h
    include/sims3000/render/WaterVisualConfig.h
    include/sims3000/render/VegetationInstance.h
    include/sims3000/render/VegetationRenderer.h
    include/sims3000/render/TerrainDebugOverlay.h
    include/sims3000/sync/ICursorSync.h
    include/sims3000/input/InputSystem.h
    include/sims3000/input/ActionMapping.h
    include/sims3000/input/CameraController.h
    include/sims3000/input/ZoomController.h
    include/sims3000/input/PanController.h
    include/sims3000/input/OrbitController.h
    include/sims3000/input/CameraAnimator.h
    include/sims3000/input/PresetSnapController.h
    include/sims3000/input/CameraModeManager.h
    include/sims3000/core/Easing.h
    include/sims3000/assets/AssetManager.h
    include/sims3000/assets/TextureLoader.h
    include/sims3000/assets/ModelLoader.h
    include/sims3000/ecs/Registry.h
    include/sims3000/ecs/Components.h
    include/sims3000/ecs/SystemManager.h
    include/sims3000/ecs/PositionSyncSystem.h
    include/sims3000/ecs/InterpolatedTransformComponent.h
    include/sims3000/ecs/TransformInterpolationSystem.h
    include/sims3000/net/InputMessage.h
    include/sims3000/net/NetworkBuffer.h
    include/sims3000/net/INetworkTransport.h
    include/sims3000/net/ENetTransport.h
    include/sims3000/net/MockTransport.h
    include/sims3000/net/NetworkThread.h
    include/sims3000/net/NetworkMessage.h
    include/sims3000/net/ClientMessages.h
    include/sims3000/net/ServerMessages.h
    include/sims3000/net/NetworkClient.h
    include/sims3000/net/NetworkServer.h
    include/sims3000/net/INetworkHandler.h
    include/sims3000/net/InputHandler.h
    include/sims3000/net/RateLimiter.h
    include/sims3000/net/ConnectionValidator.h
    include/sims3000/input/InputProducer.h
    include/sims3000/input/PendingActionTracker.h
    include/sims3000/sync/SyncSystem.h
    include/sims3000/sync/EntityIdGenerator.h
    include/sims3000/persistence/IPersistenceProvider.h
    include/sims3000/persistence/NullPersistenceProvider.h
    include/sims3000/persistence/FilePersistenceProvider.h
    include/sims3000/terrain/TerrainTypes.h
    include/sims3000/terrain/TerrainTypeInfo.h
    include/sims3000/terrain/TerrainGrid.h
    include/sims3000/terrain/TerrainEvents.h
    include/sims3000/terrain/ChunkDirtyTracker.h
    include/sims3000/terrain/WaterData.h
    include/sims3000/terrain/WaterDistanceField.h
    include/sims3000/terrain/ProceduralNoise.h
    include/sims3000/terrain/ContaminationSourceQuery.h
    include/sims3000/terrain/ITerrainQueryable.h
    include/sims3000/terrain/ITerrainRenderData.h
    include/sims3000/terrain/ElevationGenerator.h
    include/sims3000/terrain/BiomeGenerator.h
    include/sims3000/terrain/WaterBodyGenerator.h
    include/sims3000/terrain/SpawnPointGenerator.h
    include/sims3000/terrain/MapSizeScaling.h
    include/sims3000/terrain/MapValidator.h
    include/sims3000/terrain/ITerrainModifier.h
    include/sims3000/terrain/TerrainModificationSystem.h
    include/sims3000/terrain/TerrainModificationComponent.h
    include/sims3000/terrain/GradeTerrainOperation.h
    include/sims3000/terrain/TerraformOperation.h
    include/sims3000/terrain/TerrainVertex.h
    include/sims3000/terrain/TerrainChunk.h
    include/sims3000/terrain/TerrainLODMesh.h
    include/sims3000/terrain/TerrainNetworkMessages.h
    include/sims3000/terrain/TerrainNetworkHandler.h
    include/sims3000/terrain/TerrainClientHandler.h
    include/sims3000/terrain/TerrainNormals.h
    include/sims3000/terrain/WaterMesh.h
    include/sims3000/terrain/TerrainChunkMeshGenerator.h
    include/sims3000/terrain/TerrainModificationVisualPipeline.h
    include/sims3000/terrain/TerrainChunkCuller.h
    include/sims3000/terrain/TerrainGridSerializer.h
    include/sims3000/terrain/TerrainNetworkSync.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SIMS3000_SOURCES}
    ${SIMS3000_HEADERS}
)

# Depend on embedded shaders generation and runtime shaders
add_dependencies(${PROJECT_NAME} embedded_shaders toon_shaders edge_shaders bloom_shaders debug_grid_shaders stats_overlay_shaders debug_bbox_shaders terrain_shaders water_shaders terrain_debug_shaders)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CGLTF_INCLUDE_DIR}
        ${STB_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        glm::glm
        EnTT::EnTT
        lz4::lz4
        unofficial::enet::enet
        readerwriterqueue::readerwriterqueue
)

# ENet requires Windows networking libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMS3000_HAS_JSON=1)
endif()

# Compiler warnings and strict FP for cross-platform determinism
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX- /fp:strict)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -ffp-contract=off)
endif()

# Debug/Release definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:SIMS3000_DEBUG=1>
    $<$<CONFIG:Release>:SIMS3000_RELEASE=1>
)

# Copy DLLs on Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3_ttf::SDL3_ttf>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Copy compiled shaders to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${TOON_SHADER_OUTPUT_DIR}"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMENT "Copying compiled shaders to output directory"
)

# POC subdirectories
if(BUILD_POC1)
    add_subdirectory(poc/poc1-toon-rendering)
endif()

if(BUILD_POC2)
    add_subdirectory(poc/poc2-enet-snapshot-sync)
endif()

if(BUILD_POC3)
    add_subdirectory(poc/poc3-dense-grid)
endif()

if(BUILD_POC4)
    add_subdirectory(poc/poc4-ecs-simulation)
endif()

if(BUILD_POC6)
    add_subdirectory(poc/poc6-ui-overlay)
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
