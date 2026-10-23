cmake_minimum_required(VERSION 3.22)

# GraphicsFramework CMakeLists.txt - Contains configuration for the GraphicsFramework library

# Set project name
project(GraphicsFramework LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# Validate platform
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "GraphicsFramework currently requires 64-bit Windows and MSVC.")
endif()

# Set project directories
set(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# Include ThirdParty subdirectory
add_subdirectory("${GF_PROJECT_DIR}/ThirdParty" "${CMAKE_BINARY_DIR}/ThirdParty" EXCLUDE_FROM_ALL)

# Collect all source files from the Source folder
file(GLOB_RECURSE GF_SOURCES
    "${GF_SOURCE_DIR}/*.cpp"
    "${GF_SOURCE_DIR}/*.h"
)

# Create the GraphicsFramework library
add_library(GraphicsFramework ${GF_SOURCES})

# Set include directories
target_include_directories(GraphicsFramework PUBLIC "${GF_SOURCE_DIR}")
