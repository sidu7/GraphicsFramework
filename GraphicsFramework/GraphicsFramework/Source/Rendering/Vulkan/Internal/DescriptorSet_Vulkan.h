#pragma once

#include "Rendering/Vulkan/Renderer_Vulkan.h"

class DescriptorPool_Vulkan;

class DescriptorSet_Vulkan
{
	friend class DescriptorPool_Vulkan;
public:
	DescriptorSet_Vulkan();
	virtual ~DescriptorSet_Vulkan();

	void AddBinding(VkDescriptorSetLayoutBinding bindingInfo);
	void Init(DescriptorPool_Vulkan* descPool = nullptr);

	VkDescriptorSet DescSet;
	VkDescriptorSetLayout DescLayout;

protected:
	DescriptorPool_Vulkan* mOwningPool;
	uint32_t Count;
	std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
};