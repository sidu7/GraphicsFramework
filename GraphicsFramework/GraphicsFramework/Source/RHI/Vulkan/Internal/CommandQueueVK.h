#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class CommandBufferVK;
class SemaphoreVK;
class FenceVK;
class SwapChainVK;
class CommandQueueVK
{
public:
	CommandQueueVK();
	~CommandQueueVK();

	void Init(CommandQueueType type);
	void SubmitCommandBuffer(const CommandBufferVK* commandBuffer, bool bWait = false) const;
	void SubmitCommandBuffer(const CommandBufferVK* commandBuffer, const SemaphoreVK* WaitFor = nullptr, const SemaphoreVK* SignalFinish = nullptr, const FenceVK* Fence = nullptr) const;
	void PresentToScreen(const SemaphoreVK* WaitFor, SwapChainVK* SwapChain) const;

	VkQueue mQueue;
	CommandQueueType mType;
};
}
