cmake_minimum_required(VERSION 3.21)

project(poc5_transport_graph
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(POC5_SOURCES
    main.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${POC5_SOURCES}
)

# Include directories (header-only implementation)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/poc/poc-5-transport-graph"
)
