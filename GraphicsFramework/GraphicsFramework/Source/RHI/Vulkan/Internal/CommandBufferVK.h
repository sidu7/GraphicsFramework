#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class CommandBufferPoolVK;
class CommandBufferVK
{
public:
	CommandBufferVK();
	~CommandBufferVK();

	void Init(const CommandBufferPoolVK* commandBufferPool, bool bIsTransient);
	void Reset() const;
	void Begin() const;
	void End() const;

	VkCommandBuffer mCommandBuffer;
	bool bIsTransient;
	std::vector<OnCommandExecutionEnd> ExecutionFinishDelegates;
	CommandBufferPoolVK* mOwningPool;
};
}
