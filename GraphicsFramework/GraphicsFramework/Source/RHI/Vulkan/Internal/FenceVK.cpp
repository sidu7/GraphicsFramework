#include "FenceVK.h"

#include "Core/Core.h"

namespace rhi::vk
{
FenceVK::FenceVK(bool bSignaled) :
	mFence(VK_NULL_HANDLE)
{
	VkFenceCreateInfo FenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (bSignaled)
	{
		FenceInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;
	}
	VKCall(vkCreateFence(Device::Get()->GetDevice(), &FenceInfo, nullptr, &mFence), "Fence creation Failed.");
}

FenceVK::~FenceVK()
{
	vkDestroyFence(Device::Get()->GetDevice(), mFence, nullptr);
}

void FenceVK::Wait()
{
	VKCall(vkWaitForFences(Device::Get()->GetDevice(), 1, &mFence, VK_TRUE, UINT64_MAX), "Waiting for Fence Failed.");
}

void FenceVK::Reset()
{
	VKCall(vkResetFences(Device::Get()->GetDevice(), 1, &mFence), "Fence Reset Failed.");
}
}
