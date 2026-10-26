#include "CommandBufferVK.h"

#include "Core/Core.h"
#include "CommandBufferPoolVK.h"

namespace rhi::vk
{
CommandBufferVK::CommandBufferVK() :
	mCommandBuffer(VK_NULL_HANDLE),
	bIsTransient(false),
	mOwningPool(nullptr)
{
}

CommandBufferVK::~CommandBufferVK()
{
	if (mOwningPool)
	{
		vkFreeCommandBuffers(Device::Get()->GetDevice(), mOwningPool->mCommandPool, 1, &mCommandBuffer);
	}
}

void CommandBufferVK::Init(const CommandBufferPoolVK* commandBufferPool, bool bTransient)
{
	bIsTransient = bTransient;

	VkCommandBufferAllocateInfo CmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CmdAllocInfo.commandPool = commandBufferPool->mCommandPool;
	CmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CmdAllocInfo.commandBufferCount = 1;

	VKCall(vkAllocateCommandBuffers(Device::Get()->GetDevice(), &CmdAllocInfo, &mCommandBuffer), "Command Buffer allocation Failed.");
}

void CommandBufferVK::Reset() const
{
	VKCall(vkResetCommandBuffer(mCommandBuffer, 0), "Command Buffer Reset Failed.");
}

void CommandBufferVK::Begin() const
{
	VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CommandBufferBeginInfo.flags = bIsTransient ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;

	VKCall(vkBeginCommandBuffer(mCommandBuffer, &CommandBufferBeginInfo), "Command Buffer begin Failed.");
}

void CommandBufferVK::End() const
{
	VKCall(vkEndCommandBuffer(mCommandBuffer), "Command Buffer end Failed.");
}
}
