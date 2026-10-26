#include "CommandBufferPoolVK.h"

#include "Core/Core.h"
#include "CommandBufferVK.h"

namespace rhi::vk
{
CommandBufferPoolVK::CommandBufferPoolVK() :
	mCommandPool(VK_NULL_HANDLE)
{
}

CommandBufferPoolVK::~CommandBufferPoolVK()
{
	vkDestroyCommandPool(Device::Get()->GetDevice(), mCommandPool, nullptr);
}

void CommandBufferPoolVK::Init(CommandQueueType type, bool isTransient)
{
	mType = type;
	bTransient = isTransient;

	VkCommandPoolCreateInfo CmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (bTransient)
	{
		CmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	}
	CmdPoolInfo.queueFamilyIndex = Device::Get()->GetQueueFamilyIndex(type);

	VKCall(vkCreateCommandPool(Device::Get()->GetDevice(), &CmdPoolInfo, nullptr, &mCommandPool), "Command Buffer creation Failed");
}

CommandBufferVK* CommandBufferPoolVK::AllocateCommandBuffer()
{
	CommandBufferVK* newCommandBuffer = new CommandBufferVK();
	newCommandBuffer->Init(this, bTransient);

	return newCommandBuffer;
}
}
