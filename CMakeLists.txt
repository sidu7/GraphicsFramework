cmake_minimum_required(VERSION 3.22)

project(GraphicsFramework LANGUAGES C CXX)

# Enable debug profiler (CPU + GPU timing, ImGui overlay, CSV export)
option(GF_BUILD_DEBUG_PROFILER "Enable CPU/GPU profiler (Debug builds: always on; Release builds: on if this option is ON)" ON)

# Main CMakeLists.txt - Minimal configuration that includes subdirectories

# Include the GraphicsFramework directory
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/GraphicsFramework" "${CMAKE_BINARY_DIR}/GraphicsFramework_Binary" EXCLUDE_FROM_ALL)
