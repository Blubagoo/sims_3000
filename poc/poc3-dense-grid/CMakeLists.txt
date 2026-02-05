cmake_minimum_required(VERSION 3.21)

project(poc3_dense_grid
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(lz4 REQUIRED CONFIG)

# Source files
set(POC3_SOURCES
    main.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${POC3_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        lz4::lz4
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/poc/poc3-dense-grid"
)
