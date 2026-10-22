cmake_minimum_required(VERSION 3.22)

project(GraphicsFramework LANGUAGES C CXX)

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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GF_BUILD_SAMPLES "Add the existing sample application and test targets" ON)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "GraphicsFramework currently requires 64-bit Windows and MSVC.")
endif()

set(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/GraphicsFramework")
set(GF_THIRD_PARTY "${GF_PROJECT_DIR}/GraphicsFramework/ThirdParty")
set(GF_SOURCE_DIR "${GF_PROJECT_DIR}/GraphicsFramework/Source")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build third-party libraries statically" FORCE)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/assimp" "${CMAKE_BINARY_DIR}/ThirdParty/assimp" EXCLUDE_FROM_ALL)

set(SDL_SHARED OFF CACHE BOOL "Build SDL2 as a shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL2 as a static library" FORCE)
set(SDL_TEST OFF CACHE BOOL "Build SDL2 tests" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL2 test programs" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/SDL" "${CMAKE_BINARY_DIR}/ThirdParty/SDL" EXCLUDE_FROM_ALL)

set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "Build GLEW as a shared library" FORCE)
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "Build GLEW as a static library" FORCE)
set(ONLY_LIBS ON CACHE BOOL "Build only GLEW libraries" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/GLEW" "${CMAKE_BINARY_DIR}/ThirdParty/GLEW" EXCLUDE_FROM_ALL)

set(GLM_BUILD_LIBRARY OFF CACHE BOOL "Keep GLM header-only" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Build GLM tests" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/glm" "${CMAKE_BINARY_DIR}/ThirdParty/glm" EXCLUDE_FROM_ALL)

set(SPIRV_REFLECT_EXECUTABLE OFF CACHE BOOL "Build SPIRV-Reflect executable" FORCE)
set(SPIRV_REFLECT_STATIC_LIB ON CACHE BOOL "Build SPIRV-Reflect statically" FORCE)
set(SPIRV_REFLECT_SHARED_LIB OFF CACHE BOOL "Build SPIRV-Reflect as a shared library" FORCE)
set(SPIRV_REFLECT_INSTALL OFF CACHE BOOL "Install SPIRV-Reflect" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/spirv_reflect" "${CMAKE_BINARY_DIR}/ThirdParty/spirv_reflect" EXCLUDE_FROM_ALL)

set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "Use Vulkan headers from the framework submodule" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "Install Volk" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/volk" "${CMAKE_BINARY_DIR}/ThirdParty/volk" EXCLUDE_FROM_ALL)

set(VMA_ENABLE_INSTALL OFF CACHE BOOL "Install Vulkan Memory Allocator" FORCE)
add_subdirectory("${GF_THIRD_PARTY}/vma" "${CMAKE_BINARY_DIR}/ThirdParty/vma" EXCLUDE_FROM_ALL)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "${GF_THIRD_PARTY}/rapidjson/include")

find_package(Vulkan REQUIRED)

add_library(vulkan_headers INTERFACE)
target_include_directories(vulkan_headers INTERFACE
	"${Vulkan_INCLUDE_DIRS}"
	"${GF_THIRD_PARTY}/vulkan/include"
	"${GF_THIRD_PARTY}/SPIRV-Headers/include"
)

target_include_directories(spirv-reflect-static PUBLIC "${GF_THIRD_PARTY}/SPIRV-Headers/include")
target_link_libraries(spirv-reflect-static PUBLIC vulkan_headers)
target_link_libraries(volk PUBLIC vulkan_headers)
target_link_libraries(VulkanMemoryAllocator INTERFACE vulkan_headers)

file(GLOB_RECURSE GF_COMMON_SOURCES CONFIGURE_DEPENDS
	"${GF_SOURCE_DIR}/*.cpp"
)
list(FILTER GF_COMMON_SOURCES EXCLUDE REGEX "/Rendering/(OpenGL|Vulkan)/.*\\.cpp$")

file(GLOB_RECURSE GF_OPENGL_SOURCES CONFIGURE_DEPENDS
	"${GF_SOURCE_DIR}/Rendering/OpenGL/*.cpp"
)
file(GLOB_RECURSE GF_VULKAN_SOURCES CONFIGURE_DEPENDS
	"${GF_SOURCE_DIR}/Rendering/Vulkan/*.cpp"
)

add_library(imgui STATIC
	"${GF_THIRD_PARTY}/Imgui/imgui.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_demo.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_draw.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_tables.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_widgets.cpp"
	"${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_sdl2.cpp"
)
target_sources(imgui PRIVATE
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_opengl3.cpp>"
	"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/Imgui/backends/imgui_impl_vulkan.cpp>"
)
target_include_directories(imgui PUBLIC
	"${GF_THIRD_PARTY}"
	"${GF_THIRD_PARTY}/Imgui"
	"${GF_THIRD_PARTY}/Imgui/backends"
	"${GF_THIRD_PARTY}/SDL/include"
	"${GF_THIRD_PARTY}/vulkan/include"
)
target_compile_definitions(imgui PRIVATE
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:IMGUI_IMPL_OPENGL_LOADER_GLEW>"
)
target_link_libraries(imgui PUBLIC
	SDL2::SDL2-static
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:libglew_static>"
)
target_link_libraries(imgui PUBLIC vulkan_headers)

add_library(stb_image STATIC "${GF_THIRD_PARTY}/stbi_image_impl.cpp")
target_include_directories(stb_image PUBLIC "${GF_THIRD_PARTY}/stbi_image")

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
target_include_directories(GraphicsFramework PUBLIC
	"${GF_SOURCE_DIR}"
	"${GF_THIRD_PARTY}"
	"${GF_THIRD_PARTY}/glm"
	"${GF_THIRD_PARTY}/vulkan/include"
	"${Vulkan_INCLUDE_DIRS}"
	"${GF_THIRD_PARTY}/Imgui"
	"${GF_THIRD_PARTY}/Imgui/backends"
	"${GF_THIRD_PARTY}/SDL/include"
	"${GF_THIRD_PARTY}/SPIRV-Headers/include"
	"${GF_THIRD_PARTY}/spirv_reflect"
	"${GF_THIRD_PARTY}/volk"
	"${GF_THIRD_PARTY}/vma/include"
	"${GF_THIRD_PARTY}/rapidjson/include"
	"${GF_THIRD_PARTY}/stbi_image"
)
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

file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
function(add_framework_app target_name source_dir)
	file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS "${source_dir}/*.cpp")
	if(NOT app_sources)
		message(FATAL_ERROR "No C++ sources found for ${target_name} in ${source_dir}")
	endif()

	add_executable(${target_name} EXCLUDE_FROM_ALL ${app_sources})
	target_link_libraries(${target_name} PRIVATE GraphicsFramework SDL2::SDL2main)
	
	# Copy the entire res folder to output directory to ensure resources are accessible
	# This ensures that files like Settings.json are available regardless of working directory
	add_custom_command(TARGET ${target_name} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${source_dir}/../res"
			"$<TARGET_FILE_DIR:${target_name}>/res"
		VERBATIM
	)
	
	foreach(runtime_dll IN LISTS GF_RUNTIME_DLLS)
		add_custom_command(TARGET ${target_name} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				"${runtime_dll}" "$<TARGET_FILE_DIR:${target_name}>"
			VERBATIM
		)
	endforeach()
endfunction()

if(GF_BUILD_SAMPLES)
	add_framework_app(FlockingParticles "${GF_PROJECT_DIR}/FlockingParticles/src")
	add_framework_app(AdvancedRenderingTechniques "${GF_PROJECT_DIR}/AdvancedRenderingTechniques/src")
	add_framework_app(AnimationTechniques "${GF_PROJECT_DIR}/AnimationTechniques/src")
	add_framework_app(RealTimeRayTracing "${GF_PROJECT_DIR}/RealTimeRayTracing/src")
	add_framework_app(WaveSimulation "${GF_PROJECT_DIR}/WaveSimulation/src")
	add_framework_app(UnitTests "${GF_PROJECT_DIR}/UnitTests")

endif()

message(STATUS "Build sample applications: ${GF_BUILD_SAMPLES}")
