#include "Fence_Vulkan.h"

#include "Core/Core.h"

Fence_Vulkan::Fence_Vulkan(bool bSignaled) :
	mFence(VK_NULL_HANDLE)
{
	VkFenceCreateInfo FenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (bSignaled)
	{
		FenceInfo.flags |= VK_FENCE_CREATE_SIGNALED_BIT;
	}
	VKCall(vkCreateFence(Renderer_Vulkan::Get()->GetDevice(), &FenceInfo, nullptr, &mFence), "Fence creation Failed.");
}

Fence_Vulkan::~Fence_Vulkan()
{
	vkDestroyFence(Renderer_Vulkan::Get()->GetDevice(), mFence, nullptr);
}

void Fence_Vulkan::Wait()
{
	VKCall(vkWaitForFences(Renderer_Vulkan::Get()->GetDevice(), 1, &mFence, VK_TRUE, UINT64_MAX), "Waiting for Fence Failed.");
}
