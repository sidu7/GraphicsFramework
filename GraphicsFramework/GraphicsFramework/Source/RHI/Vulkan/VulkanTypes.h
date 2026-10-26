#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

// Vulkan-specific handle wrappers.
// Kept separate from the public RHI headers so backend-only types never leak
// into backend-agnostic code.

// VMA opaque handles. The full definitions live in vk_mem_alloc.h, which is only
// included by the backend translation units that actually allocate memory, so the
// resource classes can keep the handles in their headers without pulling VMA in.
struct VmaAllocator_T;
struct VmaAllocation_T;
typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;

namespace rhi::vk
{
// Buffer plus the VMA allocation that backs it. `MappedMemory` is only non-null
// between MapBuffer() and UnmapBuffer().
struct BufferInfo
{
	BufferInfo() : Buffer(VK_NULL_HANDLE), Allocation(VK_NULL_HANDLE), Size(0), MappedMemory(nullptr) {}

	VkBuffer Buffer;
	VmaAllocation Allocation;
	uint32_t Size;
	void* MappedMemory;
};

// Image plus the VMA allocation that backs it.
struct ImageInfo
{
	ImageInfo() : Image(VK_NULL_HANDLE), Allocation(VK_NULL_HANDLE), Size(0) {}

	VkImage Image;
	VmaAllocation Allocation;
	uint32_t Size;
};
}
