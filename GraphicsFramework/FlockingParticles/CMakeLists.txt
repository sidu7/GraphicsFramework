cmake_minimum_required(VERSION 3.22)

# FlockingParticles CMakeLists.txt - Contains configuration for the FlockingParticles application

# Set project name
project(FlockingParticles LANGUAGES CXX)

# Get the parent directory
get_filename_component(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Define source files
file(GLOB_RECURSE FLOCKING_PARTICLES_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create executable
add_executable(FlockingParticles EXCLUDE_FROM_ALL ${FLOCKING_PARTICLES_SOURCES})

# Link with GraphicsFramework
target_link_libraries(FlockingParticles PRIVATE GraphicsFramework SDL2::SDL2main)

# Set include directories
target_include_directories(FlockingParticles PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Core"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/OpenGL"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/Vulkan"
)

# Copy the entire res folder to output directory
add_custom_command(TARGET FlockingParticles POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../res"
        "$<TARGET_FILE_DIR:FlockingParticles>/res"
    VERBATIM
)

# Copy runtime DLLs
file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
    add_custom_command(TARGET FlockingParticles POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${runtime_dll}" "$<TARGET_FILE_DIR:FlockingParticles>"
        VERBATIM
    )
endforeach()
