# ShaderCompilation.cmake
# CMake module for HLSL shader compilation to SPIR-V and DXIL
#
# Usage:
#   include(cmake/ShaderCompilation.cmake)
#   compile_shaders(
#       TARGET my_target
#       SHADERS
#           "shaders/toon.vert:vs_6_0"
#           "shaders/toon.frag:ps_6_0"
#       OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders"
#       SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders"
#   )
#
# This will:
#   1. Compile each HLSL shader to DXIL (for D3D12)
#   2. Compile each HLSL shader to SPIR-V (for Vulkan/Metal)
#   3. Add the compiled shaders as dependencies of the target
#   4. Copy compiled shaders to the target output directory
#
# For embedded fallback shaders:
#   generate_embedded_shaders(
#       OUTPUT_FILE "${CMAKE_BINARY_DIR}/generated/EmbeddedShaders.cpp"
#       SHADERS
#           "fallback.vert:vs_6_0"
#           "fallback.frag:ps_6_0"
#       SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders"
#   )

# Find DXC shader compiler from Windows SDK
function(find_dxc_compiler)
    if(DXC_EXECUTABLE)
        return()
    endif()

    # Common Windows SDK paths
    set(SDK_PATHS
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22000.0/x64"
        "C:/Program Files (x86)/Windows Kits/10/bin/10.0.19041.0/x64"
    )

    find_program(DXC_EXECUTABLE dxc
        PATHS ${SDK_PATHS}
        DOC "DirectX Shader Compiler (dxc)"
    )

    if(DXC_EXECUTABLE)
        message(STATUS "Found DXC: ${DXC_EXECUTABLE}")
    else()
        message(FATAL_ERROR
            "DXC (DirectX Shader Compiler) not found.\n"
            "Install the Windows SDK from: https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/\n"
            "Or set DXC_EXECUTABLE to the path of dxc.exe"
        )
    endif()

    set(DXC_EXECUTABLE ${DXC_EXECUTABLE} PARENT_SCOPE)
endfunction()

