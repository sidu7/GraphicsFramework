#include "CommandQueueVK.h"

#include "Core/Core.h"
#include "CommandBufferVK.h"
#include "FenceVK.h"
#include "SemaphoreVK.h"
#include "SwapChainVK.h"

namespace rhi::vk
{
CommandQueueVK::CommandQueueVK() :
	mQueue(VK_NULL_HANDLE)
{
}

CommandQueueVK::~CommandQueueVK()
{
}

void CommandQueueVK::Init(CommandQueueType type)
{
	mType = type;
	vkGetDeviceQueue(Device::Get()->GetDevice(), Device::Get()->GetQueueFamilyIndex(type), 0, &mQueue);
}

void CommandQueueVK::SubmitCommandBuffer(const CommandBufferVK* commandBuffer, bool bWait) const
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

	FenceVK* TransientFence = nullptr;
	if (bWait)
	{
		TransientFence = new FenceVK();
	}

	VKCall(vkQueueSubmit(mQueue, 1, &SubmitInfo, bWait ? TransientFence->mFence : VK_NULL_HANDLE), "Transient Command Buffer begin Failed.");

	if (bWait)
	{
		TransientFence->Wait();
		delete TransientFence;
	}
}

void CommandQueueVK::SubmitCommandBuffer(const CommandBufferVK* commandBuffer, const SemaphoreVK* WaitFor, const SemaphoreVK* SignalFinish, const FenceVK* Fence) const
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

void CommandQueueVK::PresentToScreen(const SemaphoreVK* WaitFor, SwapChainVK* SwapChain) const
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
}
