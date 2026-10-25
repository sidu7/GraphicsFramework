cmake_minimum_required(VERSION 3.22)

# UnitTests CMakeLists.txt - Contains configuration for the UnitTests application

# Set project name
project(UnitTests LANGUAGES CXX)

# Get the parent directory
get_filename_component(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Define source files
file(GLOB_RECURSE UNIT_TESTS_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create executable
add_executable(UnitTests EXCLUDE_FROM_ALL ${UNIT_TESTS_SOURCES})

# Link with GraphicsFramework
target_link_libraries(UnitTests PRIVATE GraphicsFramework SDL2::SDL2main)

# Set include directories
target_include_directories(UnitTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Core"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/OpenGL"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/Vulkan"
)

# SPIR-V compilation (Vulkan builds only)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(GLSLC_ARCH x64)
else()
    set(GLSLC_ARCH x86)
endif()
set(GLSLC "$ENV{VULKAN_SDK}/Bin/${GLSLC_ARCH}/glslc.exe" CACHE STRING "Path to glslc compiler")

# Create spirv output directory
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/spirv")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/spirv/Vulkan")

# Compile regular test shaders
file(GLOB UNITTEST_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/res/*.vert"
                          "${CMAKE_CURRENT_SOURCE_DIR}/res/*.frag")
foreach(shader IN LISTS UNITTEST_SHADERS)
    get_filename_component(shader_name "${shader}" NAME)

    set(output_path "${CMAKE_CURRENT_SOURCE_DIR}/spirv/${shader_name}.spv")

    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/spirv"
        COMMAND ${GLSLC} "${shader}" -o "${output_path}" --target-env=vulkan1.2
        DEPENDS "${shader}"
        COMMENT "[SPIR-V] Compiling ${shader_name} -> ${shader_name}.spv"
        VERBATIM
    )
    list(APPEND SPIRV_OUTPUTS "${output_path}")
endforeach()

# Compile Vulkan variant shaders (ShaderTest.vert in res/Vulkan/)
file(GLOB VULKAN_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/res/Vulkan/*.vert"
                        "${CMAKE_CURRENT_SOURCE_DIR}/res/Vulkan/*.frag")
foreach(shader IN LISTS VULKAN_SHADERS)
    get_filename_component(shader_name "${shader}" NAME)
    # Output to spirv/Vulkan/ subdirectory
    set(output_path "${CMAKE_CURRENT_SOURCE_DIR}/spirv/Vulkan/${shader_name}.spv")

    add_custom_command(
        OUTPUT "${output_path}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/spirv/Vulkan"
        COMMAND ${GLSLC} "${shader}" -o "${output_path}" --target-env=vulkan1.2
        DEPENDS "${shader}"
        COMMENT "[SPIR-V] Compiling Vulkan/${shader_name} -> Vulkan/${shader_name}.spv"
        VERBATIM
    )
    list(APPEND SPIRV_OUTPUTS "${output_path}")
endforeach()

add_custom_target(CompileUnitTestShadersToSPIRV DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(UnitTests CompileUnitTestShadersToSPIRV)
