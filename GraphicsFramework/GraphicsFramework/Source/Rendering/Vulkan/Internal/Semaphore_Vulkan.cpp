#include "Semaphore_Vulkan.h"

#include "Core/Core.h"

Semaphore_Vulkan::Semaphore_Vulkan() :
	mSemaphore(VK_NULL_HANDLE)
{
	VkSemaphoreCreateInfo SemaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VKCall(vkCreateSemaphore(Renderer_Vulkan::Get()->GetDevice(), &SemaphoreCreateInfo, nullptr, &mSemaphore), "Semaphore creation Failed.");
}

Semaphore_Vulkan::~Semaphore_Vulkan()
{
	vkDestroySemaphore(Renderer_Vulkan::Get()->GetDevice(), mSemaphore, nullptr);
}