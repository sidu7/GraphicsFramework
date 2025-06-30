#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class CommandBufferPool_Vulkan;

class CommandBuffer_Vulkan
{
public:
	CommandBuffer_Vulkan();
	~CommandBuffer_Vulkan();

	void Init(const CommandBufferPool_Vulkan* commandBufferPool, bool bIsTransient);
	void Reset() const;
	void Begin() const;
	void End() const;

	VkCommandBuffer mCommandBuffer;
	bool bIsTransient;
	std::vector<OnCommandExecutionEnd> ExecutionFinishDelegates;
	CommandBufferPool_Vulkan* mOwningPool;
};