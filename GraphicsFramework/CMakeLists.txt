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

# Define a function to add framework apps
file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
function(add_framework_app target_name source_dir)
    file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS "${source_dir}/*.cpp")
    if(NOT app_sources)
        message(FATAL_ERROR "No C++ sources found for ${target_name} in ${source_dir}")
    endif()

    add_executable(${target_name} EXCLUDE_FROM_ALL ${app_sources})
    target_link_libraries(${target_name} PRIVATE GraphicsFramework SDL2::SDL2main)
    
    # Copy the entire res folder to output directory
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${source_dir}/../res"
            "$<TARGET_FILE_DIR:${target_name}>/res"
        VERBATIM
    )
    
    # Copy runtime DLLs
    foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dll}" "$<TARGET_FILE_DIR:${target_name}>"
            VERBATIM
        )
    endforeach()
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
