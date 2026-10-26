#include "DescriptorSetVK.h"

#include "DescriptorPoolVK.h"

namespace rhi::vk
{
DescriptorSetVK::DescriptorSetVK() :
	DescSet(VK_NULL_HANDLE),
	DescLayout(VK_NULL_HANDLE),
	mOwningPool(nullptr),
	Count(0)
{
}

DescriptorSetVK::~DescriptorSetVK()
{
	vkDestroyDescriptorSetLayout(Device::Get()->GetDevice(), DescLayout, nullptr);
	if (mOwningPool)
	{
		mOwningPool->FreeDescriptorSet(this);
		vkFreeDescriptorSets(Device::Get()->GetDevice(), mOwningPool->DescPool, 1, &DescSet);
	}
}

void DescriptorSetVK::AddBinding(VkDescriptorSetLayoutBinding bindingInfo)
{
	LayoutBindings.push_back(bindingInfo);
}

void DescriptorSetVK::Init(DescriptorPoolVK* descPool)
{
	mOwningPool = descPool ? descPool : Device::Get()->GetDescriptorPool();
	Count = LayoutBindings.size();

	VkDescriptorSetLayoutCreateInfo UboDescLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	UboDescLayoutInfo.bindingCount = LayoutBindings.size();
	UboDescLayoutInfo.pBindings = LayoutBindings.data();

	VKCall(vkCreateDescriptorSetLayout(Device::Get()->GetDevice(), &UboDescLayoutInfo, nullptr, &DescLayout), "Ubo Descriptor Set Layout creation Failed.");

	VkDescriptorSetAllocateInfo DescAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescAllocInfo.descriptorPool = mOwningPool->DescPool;
	DescAllocInfo.descriptorSetCount = 1;
	DescAllocInfo.pSetLayouts = &DescLayout;

	VKCall(vkAllocateDescriptorSets(Device::Get()->GetDevice(), &DescAllocInfo, &DescSet), "Descriptor Set allocation Failed.");

	mOwningPool->AllocateDescriptorSet(this);
}
}
