#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class SemaphoreVK
{
public:
	SemaphoreVK();
	~SemaphoreVK();

	VkSemaphore mSemaphore;
};
}
