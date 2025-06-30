#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorPool_Vulkan;

class DescriptorSet_Vulkan
{
public:
	DescriptorSet_Vulkan();
	virtual ~DescriptorSet_Vulkan();

	void Init(DescriptorPool_Vulkan* descPool, VkDescriptorType type, uint32_t count, uint32_t binding);

	VkDescriptorSet DescSet;
	VkDescriptorSetLayout DescLayout;
	DescriptorPool_Vulkan* mOwningPool;
	uint32_t Count;
};