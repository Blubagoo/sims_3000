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

# Find cgltf (header-only)
find_path(CGLTF_INCLUDE_DIR cgltf.h)
if(NOT CGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "cgltf.h not found")
endif()

# nlohmann/json for configuration (add to vcpkg.json if not present)
find_package(nlohmann_json QUIET CONFIG)

# Define source files
set(SIMS3000_SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/app/SimulationClock.cpp
    src/app/FrameStats.cpp
    src/app/Config.cpp
    src/render/Window.cpp
    src/input/InputSystem.cpp
    src/input/ActionMapping.cpp
    src/input/CameraController.cpp
    src/assets/AssetManager.cpp
    src/assets/TextureLoader.cpp
    src/assets/ModelLoader.cpp
    src/ecs/Registry.cpp
    src/ecs/SystemManager.cpp
)

# Define header files (for IDE integration)
set(SIMS3000_HEADERS
    include/sims3000/core/types.h
    include/sims3000/core/ISimulationTime.h
    include/sims3000/core/ISimulatable.h
    include/sims3000/core/Serialization.h
    include/sims3000/app/Application.h
    include/sims3000/app/AppState.h
    include/sims3000/app/Config.h
    include/sims3000/app/SimulationClock.h
    include/sims3000/app/FrameStats.h
    include/sims3000/render/Window.h
    include/sims3000/input/InputSystem.h
    include/sims3000/input/ActionMapping.h
    include/sims3000/input/CameraController.h
    include/sims3000/assets/AssetManager.h
    include/sims3000/assets/TextureLoader.h
    include/sims3000/assets/ModelLoader.h
    include/sims3000/ecs/Registry.h
    include/sims3000/ecs/Components.h
    include/sims3000/ecs/SystemManager.h
    include/sims3000/net/InputMessage.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SIMS3000_SOURCES}
    ${SIMS3000_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CGLTF_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        glm::glm
        EnTT::EnTT
        lz4::lz4
)

if(nlohmann_json_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMS3000_HAS_JSON=1)
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
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
