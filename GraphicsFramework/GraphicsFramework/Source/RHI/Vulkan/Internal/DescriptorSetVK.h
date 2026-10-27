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
	void Init();

	// Allocate a descriptor set matching this layout from `pool`. The pipeline's
	// descriptor set is re-created for every draw so that recording several draws
	// with different resources into the same command buffer stays correct.
	VkDescriptorSet Allocate(VkDescriptorPool pool) const;

	// Descriptor writes. Every binding of a shader lives in this single set, so
	// the binding number is exactly the one declared in GLSL.
	void WriteUniformBuffer(VkDescriptorSet dstSet, uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0) const;
	void WriteTexture(VkDescriptorSet dstSet, uint32_t binding, VkImageView imageView, VkSampler sampler) const;

	// True when the reflected layout declares this binding. Used to skip writes
	// for stale bind state (e.g. a texture left bound by a previous scene).
	bool HasBinding(uint32_t binding) const;
	VkDescriptorType GetBindingType(uint32_t binding) const;

	uint32_t GetBindingCount() const { return static_cast<uint32_t>(LayoutBindings.size()); }

	VkDescriptorSetLayout DescLayout;

protected:
	std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
};
}
