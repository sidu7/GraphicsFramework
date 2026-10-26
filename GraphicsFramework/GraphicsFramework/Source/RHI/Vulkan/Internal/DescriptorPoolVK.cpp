#include "DescriptorPoolVK.h"

#include "DescriptorSetVK.h"

namespace rhi::vk
{
DescriptorPoolVK::DescriptorPoolVK() :
	DescPool(VK_NULL_HANDLE),
	PoolSize(0),
	Count(0)
{
}

DescriptorPoolVK::~DescriptorPoolVK()
{
	vkDestroyDescriptorPool(Device::Get()->GetDevice(), DescPool, nullptr);
}

void DescriptorPoolVK::AddPoolType(VkDescriptorType type, uint32_t count)
{
	VkDescriptorPoolSize PoolSizeInfo = {};
	PoolSizeInfo.type = type;
	PoolSizeInfo.descriptorCount = count;
	PoolSize = std::max(count, PoolSize);
	PoolInfos.push_back(PoolSizeInfo);
	PoolSizesMap[type] = count;
}

void DescriptorPoolVK::Init()
{
	VkDescriptorPoolCreateInfo DescPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	DescPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	DescPoolInfo.maxSets = PoolSize;
	DescPoolInfo.poolSizeCount = PoolInfos.size();
	DescPoolInfo.pPoolSizes = PoolInfos.data();

	VKCall(vkCreateDescriptorPool(Device::Get()->GetDevice(), &DescPoolInfo, nullptr, &DescPool), "Desciptor Pool creation Failed.");
}

bool DescriptorPoolVK::AllocateDescriptorSet(const DescriptorSetVK* DescSet)
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

bool DescriptorPoolVK::FreeDescriptorSet(const DescriptorSetVK* DescSet)
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
}
