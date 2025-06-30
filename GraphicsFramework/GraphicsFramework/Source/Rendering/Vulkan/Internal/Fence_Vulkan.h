#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class Fence_Vulkan
{
public:
	Fence_Vulkan(bool bSignaled = false);
	~Fence_Vulkan();

	void Wait();

	VkFence mFence;
};