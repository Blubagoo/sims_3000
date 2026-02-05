cmake_minimum_required(VERSION 3.21)

project(poc2_enet_snapshot_sync LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)
find_package(unofficial-enet CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    src/network_buffer.cpp
    src/compression.cpp
    src/entity_store.cpp
    src/simulation.cpp
    src/snapshot_generator.cpp
    src/snapshot_applier.cpp
    src/server.cpp
    src/client.cpp
    src/packet_loss_sim.cpp
    src/benchmark.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        unofficial::enet::enet
        lz4::lz4
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 winmm)
endif()
