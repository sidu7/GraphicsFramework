#include "CommandBufferPool_Vulkan.h"

#include "Core/Core.h"
#include "CommandBuffer_Vulkan.h"

CommandBufferPool_Vulkan::CommandBufferPool_Vulkan() :
	mCommandPool(VK_NULL_HANDLE)
{
}

CommandBufferPool_Vulkan::~CommandBufferPool_Vulkan()
{
	vkDestroyCommandPool(Renderer_Vulkan::Get()->GetDevice(), mCommandPool, nullptr);
}

void CommandBufferPool_Vulkan::Init(CommandQueueType type, bool isTransient)
{
	mType = type;
	bTransient = isTransient;

	VkCommandPoolCreateInfo CmdPoolInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	CmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	if (bTransient)
	{
		CmdPoolInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	}
	CmdPoolInfo.queueFamilyIndex = Renderer_Vulkan::Get()->GetQueueFamilyIndex(type);

	VKCall(vkCreateCommandPool(Renderer_Vulkan::Get()->GetDevice(), &CmdPoolInfo, nullptr, &mCommandPool), "Command Buffer creation Failed");
}

CommandBuffer_Vulkan* CommandBufferPool_Vulkan::AllocateCommandBuffer()
{
	CommandBuffer_Vulkan* newCommandBuffer = new CommandBuffer_Vulkan();
	newCommandBuffer->Init(this, bTransient);

	return newCommandBuffer;
}