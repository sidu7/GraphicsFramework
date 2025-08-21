#include "DescriptorPool_Vulkan.h"

#include "DescriptorSet_Vulkan.h"

DescriptorPool_Vulkan::DescriptorPool_Vulkan() :
	DescPool(VK_NULL_HANDLE),
	PoolSize(0),
	Count(0)
{
}

DescriptorPool_Vulkan::~DescriptorPool_Vulkan()
{
	vkDestroyDescriptorPool(Renderer_Vulkan::Get()->GetDevice(), DescPool, nullptr);
}

void DescriptorPool_Vulkan::AddPoolType(VkDescriptorType type, uint32_t count)
{
	VkDescriptorPoolSize PoolSizeInfo = {};
	PoolSizeInfo.type = type;
	PoolSizeInfo.descriptorCount = count;
	PoolSize = std::max(count, PoolSize);
	PoolInfos.push_back(PoolSizeInfo);
	PoolSizesMap[type] = count;
}

void DescriptorPool_Vulkan::Init()
{
	VkDescriptorPoolCreateInfo DescPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	DescPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	DescPoolInfo.maxSets = PoolSize;
	DescPoolInfo.poolSizeCount = PoolInfos.size();
	DescPoolInfo.pPoolSizes = PoolInfos.data();

	VKCall(vkCreateDescriptorPool(Renderer_Vulkan::Get()->GetDevice(), &DescPoolInfo, nullptr, &DescPool), "Desciptor Pool creation Failed.");
}

bool DescriptorPool_Vulkan::AllocateDescriptorSet(const DescriptorSet_Vulkan* DescSet)
{
	if (DescSet)
	{
		for (const VkDescriptorSetLayoutBinding& Binding : DescSet->LayoutBindings)
		{
			auto FoundItr = PoolSizesMap.find(Binding.descriptorType);
			if (FoundItr != PoolSizesMap.end() && (FoundItr->second - Binding.descriptorCount) >= 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool DescriptorPool_Vulkan::FreeDescriptorSet(const DescriptorSet_Vulkan* DescSet)
{
	if (DescSet)
	{
		for (const VkDescriptorSetLayoutBinding& Binding : DescSet->LayoutBindings)
		{
			if (PoolSizesMap.find(Binding.descriptorType) != PoolSizesMap.end())
			{
				return true;
			}
		}
	}

	return false;
}