#include "SemaphoreVK.h"

#include "Core/Core.h"

namespace rhi::vk
{
SemaphoreVK::SemaphoreVK() :
	mSemaphore(VK_NULL_HANDLE)
{
	VkSemaphoreCreateInfo SemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VKCall(vkCreateSemaphore(Device::Get()->GetDevice(), &SemaphoreCreateInfo, nullptr, &mSemaphore), "Semaphore creation Failed.");
}

SemaphoreVK::~SemaphoreVK()
{
	vkDestroySemaphore(Device::Get()->GetDevice(), mSemaphore, nullptr);
}
}
