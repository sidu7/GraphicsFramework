#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorSet_Vulkan;

struct DescriptorPoolInfo
{
	VkDescriptorType Type;
	uint32_t Count;
};

class DescriptorPool_Vulkan
{
public:
	DescriptorPool_Vulkan();
	virtual ~DescriptorPool_Vulkan();

	void Init(const DescriptorPoolInfo& poolInfo);

	DescriptorSet_Vulkan* AllocateDescriptor(uint32_t Count, uint32_t binding);
	bool Supports(VkDescriptorType type, uint32_t count) const;

	VkDescriptorPool DescPool;
	uint32_t PoolSize;
	DescriptorPoolInfo PoolInfo;
	uint32_t Count;
};