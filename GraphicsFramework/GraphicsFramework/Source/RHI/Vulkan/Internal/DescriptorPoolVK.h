#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class DescriptorSetVK;
class DescriptorPoolVK
{
public:
	DescriptorPoolVK();
	virtual ~DescriptorPoolVK();

	void AddPoolType(VkDescriptorType type, uint32_t count);
	void Init();

	bool AllocateDescriptorSet(const DescriptorSetVK* DescSet);
	bool FreeDescriptorSet(const DescriptorSetVK* DescSet);

	VkDescriptorPool DescPool;
	uint32_t PoolSize;

private:
	uint32_t Count;
	std::vector<VkDescriptorPoolSize> PoolInfos;
	std::unordered_map<VkDescriptorType, uint32_t> PoolSizesMap;
};
}
