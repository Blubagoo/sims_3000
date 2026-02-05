cmake_minimum_required(VERSION 3.21)

project(poc1_toon_rendering
    VERSION 0.1.0
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(SDL3 REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)

# Find cgltf (header-only library from vcpkg)
find_path(CGLTF_INCLUDE_DIR cgltf.h)
if(NOT CGLTF_INCLUDE_DIR)
    message(FATAL_ERROR "cgltf.h not found. Ensure cgltf is installed via vcpkg.")
endif()

# Find DXC shader compiler from Windows SDK
find_program(DXC_EXECUTABLE dxc
    PATHS
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22000.0/x64"
    DOC "DirectX Shader Compiler"
)
if(NOT DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC (DirectX Shader Compiler) not found. Install the Windows SDK.")
endif()
message(STATUS "Found DXC: ${DXC_EXECUTABLE}")

# Shader compilation - compile HLSL to both SPIRV and DXIL at build time
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders_compiled)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# Define shaders: name, profile
set(SHADER_SOURCES
    "toon.vert:vs_6_0"
    "toon.frag:ps_6_0"
    "outline.vert:vs_6_0"
    "outline.frag:ps_6_0"
)

set(COMPILED_SHADERS "")
foreach(SHADER_ENTRY ${SHADER_SOURCES})
    # Parse "name:profile"
    string(REPLACE ":" ";" SHADER_PARTS ${SHADER_ENTRY})
    list(GET SHADER_PARTS 0 SHADER_NAME)
    list(GET SHADER_PARTS 1 SHADER_PROFILE)

    set(HLSL_SOURCE ${SHADER_SOURCE_DIR}/${SHADER_NAME}.hlsl)
    set(DXIL_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.dxil)

    # Compile HLSL -> DXIL (native format for D3D12)
    add_custom_command(
        OUTPUT ${DXIL_OUTPUT}
        COMMAND ${DXC_EXECUTABLE} -T ${SHADER_PROFILE} -E main
                ${HLSL_SOURCE} -Fo ${DXIL_OUTPUT}
        DEPENDS ${HLSL_SOURCE}
        COMMENT "Compiling ${SHADER_NAME}.hlsl -> DXIL"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${DXIL_OUTPUT})
endforeach()

# Custom target for shader compilation
add_custom_target(poc1_shaders DEPENDS ${COMPILED_SHADERS})

# Source files
set(POC1_SOURCES
    main.cpp
    src/gpu_device.cpp
    src/camera.cpp
    src/instance_buffer.cpp
    src/benchmark.cpp
    src/model_loader.cpp
    src/shader_loader.cpp
    src/gpu_mesh.cpp
    src/scene.cpp
    src/toon_pipeline.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${POC1_SOURCES}
)

# Depend on compiled shaders
add_dependencies(${PROJECT_NAME} poc1_shaders)

# Include directories for header-only libraries
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CGLTF_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SDL3::SDL3
        glm::glm
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/poc/poc1-toon-rendering"
)

# Copy compiled shaders to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_OUTPUT_DIR}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
