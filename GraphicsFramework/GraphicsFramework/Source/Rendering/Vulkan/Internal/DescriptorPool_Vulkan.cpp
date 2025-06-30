#include "DescriptorPool_Vulkan.h"

#include "DescriptorSet_Vulkan.h"

DescriptorPool_Vulkan::DescriptorPool_Vulkan() :
	DescPool(VK_NULL_HANDLE),
	PoolSize(0),
	PoolInfo{ VK_DESCRIPTOR_TYPE_MAX_ENUM, 0 },
	Count(0)
{
}

DescriptorPool_Vulkan::~DescriptorPool_Vulkan()
{
	vkDestroyDescriptorPool(Renderer_Vulkan::Get()->GetDevice(), DescPool, nullptr);
}

void DescriptorPool_Vulkan::Init(const DescriptorPoolInfo& poolInfo)
{
	PoolSize = 0;
	PoolInfo = poolInfo;
	VkDescriptorPoolSize PoolSizeInfo;	
	PoolSizeInfo.type = PoolInfo.Type;
	PoolSizeInfo.descriptorCount = PoolInfo.Count;
	PoolSize += PoolInfo.Count;
	
	VkDescriptorPoolCreateInfo DescPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	DescPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	DescPoolInfo.maxSets = PoolSize;
	DescPoolInfo.poolSizeCount = 1;
	DescPoolInfo.pPoolSizes = &PoolSizeInfo;

	VKCall(vkCreateDescriptorPool(Renderer_Vulkan::Get()->GetDevice(), &DescPoolInfo, nullptr, &DescPool), "Desciptor Pool creation Failed.");
}

DescriptorSet_Vulkan* DescriptorPool_Vulkan::AllocateDescriptor(uint32_t Count, uint32_t binding)
{
	DescriptorSet_Vulkan* DescSet = new DescriptorSet_Vulkan();
	DescSet->Init(this, PoolInfo.Type, Count, binding);
	return DescSet;	
}

bool DescriptorPool_Vulkan::Supports(VkDescriptorType type, uint32_t count) const
{
	return PoolInfo.Type == type && (Count + count) <= PoolSize;
}
