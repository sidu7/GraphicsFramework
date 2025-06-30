#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class CommandBuffer_Vulkan;

class CommandBufferPool_Vulkan
{
public:
	CommandBufferPool_Vulkan();
	~CommandBufferPool_Vulkan();

	void Init(CommandQueueType type, bool isTransient = false);

	CommandBuffer_Vulkan* AllocateCommandBuffer();

	VkCommandPool mCommandPool;
	CommandQueueType mType;
	bool bTransient;
};