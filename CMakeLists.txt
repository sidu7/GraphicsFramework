cmake_minimum_required(VERSION 3.20)

project(GraphicsFramework LANGUAGES C CXX)

set(CMAKE_CONFIGURATION_TYPES "Debug_OpenGL;Release_OpenGL;Debug_Vulkan;Release_Vulkan"
	CACHE STRING "Available renderer and build configurations" FORCE)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS "Debug_OpenGL;Debug_Vulkan")
set(CMAKE_MSVC_RUNTIME_LIBRARY
	"MultiThreaded$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Debug_Vulkan>>:Debug>DLL")
foreach(configuration IN ITEMS Debug_OpenGL Debug_Vulkan Release_OpenGL Release_Vulkan)
	string(TOUPPER "${configuration}" configuration_upper)
	if(configuration MATCHES "^Debug")
		set(base_configuration DEBUG)
	else()
		set(base_configuration RELEASE)
	endif()
	set(CMAKE_C_FLAGS_${configuration_upper} "${CMAKE_C_FLAGS_${base_configuration}}"
		CACHE STRING "C compiler flags for ${configuration}" FORCE)
	set(CMAKE_CXX_FLAGS_${configuration_upper} "${CMAKE_CXX_FLAGS_${base_configuration}}"
		CACHE STRING "C++ compiler flags for ${configuration}" FORCE)
	set(CMAKE_EXE_LINKER_FLAGS_${configuration_upper} "${CMAKE_EXE_LINKER_FLAGS_${base_configuration}}"
		CACHE STRING "Executable linker flags for ${configuration}" FORCE)
endforeach()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GF_BUILD_SAMPLES "Build the existing sample applications and tests" OFF)

if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(FATAL_ERROR "GraphicsFramework currently requires 64-bit Windows and MSVC; its bundled dependencies are Windows x64 libraries.")
endif()

set(GF_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/GraphicsFramework")
set(GF_THIRD_PARTY "${GF_PROJECT_DIR}/GraphicsFramework/ThirdParty")
set(GF_SOURCE_DIR "${GF_PROJECT_DIR}/GraphicsFramework/Source")
set(GF_SDL_LIB "${GF_THIRD_PARTY}/SDL/lib/SDL2.lib")
set(GF_SDL_MAIN_LIB "${GF_THIRD_PARTY}/SDL/lib/SDL2main.lib")
set(GF_ASSIMP_LIB "${GF_THIRD_PARTY}/assimp/lib/assimp-vc140-mt.lib")
set(GF_GLEW_LIB "${GF_THIRD_PARTY}/GLEW/lib/glew32s.lib")
set(GF_VULKAN_LIB "${GF_THIRD_PARTY}/vulkan/lib/vulkan-1.lib")

foreach(required_file IN ITEMS
		"${GF_SDL_LIB}"
		"${GF_SDL_MAIN_LIB}"
		"${GF_ASSIMP_LIB}"
		"${GF_GLEW_LIB}"
		"${GF_VULKAN_LIB}")
	if(NOT EXISTS "${required_file}")
		message(FATAL_ERROR "Required bundled library was not found: ${required_file}")
	endif()
endforeach()

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

file(GLOB GF_IMGUI_SOURCES CONFIGURE_DEPENDS
	"${GF_THIRD_PARTY}/Imgui/imgui.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_demo.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_draw.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_impl_sdl.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_tables.cpp"
	"${GF_THIRD_PARTY}/Imgui/imgui_widgets.cpp"
)

add_library(GraphicsFramework STATIC
	${GF_COMMON_SOURCES}
	${GF_IMGUI_SOURCES}
	"${GF_THIRD_PARTY}/stbi_image/stbi_image.cpp"
)
foreach(source IN LISTS GF_OPENGL_SOURCES)
	target_sources(GraphicsFramework PRIVATE
		"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${source}>")
endforeach()
foreach(source IN LISTS GF_VULKAN_SOURCES)
	target_sources(GraphicsFramework PRIVATE
		"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${source}>")
endforeach()
target_sources(GraphicsFramework PRIVATE
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${GF_THIRD_PARTY}/Imgui/imgui_impl_opengl3.cpp>"
	"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/Imgui/imgui_impl_vulkan.cpp>"
	"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/vulkan/include/Volk/volk.c>"
	"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_THIRD_PARTY}/vulkan/include/spirv_reflect/spirv_reflect.c>"
)
target_include_directories(GraphicsFramework PUBLIC
	"${GF_SOURCE_DIR}"
	"${GF_THIRD_PARTY}"
	"${GF_THIRD_PARTY}/Imgui"
	"${GF_THIRD_PARTY}/SDL/include"
	"${GF_THIRD_PARTY}/GLEW/include"
	"${GF_THIRD_PARTY}/vulkan/include"
)
target_include_directories(GraphicsFramework PUBLIC "${GF_THIRD_PARTY}/vulkan/include/Volk")
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
)
target_link_libraries(GraphicsFramework PUBLIC
	"${GF_SDL_LIB}"
	"${GF_ASSIMP_LIB}"
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:${GF_GLEW_LIB}>"
	"$<$<OR:$<CONFIG:Debug_OpenGL>,$<CONFIG:Release_OpenGL>>:opengl32>"
	"$<$<OR:$<CONFIG:Debug_Vulkan>,$<CONFIG:Release_Vulkan>>:${GF_VULKAN_LIB}>"
)

file(GLOB GF_RUNTIME_DLLS CONFIGURE_DEPENDS "${GF_PROJECT_DIR}/DLLs/*.dll")
function(add_framework_app target_name source_dir)
	file(GLOB_RECURSE app_sources CONFIGURE_DEPENDS "${source_dir}/*.cpp")
	if(NOT app_sources)
		message(FATAL_ERROR "No C++ sources found for ${target_name} in ${source_dir}")
	endif()

	add_executable(${target_name} ${app_sources})
	target_link_libraries(${target_name} PRIVATE GraphicsFramework "${GF_SDL_MAIN_LIB}")
	set_property(TARGET ${target_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${GF_PROJECT_DIR}/${target_name}")
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
