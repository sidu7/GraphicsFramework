// Single translation unit that compiles the Vulkan Memory Allocator implementation.
// VMA is a single-header library: every other file only includes <vk_mem_alloc.h>
// for the declarations, and exactly this file defines VMA_IMPLEMENTATION.
//
// It is picked up by the RHI/Vulkan/*.cpp glob in Source/CMakeLists.txt, so it is
// compiled for the Vulkan configurations only.

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
