cmake_minimum_required(VERSION 3.22)

# AdvancedRenderingTechniques CMakeLists.txt - Contains configuration for the AdvancedRenderingTechniques application

# Set project name
project(AdvancedRenderingTechniques LANGUAGES CXX)

# Get the parent directory
get_filename_component(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Define source files
file(GLOB_RECURSE ADVANCED_RENDERING_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create executable
add_executable(AdvancedRenderingTechniques EXCLUDE_FROM_ALL ${ADVANCED_RENDERING_SOURCES})

# Link with GraphicsFramework
target_link_libraries(AdvancedRenderingTechniques PRIVATE GraphicsFramework SDL2::SDL2main)

# Set include directories
target_include_directories(AdvancedRenderingTechniques PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Core"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/OpenGL"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/Vulkan"
)

# Copy the entire res folder to output directory
add_custom_command(TARGET AdvancedRenderingTechniques POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../res"
        "$<TARGET_FILE_DIR:AdvancedRenderingTechniques>/res"
    VERBATIM
)

# Copy runtime DLLs
file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
    add_custom_command(TARGET AdvancedRenderingTechniques POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${runtime_dll}" "$<TARGET_FILE_DIR:AdvancedRenderingTechniques>"
        VERBATIM
    )
endforeach()

# ── SPIR-V Compilation (Vulkan builds only) ─────────────────────────
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GLSLC_ARCH x64)
else()
    set(GLSLC_ARCH x86)
endif()
set(GLSLC "$ENV{VULKAN_SDK}/Bin/${GLSLC_ARCH}/glslc.exe" CACHE STRING "Path to glslc compiler")

# Collect all GLSL shader files in res/shaders/
file(GLOB GLSL_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/*.vert"
                       "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/*.frag"
                       "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/*.compute")

# Also collect motionblur shaders
file(GLOB MOTIONBLUR_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/res/motionblur/*.vert"
                             "${CMAKE_CURRENT_SOURCE_DIR}/res/motionblur/*.frag")

list(APPEND ALL_SHADERS ${GLSL_SHADERS} ${MOTIONBLUR_SHADERS})

# Create spirv output directories
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/spirv")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/motionblur/spirv")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/res/Vulkan")

# Compile each shader to SPIR-V
foreach(shader IN LISTS ALL_SHADERS)
    get_filename_component(shader_name "${shader}" NAME)
    get_filename_component(shader_dir "${shader}" ABSOLUTE)

    # Determine output directory based on source location
    if("${shader}" MATCHES "res/motionblur/")
        set(spirv_subdir "motionblur")
    else()
        set(spirv_subdir "")
    endif()

    if(shader_name MATCHES "\\.(vert|frag|compute)$")
        if(shader_dir MATCHES "res/shaders/|res/Vulkan/")
            set(output_dir "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/spirv")
        endif()
        if(shader_dir MATCHES "res/motionblur/")
            set(output_dir "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/motionblur/spirv")
        endif()

        set(output_path "${output_dir}/${shader_name}.spv")

        add_custom_command(
            OUTPUT "${output_path}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
            COMMAND ${GLSLC} "${shader}" -o "${output_path}" --target-env=vulkan1.2
            DEPENDS "${shader}"
            COMMENT "[SPIR-V] Compiling ${shader_name} -> ${shader_name}.spv"
            VERBATIM
        )
        list(APPEND SPIRV_OUTPUTS "${output_path}")
    endif()
endforeach()

add_custom_target(CompileShadersToSPIRV DEPENDS ${SPIRV_OUTPUTS})

# Ensure shaders are compiled before linking (Vulkan builds only)
set_target_properties(AdvancedRenderingTechniques PROPERTIES
    ADDITIONAL_CLEAN_FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/shaders/spirv"
)
add_dependencies(AdvancedRenderingTechniques CompileShadersToSPIRV)
