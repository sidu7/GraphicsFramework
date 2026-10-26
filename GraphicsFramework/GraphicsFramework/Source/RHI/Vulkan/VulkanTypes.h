#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

// Vulkan-specific handle wrappers.
// Kept separate from the public RHI headers so backend-only types never leak
// into backend-agnostic code.

namespace rhi::vk
{
struct BufferInfo
{
	BufferInfo() : Buffer(VK_NULL_HANDLE), BufferMemory(VK_NULL_HANDLE), Size(0), MappedMemory(nullptr) {}

	VkBuffer Buffer;
	VkDeviceMemory BufferMemory;
	uint32_t Size;
	void* MappedMemory;
};

struct ImageInfo
{
	ImageInfo() : Image(VK_NULL_HANDLE), ImageMemory(VK_NULL_HANDLE), Size(0) {}

	VkImage Image;
	VkDeviceMemory ImageMemory;
	uint32_t Size;
};
}
