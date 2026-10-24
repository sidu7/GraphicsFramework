cmake_minimum_required(VERSION 3.22)

# ThirdParty CMakeLists.txt - Contains all third-party library configurations

set(GF_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}")

# Set build options for third-party libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build third-party libraries statically" FORCE)

# Assimp configuration
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/assimp" "${CMAKE_BINARY_DIR}/ThirdParty/assimp" EXCLUDE_FROM_ALL)

# SDL configuration
set(SDL_SHARED OFF CACHE BOOL "Build SDL2 as a shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL2 as a static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL2 tests" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL2 test programs" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/SDL" "${CMAKE_BINARY_DIR}/ThirdParty/SDL" EXCLUDE_FROM_ALL)

# GLEW configuration
set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "Build GLEW as a shared library" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "Build GLEW as a static library" FORCE)
set(ONLY_LIBS ON CACHE BOOL "Build only GLEW libraries" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/GLEW" "${CMAKE_BINARY_DIR}/ThirdParty/GLEW" EXCLUDE_FROM_ALL)

# GLM configuration
set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Keep GLM header-only" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Build GLM tests" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/glm" "${CMAKE_BINARY_DIR}/ThirdParty/glm" EXCLUDE_FROM_ALL)

# SPIRV-Reflect configuration
set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "Build SPIRV-Reflect executable" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "Build SPIRV-Reflect statically" FORCE)
set(SPIRV_REFLECT_SHARED_LIB OFF CACHE BOOL "Build SPIRV-Reflect as a shared library" FORCE)
set(SPIRV_REFLECT_INSTALL OFF CACHE BOOL "Install SPIRV-Reflect" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/spirv_reflect" "${CMAKE_BINARY_DIR}/ThirdParty/spirv_reflect" EXCLUDE_FROM_ALL)

# Volk configuration
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "Use Vulkan headers from the framework submodule" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "Install Volk" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/volk" "${CMAKE_BINARY_DIR}/ThirdParty/volk" EXCLUDE_FROM_ALL)

# Vulkan Memory Allocator configuration
set(VMA_ENABLE_INSTALL OFF CACHE BOOL "Install Vulkan Memory Allocator" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/vma" "${CMAKE_BINARY_DIR}/ThirdParty/vma" EXCLUDE_FROM_ALL)

# RapidJSON
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "${GF_THIRD_PARTY}/rapidjson/include")

# Find Vulkan
find_package(Vulkan REQUIRED)

# Create Vulkan headers interface
add_library(vulkan_headers INTERFACE)
target_include_directories(vulkan_headers INTERFACE
    "${Vulkan_INCLUDE_DIRS}"
    "${GF_THIRD_PARTY}/vulkan/include"
    "${GF_THIRD_PARTY}/SPIRV-Headers/include"
)

# Set up target dependencies
target_link_libraries(spirv-reflect-static PUBLIC vulkan_headers)
target_link_libraries(volk PUBLIC vulkan_headers)
target_link_libraries(VulkanMemoryAllocator INTERFACE vulkan_headers)

# Imgui
add_library(imgui STATIC
    ${GF_THIRD_PARTY}/Imgui/imgui.cpp
    ${GF_THIRD_PARTY}/Imgui/imgui_draw.cpp
    ${GF_THIRD_PARTY}/Imgui/imgui_tables.cpp
    ${GF_THIRD_PARTY}/Imgui/imgui_widgets.cpp
    ${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_win32.cpp
    ${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_dx12.cpp
    $<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_sdl2.cpp>
    $<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_opengl3.cpp>
    $<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_sdl2.cpp>
    $<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_vulkan.cpp>
)

target_include_directories(imgui PUBLIC
    ${GF_THIRD_PARTY}/Imgui
    ${GF_THIRD_PARTY}/Imgui/backends
    ${GF_THIRD_PARTY}/SDL/include
)

# STB Image
add_library(stb_image STATIC ${GF_THIRD_PARTY}/stbi_image_impl.cpp)
target_include_directories(stb_image PUBLIC ${GF_THIRD_PARTY}/stbi_image)