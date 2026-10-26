#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class FenceVK
{
public:
	FenceVK(bool bSignaled = false);
	~FenceVK();

	void Wait();
	void Reset();

	VkFence mFence;
};
}
