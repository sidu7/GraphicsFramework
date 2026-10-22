cmake_minimum_required(VERSION 3.22)

# GraphicsFramework Source CMakeLists.txt - Contains all source code configurations

# Define source files
file(GLOB_RECURSE GF_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER GF_COMMON_SOURCES EXCLUDE REGEX "/Rendering/(OpenGL|Vulkan)/.*\\.cpp$")

file(GLOB_RECURSE GF_OPENGL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/OpenGL/*.cpp"
)
file(GLOB_RECURSE GF_VULKAN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Rendering/Vulkan/*.cpp"
)

# Create the main library
add_library(GraphicsFramework STATIC
    ${GF_COMMON_SOURCES}
)
foreach(source IN LISTS GF_OPENGL_SOURCES)
    target_sources(GraphicsFramework PRIVATE
        "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${source}>")
endforeach()
foreach(source IN LISTS GF_VULKAN_SOURCES)
    target_sources(GraphicsFramework PRIVATE
        "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${source}>")
endforeach()

# Set include directories
target_include_directories(GraphicsFramework PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/glm"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/vulkan/include"
    "${Vulkan_INCLUDE_DIRS}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/Imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/SDL/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/SPIRV-Headers/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/spirv_reflect"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/volk"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/vma/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/rapidjson/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/stbi_image"
)

# Set compile definitions
target_compile_definitions(GraphicsFramework PUBLIC
    SDL_MAIN_HANDLED
    _MBCS
    WINDOWS_IGNORE_PACKING_MISMATCH
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:RENDERER_OPENGL>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:RENDERER_VULKAN>"
)
target_compile_definitions(GraphicsFramework PRIVATE
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:GLEW_STATIC>"
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:IMGUI_IMPL_OPENGL_LOADER_GLEW>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:SPIRV_REFLECT_USE_SYSTEM_SPIRV_H>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:VK_ENABLE_BETA_EXTENSIONS>"
)

# Set library dependencies
target_link_libraries(GraphicsFramework PUBLIC
    assimp::assimp
    SDL2::SDL2-static
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:libglew_static>"
    glm::glm
    imgui
    rapidjson
    VulkanMemoryAllocator
    stb_image
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:spirv-reflect-static>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:volk>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${Vulkan_LIBRARY}>"
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:opengl32>"
)
target_link_libraries(GraphicsFramework PUBLIC vulkan_headers)
