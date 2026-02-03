cmake_minimum_required(VERSION 3.21)

project(sims_3000
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
