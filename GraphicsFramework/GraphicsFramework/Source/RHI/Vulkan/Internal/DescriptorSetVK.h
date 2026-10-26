#pragma once

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class DescriptorPoolVK;
class DescriptorSetVK
{
	friend class DescriptorPoolVK;
public:
	DescriptorSetVK();
	virtual ~DescriptorSetVK();

	void AddBinding(VkDescriptorSetLayoutBinding bindingInfo);
	void Init(DescriptorPoolVK* descPool = nullptr);

	VkDescriptorSet DescSet;
	VkDescriptorSetLayout DescLayout;

protected:
	DescriptorPoolVK* mOwningPool;
	uint32_t Count;
	std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
};
}
