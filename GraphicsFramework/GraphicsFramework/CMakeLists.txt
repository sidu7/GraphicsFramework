# GraphicsFramework CMakeLists.txt - Contains configuration for the GraphicsFramework library
# (C++ standard, configurations, and platform validation are set in the root CMakeLists.txt)

# Set project directories
set(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source")

# Include ThirdParty subdirectory
add_subdirectory("${GF_PROJECT_DIR}/ThirdParty" "${CMAKE_BINARY_DIR}/ThirdParty" EXCLUDE_FROM_ALL)

# Include Source subdirectory (defines GraphicsFramework library with all ThirdParty include paths)
add_subdirectory("${GF_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/GraphicsFramework_Binary/Source")
