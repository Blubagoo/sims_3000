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
