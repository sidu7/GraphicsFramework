#include "CommandBuffer_Vulkan.h"

#include "Core/Core.h"
#include "CommandBufferPool_Vulkan.h"

CommandBuffer_Vulkan::CommandBuffer_Vulkan() :
	mCommandBuffer(VK_NULL_HANDLE),
	bIsTransient(false),
	mOwningPool(nullptr)
{
}

CommandBuffer_Vulkan::~CommandBuffer_Vulkan()
{
	if (mOwningPool)
	{
		vkFreeCommandBuffers(Renderer_Vulkan::Get()->GetDevice(), mOwningPool->mCommandPool, 1, &mCommandBuffer);
	}
}

void CommandBuffer_Vulkan::Init(const CommandBufferPool_Vulkan* commandBufferPool, bool bTransient)
{
	bIsTransient = bTransient;

	VkCommandBufferAllocateInfo CmdAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	CmdAllocInfo.commandPool = commandBufferPool->mCommandPool;
	CmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	CmdAllocInfo.commandBufferCount = 1;

	VKCall(vkAllocateCommandBuffers(Renderer_Vulkan::Get()->GetDevice(), &CmdAllocInfo, &mCommandBuffer), "Command Buffer allocation Failed.");
}

void CommandBuffer_Vulkan::Reset() const
{
	VKCall(vkResetCommandBuffer(mCommandBuffer, 0), "Command Buffer Reset Failed.");
}

void CommandBuffer_Vulkan::Begin() const
{
	VkCommandBufferBeginInfo CommandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	CommandBufferBeginInfo.flags = bIsTransient ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	CommandBufferBeginInfo.pInheritanceInfo = nullptr;

	VKCall(vkBeginCommandBuffer(mCommandBuffer, &CommandBufferBeginInfo), "Command Buffer begin Failed.");
}

void CommandBuffer_Vulkan::End() const
{
	VKCall(vkEndCommandBuffer(mCommandBuffer), "Command Buffer end Failed.");
}