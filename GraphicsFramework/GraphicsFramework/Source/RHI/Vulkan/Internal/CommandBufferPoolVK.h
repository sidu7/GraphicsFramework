#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class CommandBufferVK;
class CommandBufferPoolVK
{
public:
	CommandBufferPoolVK();
	~CommandBufferPoolVK();

	void Init(CommandQueueType type, bool isTransient = false);

	CommandBufferVK* AllocateCommandBuffer();

	VkCommandPool mCommandPool;
	CommandQueueType mType;
	bool bTransient;
};
}
