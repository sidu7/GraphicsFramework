#include "DescriptorSetVK.h"

#include "DescriptorPoolVK.h"

namespace rhi::vk
{
DescriptorSetVK::DescriptorSetVK() :
	DescLayout(VK_NULL_HANDLE)
{
}

DescriptorSetVK::~DescriptorSetVK()
{
	if (DescLayout != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(Device::Get()->GetDevice(), DescLayout, nullptr);
	}
}

void DescriptorSetVK::AddBinding(VkDescriptorSetLayoutBinding bindingInfo)
{
	// A binding can be reflected by both shader stages (e.g. a uniform block used
	// in the vertex and fragment shader). Vulkan expects a single entry whose
	// stage flags are the union of the stages that read it.
	for (VkDescriptorSetLayoutBinding& Existing : LayoutBindings)
	{
		if (Existing.binding == bindingInfo.binding)
		{
			Existing.stageFlags |= bindingInfo.stageFlags;
			Existing.descriptorCount = std::max(Existing.descriptorCount, bindingInfo.descriptorCount);
			return;
		}
	}

	LayoutBindings.push_back(bindingInfo);
}

void DescriptorSetVK::Init()
{
	// No bindings - nothing to initialize
	if (LayoutBindings.empty())
	{
		DescLayout = VK_NULL_HANDLE;
		return;
	}

	VkDescriptorSetLayoutCreateInfo UboDescLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	UboDescLayoutInfo.bindingCount = static_cast<uint32_t>(LayoutBindings.size());
	UboDescLayoutInfo.pBindings = LayoutBindings.data();

	VKCall(vkCreateDescriptorSetLayout(Device::Get()->GetDevice(), &UboDescLayoutInfo, nullptr, &DescLayout), "Ubo Descriptor Set Layout creation Failed.");
}

VkDescriptorSet DescriptorSetVK::Allocate(VkDescriptorPool pool) const
{
	if (pool == VK_NULL_HANDLE || DescLayout == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	VkDescriptorSetAllocateInfo DescAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescAllocInfo.descriptorPool = pool;
	DescAllocInfo.descriptorSetCount = 1;
	DescAllocInfo.pSetLayouts = &DescLayout;

	VkDescriptorSet AllocatedSet = VK_NULL_HANDLE;
	VKCall(vkAllocateDescriptorSets(Device::Get()->GetDevice(), &DescAllocInfo, &AllocatedSet), "Descriptor Set allocation Failed.");

	return AllocatedSet;
}

void DescriptorSetVK::WriteUniformBuffer(VkDescriptorSet dstSet, uint32_t binding, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset) const
{
	VkDescriptorBufferInfo BufferInfo = {};
	BufferInfo.buffer = buffer;
	BufferInfo.offset = offset;
	BufferInfo.range = size;

	VkWriteDescriptorSet WriteDesc = {};
	WriteDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDesc.dstSet = dstSet;
	WriteDesc.dstBinding = binding;
	WriteDesc.dstArrayElement = 0;
	WriteDesc.descriptorCount = 1;
	WriteDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	WriteDesc.pBufferInfo = &BufferInfo;
	WriteDesc.pImageInfo = nullptr;
	WriteDesc.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(Device::Get()->GetDevice(), 1, &WriteDesc, 0, nullptr);
}

void DescriptorSetVK::WriteTexture(VkDescriptorSet dstSet, uint32_t binding, VkImageView imageView, VkSampler sampler) const
{
	VkDescriptorImageInfo ImageInfo = {};
	ImageInfo.sampler = sampler;
	ImageInfo.imageView = imageView;
	ImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet WriteDesc = {};
	WriteDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	WriteDesc.dstSet = dstSet;
	WriteDesc.dstBinding = binding;
	WriteDesc.dstArrayElement = 0;
	WriteDesc.descriptorCount = 1;
	WriteDesc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	WriteDesc.pBufferInfo = nullptr;
	WriteDesc.pImageInfo = &ImageInfo;
	WriteDesc.pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(Device::Get()->GetDevice(), 1, &WriteDesc, 0, nullptr);
}

VkDescriptorType DescriptorSetVK::GetBindingType(uint32_t binding) const
{
	for (const VkDescriptorSetLayoutBinding& LayoutBinding : LayoutBindings)
	{
		if (LayoutBinding.binding == binding)
		{
			return LayoutBinding.descriptorType;
		}
	}

	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

bool DescriptorSetVK::HasBinding(uint32_t binding) const
{
	return GetBindingType(binding) != VK_DESCRIPTOR_TYPE_MAX_ENUM;
}
}