# Check if DXC supports SPIR-V codegen
function(check_dxc_spirv_support RESULT_VAR)
    if(NOT DXC_EXECUTABLE)
        find_dxc_compiler()
    endif()

    # Try to compile a trivial shader to SPIR-V
    set(TEST_SHADER "${CMAKE_CURRENT_BINARY_DIR}/dxc_spirv_test.hlsl")
    file(WRITE "${TEST_SHADER}" "float4 main() : SV_Target { return float4(1,0,1,1); }")

    execute_process(
        COMMAND "${DXC_EXECUTABLE}" -T ps_6_0 -E main -spirv -Fo "${CMAKE_CURRENT_BINARY_DIR}/dxc_spirv_test.spv" "${TEST_SHADER}"
        RESULT_VARIABLE SPIRV_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    file(REMOVE "${TEST_SHADER}" "${CMAKE_CURRENT_BINARY_DIR}/dxc_spirv_test.spv")

    if(SPIRV_RESULT EQUAL 0)
        set(${RESULT_VAR} TRUE PARENT_SCOPE)
        message(STATUS "DXC SPIR-V codegen: available")
    else()
        set(${RESULT_VAR} FALSE PARENT_SCOPE)
        message(STATUS "DXC SPIR-V codegen: not available (Windows SDK DXC)")
    endif()
endfunction()

# Compile a single shader to DXIL (and optionally SPIRV if supported)
function(compile_shader)
    cmake_parse_arguments(SHADER "" "SOURCE;NAME;PROFILE;OUTPUT_DIR" "" ${ARGN})

    if(NOT DXC_EXECUTABLE)
        find_dxc_compiler()
    endif()

    # Check SPIR-V support once
    if(NOT DEFINED DXC_HAS_SPIRV)
        check_dxc_spirv_support(DXC_HAS_SPIRV)
        set(DXC_HAS_SPIRV ${DXC_HAS_SPIRV} CACHE INTERNAL "DXC SPIR-V support")
    endif()

    set(HLSL_SOURCE "${SHADER_SOURCE}")
    set(DXIL_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.dxil")
    set(SPIRV_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")

    # Compile to DXIL (D3D12 native format) - always available
    add_custom_command(
        OUTPUT "${DXIL_OUTPUT}"
        COMMAND "${DXC_EXECUTABLE}"
            -T ${SHADER_PROFILE}
            -E main
            -Fo "${DXIL_OUTPUT}"
            "${HLSL_SOURCE}"
        DEPENDS "${HLSL_SOURCE}"
        COMMENT "Compiling ${SHADER_NAME}.hlsl -> DXIL"
        VERBATIM
    )

    # Compile to SPIR-V only if DXC supports it
    if(DXC_HAS_SPIRV)
        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND "${DXC_EXECUTABLE}"
                -T ${SHADER_PROFILE}
                -E main
                -spirv
                -fspv-target-env=vulkan1.1
                -Fo "${SPIRV_OUTPUT}"
                "${HLSL_SOURCE}"
            DEPENDS "${HLSL_SOURCE}"
            COMMENT "Compiling ${SHADER_NAME}.hlsl -> SPIR-V"
            VERBATIM
        )
        set(SHADER_SPIRV_OUTPUT "${SPIRV_OUTPUT}" PARENT_SCOPE)
    else()
        # Create a placeholder SPIR-V file with a single zero byte
        # This allows the build to proceed on Windows SDK DXC
        add_custom_command(
            OUTPUT "${SPIRV_OUTPUT}"
            COMMAND ${CMAKE_COMMAND} -E echo "0" > "${SPIRV_OUTPUT}"
            DEPENDS "${DXIL_OUTPUT}"
            COMMENT "Creating placeholder ${SHADER_NAME}.spv (SPIR-V not available)"
            VERBATIM
        )
        set(SHADER_SPIRV_OUTPUT "${SPIRV_OUTPUT}" PARENT_SCOPE)
    endif()

    # Return output files via parent scope
    set(SHADER_DXIL_OUTPUT "${DXIL_OUTPUT}" PARENT_SCOPE)
endfunction()

# Compile multiple shaders and add as target dependency
function(compile_shaders)
    cmake_parse_arguments(CS "" "TARGET;OUTPUT_DIR;SOURCE_DIR" "SHADERS" ${ARGN})

    if(NOT CS_TARGET)
        message(FATAL_ERROR "compile_shaders: TARGET is required")
    endif()

    if(NOT CS_OUTPUT_DIR)
        set(CS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    endif()

    if(NOT CS_SOURCE_DIR)
        set(CS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
    endif()

    file(MAKE_DIRECTORY "${CS_OUTPUT_DIR}")

    set(ALL_COMPILED_SHADERS "")

    foreach(SHADER_ENTRY ${CS_SHADERS})
        # Parse "name:profile" format
        string(REPLACE ":" ";" SHADER_PARTS "${SHADER_ENTRY}")
        list(GET SHADER_PARTS 0 SHADER_NAME)
        list(GET SHADER_PARTS 1 SHADER_PROFILE)

        set(HLSL_SOURCE "${CS_SOURCE_DIR}/${SHADER_NAME}.hlsl")

        compile_shader(
            SOURCE "${HLSL_SOURCE}"
            NAME "${SHADER_NAME}"
            PROFILE "${SHADER_PROFILE}"
            OUTPUT_DIR "${CS_OUTPUT_DIR}"
        )

        list(APPEND ALL_COMPILED_SHADERS "${SHADER_DXIL_OUTPUT}" "${SHADER_SPIRV_OUTPUT}")
    endforeach()

    # Create a custom target for all shaders
    set(SHADER_TARGET "${CS_TARGET}_shaders")
    add_custom_target(${SHADER_TARGET} DEPENDS ${ALL_COMPILED_SHADERS})

    # Add dependency to main target
    add_dependencies(${CS_TARGET} ${SHADER_TARGET})

    # Copy shaders to output directory post-build
    add_custom_command(TARGET ${CS_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CS_OUTPUT_DIR}"
            "$<TARGET_FILE_DIR:${CS_TARGET}>/shaders"
        COMMENT "Copying compiled shaders to output directory"
    )
endfunction()

# Generate C++ source file with embedded shader bytecode
function(generate_embedded_shaders)
    cmake_parse_arguments(ES "" "OUTPUT_FILE;SOURCE_DIR" "SHADERS" ${ARGN})

    if(NOT ES_OUTPUT_FILE)
        message(FATAL_ERROR "generate_embedded_shaders: OUTPUT_FILE is required")
    endif()

    if(NOT ES_SOURCE_DIR)
        set(ES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
    endif()

    if(NOT DXC_EXECUTABLE)
        find_dxc_compiler()
    endif()

    # Create output directory
    get_filename_component(OUTPUT_DIR "${ES_OUTPUT_FILE}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    # Temporary directory for compiled shaders
    set(TEMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/embedded_shaders_temp")
    file(MAKE_DIRECTORY "${TEMP_DIR}")

    set(ALL_SHADER_OUTPUTS "")
    set(SHADER_NAMES "")
    set(SHADER_STAGES "")

    foreach(SHADER_ENTRY ${ES_SHADERS})
        # Parse "name:profile" format
        string(REPLACE ":" ";" SHADER_PARTS "${SHADER_ENTRY}")
        list(GET SHADER_PARTS 0 SHADER_NAME)
        list(GET SHADER_PARTS 1 SHADER_PROFILE)

        set(HLSL_SOURCE "${ES_SOURCE_DIR}/${SHADER_NAME}.hlsl")

        compile_shader(
            SOURCE "${HLSL_SOURCE}"
            NAME "${SHADER_NAME}"
            PROFILE "${SHADER_PROFILE}"
            OUTPUT_DIR "${TEMP_DIR}"
        )

        list(APPEND ALL_SHADER_OUTPUTS "${SHADER_DXIL_OUTPUT}" "${SHADER_SPIRV_OUTPUT}")
        list(APPEND SHADER_NAMES "${SHADER_NAME}")

        # Determine stage from profile
        if(SHADER_PROFILE MATCHES "vs_")
            list(APPEND SHADER_STAGES "Vertex")
        elseif(SHADER_PROFILE MATCHES "ps_")
            list(APPEND SHADER_STAGES "Fragment")
        else()
            message(FATAL_ERROR "Unknown shader profile: ${SHADER_PROFILE}")
        endif()
    endforeach()

    # Generate the C++ source file
    set(CPP_CONTENT
"// Generated file - DO NOT EDIT
// Embedded fallback shaders for graceful degradation

#include <cstdint>
#include <cstddef>

namespace sims3000 {
namespace embedded {

")

    # This is a placeholder - actual bytecode will be embedded at build time
    # We'll use a custom command to generate the final file
    set(GENERATOR_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/generate_embedded.cmake")

    # Create the generator script
    file(WRITE "${GENERATOR_SCRIPT}"
"# Auto-generated shader embedding script
cmake_minimum_required(VERSION 3.21)

function(read_binary_file FILE_PATH VAR_NAME)
    file(READ \"\${FILE_PATH}\" FILE_CONTENT HEX)
    string(LENGTH \"\${FILE_CONTENT}\" CONTENT_LEN)
    math(EXPR BYTE_COUNT \"\${CONTENT_LEN} / 2\")

    set(BYTE_ARRAY \"\")
    set(LINE_BYTES 0)

    math(EXPR LAST_BYTE \"\${BYTE_COUNT} - 1\")
    foreach(I RANGE 0 \${LAST_BYTE})
        math(EXPR START \"\${I} * 2\")
        string(SUBSTRING \"\${FILE_CONTENT}\" \${START} 2 BYTE_HEX)
        if(LINE_BYTES EQUAL 0)
            string(APPEND BYTE_ARRAY \"    \")
        endif()
        string(APPEND BYTE_ARRAY \"0x\${BYTE_HEX}\")
        if(I LESS \${LAST_BYTE})
            string(APPEND BYTE_ARRAY \", \")
        endif()
        math(EXPR LINE_BYTES \"\${LINE_BYTES} + 1\")
        if(LINE_BYTES EQUAL 16)
            string(APPEND BYTE_ARRAY \"\\n\")
            set(LINE_BYTES 0)
        endif()
    endforeach()

    set(\${VAR_NAME} \"\${BYTE_ARRAY}\" PARENT_SCOPE)
    set(\${VAR_NAME}_SIZE \${BYTE_COUNT} PARENT_SCOPE)
endfunction()

# Read all shader bytecode files
")

    list(LENGTH SHADER_NAMES SHADER_COUNT)
    math(EXPR LAST_SHADER "${SHADER_COUNT} - 1")

    foreach(I RANGE 0 ${LAST_SHADER})
        list(GET SHADER_NAMES ${I} NAME)
        list(GET SHADER_STAGES ${I} STAGE)

        # Sanitize name for C++ identifier (replace . with _)
        string(REPLACE "." "_" SAFE_NAME "${NAME}")

        file(APPEND "${GENERATOR_SCRIPT}"
"read_binary_file(\"${TEMP_DIR}/${NAME}.dxil\" ${SAFE_NAME}_DXIL)
read_binary_file(\"${TEMP_DIR}/${NAME}.spv\" ${SAFE_NAME}_SPIRV)
")
    endforeach()

    # Write the C++ file generation
    file(APPEND "${GENERATOR_SCRIPT}"
"
# Generate C++ file
set(CPP_CONTENT \"// Generated file - DO NOT EDIT
// Embedded fallback shaders for graceful degradation
// Generated at configure time, contains precompiled shader bytecode
//
// Note: On Windows with SDK DXC, SPIR-V shaders may be placeholders.
// The runtime will use DXIL on D3D12 backends.

#include <cstdint>
#include <cstddef>

namespace sims3000 {
namespace embedded {

\")

")

    foreach(I RANGE 0 ${LAST_SHADER})
        list(GET SHADER_NAMES ${I} NAME)

        # Sanitize name for C++ identifier (replace . with _)
        string(REPLACE "." "_" SAFE_NAME "${NAME}")

        file(APPEND "${GENERATOR_SCRIPT}"
"string(APPEND CPP_CONTENT \"
// ${NAME} - DXIL format
static const uint8_t ${SAFE_NAME}_dxil[] = {
\${${SAFE_NAME}_DXIL}
};
static const size_t ${SAFE_NAME}_dxil_size = \${${SAFE_NAME}_DXIL_SIZE};

// ${NAME} - SPIR-V format
static const uint8_t ${SAFE_NAME}_spirv[] = {
\${${SAFE_NAME}_SPIRV}
};
static const size_t ${SAFE_NAME}_spirv_size = \${${SAFE_NAME}_SPIRV_SIZE};

\")
")
    endforeach()

    file(APPEND "${GENERATOR_SCRIPT}"
"
string(APPEND CPP_CONTENT \"
} // namespace embedded

const uint8_t* getEmbeddedFallbackVertexDXIL(size_t& outSize) {
    outSize = embedded::fallback_vert_dxil_size;
    return embedded::fallback_vert_dxil;
}

const uint8_t* getEmbeddedFallbackVertexSPIRV(size_t& outSize) {
    outSize = embedded::fallback_vert_spirv_size;
    return embedded::fallback_vert_spirv;
}

const uint8_t* getEmbeddedFallbackFragmentDXIL(size_t& outSize) {
    outSize = embedded::fallback_frag_dxil_size;
    return embedded::fallback_frag_dxil;
}

const uint8_t* getEmbeddedFallbackFragmentSPIRV(size_t& outSize) {
    outSize = embedded::fallback_frag_spirv_size;
    return embedded::fallback_frag_spirv;
}

} // namespace sims3000
\")

file(WRITE \"${ES_OUTPUT_FILE}\" \"\${CPP_CONTENT}\")
message(STATUS \"Generated embedded shaders: ${ES_OUTPUT_FILE}\")
")

    # Add custom command to generate the embedded shaders
    add_custom_command(
        OUTPUT "${ES_OUTPUT_FILE}"
        COMMAND ${CMAKE_COMMAND} -P "${GENERATOR_SCRIPT}"
        DEPENDS ${ALL_SHADER_OUTPUTS}
        COMMENT "Generating embedded shader source file"
        VERBATIM
    )

    # Create a target for the embedded shaders
    add_custom_target(embedded_shaders DEPENDS "${ES_OUTPUT_FILE}")
endfunction()
