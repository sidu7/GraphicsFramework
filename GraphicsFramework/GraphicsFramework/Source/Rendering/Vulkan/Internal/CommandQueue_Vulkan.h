#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class CommandBuffer_Vulkan;
class Semaphore_Vulkan;
class Fence_Vulkan;
class SwapChain_Vulkan;

class CommandQueue_Vulkan
{
public:
	CommandQueue_Vulkan();
	~CommandQueue_Vulkan();

	void Init(CommandQueueType type);
	void SubmitCommandBuffer(const CommandBuffer_Vulkan* commandBuffer, bool bWait = false) const;
	void SubmitCommandBuffer(const CommandBuffer_Vulkan* commandBuffer, const Semaphore_Vulkan* WaitFor = nullptr, const Semaphore_Vulkan* SignalFinish = nullptr, const Fence_Vulkan* Fence = nullptr) const;
	void PresentToScreen(const Semaphore_Vulkan* WaitFor, SwapChain_Vulkan* SwapChain) const;

	VkQueue mQueue;
	CommandQueueType mType;
};