#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class Semaphore_Vulkan
{
public:
	Semaphore_Vulkan();
	~Semaphore_Vulkan();

	VkSemaphore mSemaphore;
};