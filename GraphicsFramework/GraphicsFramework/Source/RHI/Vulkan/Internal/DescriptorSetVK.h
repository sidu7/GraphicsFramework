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

	// Descriptor writes. Every binding of a shader lives in this single set, so
	// the binding number is exactly the one declared in GLSL.
	void WriteUniformBuffer(uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset = 0);
	void WriteTexture(uint32_t binding, VkImageView imageView, VkSampler sampler);

	// True when the reflected layout declares this binding. Used to skip writes
	// for stale bind state (e.g. a texture left bound by a previous scene).
	bool HasBinding(uint32_t binding) const;
	uint32_t GetBindingCount() const { return static_cast<uint32_t>(LayoutBindings.size()); }

	VkDescriptorSet DescSet;
	VkDescriptorSetLayout DescLayout;

protected:
	DescriptorPoolVK* mOwningPool;
	uint32_t Count;
	std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;
};
}
