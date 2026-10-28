cmake_minimum_required(VERSION 3.22)

project(GraphicsFramework LANGUAGES C CXX)

# GraphicsFramework root CMakeLists.txt - Contains configuration for the main project

# Set configurations
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug_OpenGL;Release_OpenGL;Debug_Vulkan;Release_Vulkan" CACHE STRING "Supported GraphicsFramework configurations" FORCE)
    foreach(configuration IN ITEMS Debug_OpenGL Debug_Vulkan)
        string(TOUPPER "${configuration}" configuration_upper)
        set(CMAKE_C_FLAGS_${configuration_upper} "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "" FORCE)
        set(CMAKE_CXX_FLAGS_${configuration_upper} "${CMAKE_CXX_FLAGS_DEBUG}" CACHE STRING "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_${configuration_upper} "${CMAKE_EXE_LINKER_FLAGS_DEBUG}" CACHE STRING "" FORCE)
        set(CMAKE_MAP_IMPORTED_CONFIG_${configuration_upper} ";Debug" CACHE STRING "" FORCE)
    endforeach()
    foreach(configuration IN ITEMS Release_OpenGL Release_Vulkan)
        string(TOUPPER "${configuration}" configuration_upper)
        set(CMAKE_C_FLAGS_${configuration_upper} "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)
        set(CMAKE_CXX_FLAGS_${configuration_upper} "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_${configuration_upper} "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
        set(CMAKE_MAP_IMPORTED_CONFIG_${configuration_upper} ";Release" CACHE STRING "" FORCE)
    endforeach()
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GF_BUILD_SAMPLES "Add the existing sample application and test targets" ON)
option(GF_BUILD_DEBUG_PROFILER "Enable CPU/GPU profiler" ON)

# Validate platform
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "GraphicsFramework currently requires 64-bit Windows and MSVC.")
endif()

# Set project directories
set(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Add the GraphicsFramework library
add_subdirectory("${GF_PROJECT_DIR}/GraphicsFramework" "${CMAKE_BINARY_DIR}/GraphicsFramework")

# Set output directories for the library
set_target_properties(GraphicsFramework PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Add the library to the main project
add_library(GraphicsFramework::GraphicsFramework ALIAS GraphicsFramework)

# ---------------------------------------------------------------------------
# SPIR-V generation
#
# Shaders are loaded through ShaderRegistry, which resolves "spirv/<name>.spv"
# relative to the shader base directory the app registers ("res/" or
# "res/shaders/"). The compiled SPIR-V therefore has to sit in a "spirv/" folder
# next to the GLSL it was built from:
#
#   res/ShaderTest.vert       -> res/spirv/ShaderTest.vert.spv
#   res/shaders/Drawing.vert  -> res/shaders/spirv/Drawing.vert.spv
#
# Shaders under res/Vulkan/ are Vulkan-specific variants of the equally named
# shader in res/ (they use gl_VertexIndex instead of gl_VertexID). They take
# precedence over the base file and always land in res/spirv/.
#
# Note that custom commands are not configuration-scoped the way compile rules
# are, so when glslc is available the shaders are compiled in every
# configuration. Only the Vulkan backend reads the result; without the Vulkan
# SDK nothing is generated and the other backends are unaffected.
# ---------------------------------------------------------------------------
function(add_framework_spirv target_name res_dir)
    # glslc ships with the Vulkan SDK; without it there is nothing to generate.
    find_program(GF_GLSLC glslc
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
        DOC "Path to glslc, the SPIR-V compiler shipped with the Vulkan SDK")
    if(NOT GF_GLSLC)
        message(STATUS "glslc not found - no SPIR-V generated for ${target_name}")
        return()
    endif()

    set(shader_globs
        "${res_dir}/*.vert" "${res_dir}/*.frag" "${res_dir}/*.comp"
        "${res_dir}/*.geom" "${res_dir}/*.tesc" "${res_dir}/*.tese")

    # Vulkan variants first: they override the shader of the same name.
    set(vulkan_overrides "")
    foreach(pattern IN LISTS shader_globs)
        string(REPLACE "${res_dir}/" "${res_dir}/Vulkan/" nested_pattern "${pattern}")
        file(GLOB matched CONFIGURE_DEPENDS "${nested_pattern}")
        list(APPEND vulkan_overrides ${matched})
    endforeach()

    set(override_names "")
    foreach(shader IN LISTS vulkan_overrides)
        get_filename_component(shader_name "${shader}" NAME)
        list(APPEND override_names "${shader_name}")
    endforeach()

    set(shader_sources "")
    foreach(pattern IN LISTS shader_globs)
        file(GLOB_RECURSE matched CONFIGURE_DEPENDS "${pattern}")
        list(APPEND shader_sources ${matched})
    endforeach()

    set(spirv_outputs "")
    foreach(shader IN LISTS shader_sources)
        get_filename_component(shader_name "${shader}" NAME)
        get_filename_component(shader_dir "${shader}" DIRECTORY)

        # A res/Vulkan/<name> variant replaces res/<name>; skip the base file.
        if(shader_dir STREQUAL "${res_dir}/Vulkan")
            continue()
        endif()
        if(shader_name IN_LIST override_names)
            continue()
        endif()

        set(output "${shader_dir}/spirv/${shader_name}.spv")
        list(APPEND spirv_outputs "${output}")
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${shader_dir}/spirv"
            COMMAND "${GF_GLSLC}" "${shader}" -o "${output}" --target-env=vulkan1.2
            DEPENDS "${shader}"
            COMMENT "[SPIR-V] ${shader_name}"
            VERBATIM
        )
    endforeach()

    foreach(shader IN LISTS vulkan_overrides)
        get_filename_component(shader_name "${shader}" NAME)

        set(output "${res_dir}/spirv/${shader_name}.spv")
        list(APPEND spirv_outputs "${output}")
        add_custom_command(
            OUTPUT "${output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${res_dir}/spirv"
            COMMAND "${GF_GLSLC}" "${shader}" -o "${output}" --target-env=vulkan1.2
            DEPENDS "${shader}"
            COMMENT "[SPIR-V] Vulkan/${shader_name}"
            VERBATIM
        )
    endforeach()

    if(spirv_outputs)
        target_sources(${target_name} PRIVATE ${spirv_outputs})
    endif()
endfunction()

# Define a function to add framework apps
function(add_framework_app target_name source_dir)
    file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS "${source_dir}/*.cpp")
    if(NOT app_sources)
        message(FATAL_ERROR "No C++ sources found for ${target_name} in ${source_dir}")
    endif()

    add_executable(${target_name} EXCLUDE_FROM_ALL ${app_sources})
    target_link_libraries(${target_name} PRIVATE GraphicsFramework SDL2::SDL2main)
    
    # Copy the res folder to output directory (UnitTests has res/ alongside sources, others have res/ alongside src/)
    if(EXISTS "${source_dir}/res")
        set(REsources_DIR "${source_dir}/res")
    elseif(EXISTS "${source_dir}/../res")
        set(REsources_DIR "${source_dir}/../res")
    else()
        set(REsources_DIR "" CACHE STRING "No res directory found for ${target_name}" FORCE)
    endif()
    if(REsources_DIR)
        add_framework_spirv(${target_name} "${REsources_DIR}")

        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${REsources_DIR}"
                "$<TARGET_FILE_DIR:${target_name}>/res"
            VERBATIM
        )
    endif()
    
    # Copy runtime DLLs (if DLLs directory exists)
    if(EXISTS "${GF_PROJECT_DIR}/DLLs")
        file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
        foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
            add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${runtime_dll}" "$<TARGET_FILE_DIR:${target_name}>"
                VERBATIM
            )
        endforeach()
    endif()
endfunction()

# Add sample applications if enabled
if(GF_BUILD_SAMPLES)
    add_framework_app(FlockingParticles "${GF_PROJECT_DIR}/FlockingParticles/src")
    add_framework_app(AdvancedRenderingTechniques "${GF_PROJECT_DIR}/AdvancedRenderingTechniques/src")
    add_framework_app(AnimationTechniques "${GF_PROJECT_DIR}/AnimationTechniques/src")
    add_framework_app(RealTimeRayTracing "${GF_PROJECT_DIR}/RealTimeRayTracing/src")
    add_framework_app(WaveSimulation "${GF_PROJECT_DIR}/WaveSimulation/src")
    add_framework_app(UnitTests "${GF_PROJECT_DIR}/UnitTests")
endif()

message(STATUS "Build sample applications: ${GF_BUILD_SAMPLES}")
