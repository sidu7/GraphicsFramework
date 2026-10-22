cmake_minimum_required(VERSION 3.22)

project(GraphicsFramework LANGUAGES C CXX)

# Main CMakeLists.txt - Minimal configuration that includes subdirectories

# Include the GraphicsFramework directory
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/GraphicsFramework" "${CMAKE_BINARY_DIR}/GraphicsFramework" EXCLUDE_FROM_ALL)
