cmake_minimum_required(VERSION 3.22)

# RealTimeRayTracing CMakeLists.txt - Contains configuration for the RealTimeRayTracing application

# Set project name
project(RealTimeRayTracing LANGUAGES CXX)

# Get the parent directory
get_filename_component(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Define source files
file(GLOB_RECURSE RAY_TRACING_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create executable
add_executable(RealTimeRayTracing EXCLUDE_FROM_ALL ${RAY_TRACING_SOURCES})

# Link with GraphicsFramework
target_link_libraries(RealTimeRayTracing PRIVATE GraphicsFramework SDL2::SDL2main)

# Set include directories
target_include_directories(RealTimeRayTracing PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Core"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/OpenGL"
    "${GF_PROJECT_DIR}/GraphicsFramework/Source/Rendering/Vulkan"
)

# Copy the entire res folder to output directory
add_custom_command(TARGET RealTimeRayTracing POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/../res"
        "$<TARGET_FILE_DIR:RealTimeRayTracing>/res"
    VERBATIM
)

# Copy runtime DLLs
file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
    add_custom_command(TARGET RealTimeRayTracing POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${runtime_dll}" "$<TARGET_FILE_DIR:RealTimeRayTracing>"
        VERBATIM
    )
endforeach()
