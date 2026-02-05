cmake_minimum_required(VERSION 3.21)

project(sims_3000
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# POC build options
option(BUILD_POC1 "Build POC-1: Toon Rendering" OFF)
option(BUILD_POC2 "Build POC-2: ENet Multiplayer Snapshot Sync" OFF)
option(BUILD_POC3 "Build POC-3: Dense Grid Performance" OFF)
option(BUILD_POC4 "Build POC-4: ECS Simulation Loop" OFF)
option(BUILD_POC6 "Build POC-6: SDL_GPU UI Overlay" OFF)

# Find SDL3
find_package(SDL3 REQUIRED CONFIG)

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Link SDL3
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
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
