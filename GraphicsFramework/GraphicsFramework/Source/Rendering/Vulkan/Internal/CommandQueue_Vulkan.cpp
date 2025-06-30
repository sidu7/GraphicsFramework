#include "CommandQueue_Vulkan.h"

#include "Core/Core.h"
#include "CommandBuffer_Vulkan.h"
#include "Fence_Vulkan.h"
#include "Semaphore_Vulkan.h"
#include "SwapChain_Vulkan.h"

CommandQueue_Vulkan::CommandQueue_Vulkan() :
	mQueue(VK_NULL_HANDLE)
{
}

CommandQueue_Vulkan::~CommandQueue_Vulkan()
{
}

void CommandQueue_Vulkan::Init(CommandQueueType type)
{
	mType = type;
	vkGetDeviceQueue(Renderer_Vulkan::Get()->GetDevice(), Renderer_Vulkan::Get()->GetQueueFamilyIndex(type), 0, &mQueue);
}

void CommandQueue_Vulkan::SubmitCommandBuffer(const CommandBuffer_Vulkan* commandBuffer, bool bWait) const
{
	if (mType == CommandQueueType::Present)
	{
		return;
	}

	VkCommandBufferSubmitInfo BeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO };
	BeginInfo.commandBuffer = commandBuffer->mCommandBuffer;

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &commandBuffer->mCommandBuffer;

	Fence_Vulkan* TransientFence = nullptr;
	if (bWait)
	{
		TransientFence = new Fence_Vulkan();
	}

	VKCall(vkQueueSubmit(mQueue, 1, &SubmitInfo, bWait ? TransientFence->mFence : VK_NULL_HANDLE), "Transient Command Buffer begin Failed.");

	if (bWait)
	{
		TransientFence->Wait();
		delete TransientFence;
	}
}

void CommandQueue_Vulkan::SubmitCommandBuffer(const CommandBuffer_Vulkan* commandBuffer, const Semaphore_Vulkan* WaitFor, const Semaphore_Vulkan* SignalFinish, const Fence_Vulkan* Fence) const
{
	if (mType == CommandQueueType::Present)
	{
		return;
	}

	VkSubmitInfo SubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };

	std::vector<VkSemaphore> WaitSemaphores;
	std::vector<VkPipelineStageFlags> WaitStages;
	if (WaitFor)
	{
		WaitSemaphores.push_back(WaitFor->mSemaphore);
		WaitStages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	std::vector<VkSemaphore> SignalSemaphores;
	if (SignalFinish)
	{
		SignalSemaphores.push_back(SignalFinish->mSemaphore);
	}

	SubmitInfo.waitSemaphoreCount = WaitSemaphores.size();
	SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
	SubmitInfo.pWaitDstStageMask = WaitStages.data();
	SubmitInfo.signalSemaphoreCount = SignalSemaphores.size();
	SubmitInfo.pSignalSemaphores = SignalSemaphores.data();
	SubmitInfo.commandBufferCount = 1;
	SubmitInfo.pCommandBuffers = &commandBuffer->mCommandBuffer;

	VKCall(vkQueueSubmit(mQueue, 1, &SubmitInfo, Fence ? Fence->mFence : VK_NULL_HANDLE), "Command Buffer Submit Failed.");
}

void CommandQueue_Vulkan::PresentToScreen(const Semaphore_Vulkan* WaitFor, SwapChain_Vulkan* SwapChain) const
{
	if (mType != CommandQueueType::Present)
	{
		return;
	}

	VkSemaphore WaitSemaphores[] = { WaitFor->mSemaphore };

	VkPresentInfoKHR PresentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	PresentInfo.waitSemaphoreCount = ARRAY_SIZE(WaitSemaphores);
	PresentInfo.pWaitSemaphores = WaitSemaphores;
	PresentInfo.swapchainCount = 1;
	PresentInfo.pSwapchains = &SwapChain->SwapChain;
	PresentInfo.pImageIndices = &SwapChain->ImageIdx;
	PresentInfo.pResults = nullptr;

	VkResult PresentResult = vkQueuePresentKHR(mQueue, &PresentInfo);
}
