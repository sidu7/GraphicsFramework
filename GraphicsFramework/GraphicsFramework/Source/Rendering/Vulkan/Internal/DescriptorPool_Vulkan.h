#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorSet_Vulkan;

class DescriptorPool_Vulkan
{
public:
	DescriptorPool_Vulkan();
	virtual ~DescriptorPool_Vulkan();

	void AddPoolType(VkDescriptorType type, uint32_t count);
	void Init();

	bool AllocateDescriptorSet(const DescriptorSet_Vulkan* DescSet);
	bool FreeDescriptorSet(const DescriptorSet_Vulkan* DescSet);

	VkDescriptorPool DescPool;
	uint32_t PoolSize;

private:
	uint32_t Count;
	std::vector<VkDescriptorPoolSize> PoolInfos;
	std::unordered_map<VkDescriptorType, uint32_t> PoolSizesMap;
};