cmake_minimum_required(VERSION 3.22)

# GraphicsFramework Source CMakeLists.txt - Contains all source code configurations
#
# Targets:
#   GF_RHI_Common      INTERFACE  include directories + definitions shared by every target
#   GF_RHI_OpenGL      INTERFACE  OpenGL-only definitions/dependencies (selected by configuration)
#   GF_RHI_Vulkan      INTERFACE  Vulkan-only definitions/dependencies (selected by configuration)
#   GraphicsFramework  STATIC     all non-backend sources + exactly one backend's sources
#
# The backend is selected here at configure time. There is no runtime selector and no
# RENDERER_* #if chain in Renderer.cpp / Window.cpp: the backend translation unit that
# defines rhi::CreateDevice / rhi::CreateWindow is chosen by the configuration.
set(GF_THIRD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty")

# ---------------------------------------------------------------------------
# Configuration -> backend mapping
# ---------------------------------------------------------------------------
# Keep these lists in sync with the generator expressions below. Listing a
# configuration in neither list (or in both) is a build-system error, not a
# silent runtime fallback.
set(GF_OPENGL_CONFIGS Debug_OpenGL Release_OpenGL)
set(GF_VULKAN_CONFIGS Debug_Vulkan Release_Vulkan)

if(CMAKE_CONFIGURATION_TYPES)
    set(GF_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
elseif(CMAKE_BUILD_TYPE)
    set(GF_CONFIGS ${CMAKE_BUILD_TYPE})
else()
    message(FATAL_ERROR "No configuration selected: set CMAKE_CONFIGURATION_TYPES or CMAKE_BUILD_TYPE.")
endif()

foreach(config IN LISTS GF_CONFIGS)
    list(FIND GF_OPENGL_CONFIGS "${config}" gf_is_opengl)
    list(FIND GF_VULKAN_CONFIGS "${config}" gf_is_vulkan)

    if(gf_is_opengl GREATER -1 AND gf_is_vulkan GREATER -1)
        message(FATAL_ERROR "Configuration '${config}' selects both OpenGL and Vulkan. A configuration must map to exactly one RHI backend.")
    elseif(gf_is_opengl EQUAL -1 AND gf_is_vulkan EQUAL -1)
        message(FATAL_ERROR "Configuration '${config}' selects no RHI backend. Add it to GF_OPENGL_CONFIGS or GF_VULKAN_CONFIGS in Source/CMakeLists.txt.")
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Source lists
# ---------------------------------------------------------------------------
# Common sources: everything except the two backend trees.
file(GLOB_RECURSE GF_COMMON_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(FILTER GF_COMMON_SOURCES EXCLUDE REGEX "/RHI/(OpenGL|Vulkan)/.*\\.cpp$")

file(GLOB_RECURSE GF_OPENGL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/RHI/OpenGL/*.cpp"
)
file(GLOB_RECURSE GF_VULKAN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/RHI/Vulkan/*.cpp"
)

# ---------------------------------------------------------------------------
# Shared include directories and definitions
# ---------------------------------------------------------------------------
add_library(GF_RHI_Common INTERFACE)

target_include_directories(GF_RHI_Common INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${GF_THIRD_PARTY}"
    "${GF_THIRD_PARTY}/glm"
    "${GF_THIRD_PARTY}/Imgui"
    "${GF_THIRD_PARTY}/Imgui/backends"
    "${GF_THIRD_PARTY}/SDL/include"
    "${GF_THIRD_PARTY}/rapidjson/include"
    "${GF_THIRD_PARTY}/stbi_image"
)

target_compile_definitions(GF_RHI_Common INTERFACE
    SDL_MAIN_HANDLED
    _MBCS
    WINDOWS_IGNORE_PACKING_MISMATCH
)

# ---------------------------------------------------------------------------
# Backend targets (build requirements only)
# ---------------------------------------------------------------------------
# The backend's translation units are compiled as part of GraphicsFramework,
# selected per configuration (see target_sources below). These interface
# targets carry the API-specific definitions and dependencies so that they are
# only applied to the configuration that selects that backend.
#
# Why interface targets instead of STATIC/OBJECT libraries: with Ninja
# Multi-Config, CMake resolves target dependencies referenced inside a
# generator expression for *every* configuration, so a per-config
# $<TARGET_OBJECTS:GF_RHI_X> or $<CONFIG:...>:GF_RHI_X still causes the
# non-selected backend to be compiled. Interface targets have no build steps,
# so the same generator expressions are harmless there.
add_library(GF_RHI_OpenGL INTERFACE)
target_link_libraries(GF_RHI_OpenGL INTERFACE GF_RHI_Common)
target_compile_definitions(GF_RHI_OpenGL INTERFACE
    RENDERER_OPENGL
    GLEW_STATIC
    IMGUI_IMPL_OPENGL_LOADER_GLEW
)

add_library(GF_RHI_Vulkan INTERFACE)
target_link_libraries(GF_RHI_Vulkan INTERFACE GF_RHI_Common vulkan_headers)
target_include_directories(GF_RHI_Vulkan INTERFACE
    "${GF_THIRD_PARTY}/vulkan/include"
    "${Vulkan_INCLUDE_DIRS}"
    "${GF_THIRD_PARTY}/SPIRV-Headers/include"
    "${GF_THIRD_PARTY}/spirv_reflect"
    "${GF_THIRD_PARTY}/vma/include"
)
target_compile_definitions(GF_RHI_Vulkan INTERFACE
    RENDERER_VULKAN
    SPIRV_REFLECT_USE_SYSTEM_SPIRV_H
    VK_ENABLE_BETA_EXTENSIONS
)

# ---------------------------------------------------------------------------
# Framework library (backend-agnostic sources)
# ---------------------------------------------------------------------------
add_library(GraphicsFramework STATIC
    ${GF_COMMON_SOURCES}
)

# Only the backend belonging to this configuration is compiled into the
# library. A plain (non-generator-expression) source entry is filtered per
# configuration, which is what keeps the two backends from ever sharing a
# binary.
foreach(source IN LISTS GF_OPENGL_SOURCES)
    target_sources(GraphicsFramework PRIVATE
        "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${source}>")
endforeach()
foreach(source IN LISTS GF_VULKAN_SOURCES)
    target_sources(GraphicsFramework PRIVATE
        "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${source}>")
endforeach()

# Selecting the backend also enables its API-specific definitions/dependencies.
target_link_libraries(GraphicsFramework PRIVATE
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:GF_RHI_OpenGL>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:GF_RHI_Vulkan>"
)

target_link_libraries(GraphicsFramework PUBLIC GF_RHI_Common)

# Set compile definitions
target_compile_definitions(GraphicsFramework PUBLIC
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:RENDERER_OPENGL>"
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:RENDERER_VULKAN>"
)

# Profiler — GF_PROFILING is defined in Debug builds (always) and Release builds
# (only when GF_BUILD_DEBUG_PROFILER is ON).  All profiler code is wrapped in
# #ifdef GF_PROFILING so that when it is not defined the compiler eliminates every
# call, vtable, and data structure (zero overhead).
if(GF_BUILD_DEBUG_PROFILER)
    target_compile_definitions(GraphicsFramework PUBLIC
        $<$<CONFIG:Debug>:GF_PROFILING>
        $<$<CONFIG:Release>:GF_PROFILING>
    )
else()
    # Always on in Debug, off in Release
    target_compile_definitions(GraphicsFramework PUBLIC
        $<$<CONFIG:Debug>:GF_PROFILING>
    )
endif()

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
    # The Vulkan loader is linked directly (${Vulkan_LIBRARY}). volk is not used:
    # without a volkInitialize() call its vk* globals are null function pointers,
    # which shadow the loader imports (causing a crash on the first vk call).
    "$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${Vulkan_LIBRARY}>"
    "$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:opengl32>"
)

# Force x64 Windows SDK library search paths to take priority over x86.
# This is needed because SDL2-static links against simple library names like
# kernel32, user32, gdi32, etc., and CMake may resolve them to the wrong
# architecture if the x64 paths aren't searched first.
set(WINDOWS_KITS_ROOT "C:/Program Files (x86)/Windows Kits/10")
set(GF_X64_WINDOWS_LIBS "")
file(GLOB WINDOWS_SDK_VERSIONS "${WINDOWS_KITS_ROOT}/Lib/10.0.*")
foreach(_sdk_ver IN LISTS WINDOWS_SDK_VERSIONS)
    if(EXISTS "${_sdk_ver}/um/x64")
        list(APPEND GF_X64_WINDOWS_LIBS "${_sdk_ver}/um/x64")
    endif()
    if(EXISTS "${_sdk_ver}/ucrt/x64")
        list(APPEND GF_X64_WINDOWS_LIBS "${_sdk_ver}/ucrt/x64")
    endif()
endforeach()
list(REMOVE_DUPLICATES GF_X64_WINDOWS_LIBS)

# Also add x64 MSVC runtime library paths to override any x86 paths
set(MSVC_LIB_ROOT "C:/Program Files/Microsoft Visual Studio/18/Insiders/VC/Tools/MSVC/14.52.37025/lib")
if(EXISTS "${MSVC_LIB_ROOT}/x64")
    list(APPEND GF_X64_WINDOWS_LIBS "${MSVC_LIB_ROOT}/x64")
endif()

if(GF_X64_WINDOWS_LIBS)
    target_link_directories(GraphicsFramework PUBLIC "${GF_X64_WINDOWS_LIBS}")
    message(STATUS "Added x64 library paths: ${GF_X64_WINDOWS_LIBS}")
else()
    message(WARNING "Could not find x64 library paths")
endif()

# The backend sources are already part of the archive through target_sources
# above; no static-library back-edge is needed. Backend code can therefore call
# framework services (Engine, ObjectManager, ShaderRegistry, ...) and the
# framework can call rhi::CreateDevice / rhi::CreateWindow without a cycle.
