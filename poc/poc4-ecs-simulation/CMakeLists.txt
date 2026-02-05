cmake_minimum_required(VERSION 3.21)

project(poc4_ecs_simulation
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard (required by EnTT)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(EnTT CONFIG REQUIRED)

# Source files
set(POC4_SOURCES
    main.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${POC4_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        EnTT::EnTT
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/poc/poc4-ecs-simulation"
)
