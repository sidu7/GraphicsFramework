#include "DescriptorSet_Vulkan.h"

#include "DescriptorPool_Vulkan.h"

DescriptorSet_Vulkan::DescriptorSet_Vulkan() :
	DescSet(VK_NULL_HANDLE),
	DescLayout(VK_NULL_HANDLE),
	mOwningPool(nullptr),
	Count(0)
{
}

DescriptorSet_Vulkan::~DescriptorSet_Vulkan()
{
	vkDestroyDescriptorSetLayout(Renderer_Vulkan::Get()->GetDevice(), DescLayout, nullptr);
	if (mOwningPool)
	{
		mOwningPool->FreeDescriptorSet(this);
		vkFreeDescriptorSets(Renderer_Vulkan::Get()->GetDevice(), mOwningPool->DescPool, 1, &DescSet);
	}
}

void DescriptorSet_Vulkan::AddBinding(VkDescriptorSetLayoutBinding bindingInfo)
{
	LayoutBindings.push_back(bindingInfo);
}

void DescriptorSet_Vulkan::Init(DescriptorPool_Vulkan* descPool)
{
	mOwningPool = descPool ? descPool : Renderer_Vulkan::Get()->GetDescriptorPool();
	Count = LayoutBindings.size();

	VkDescriptorSetLayoutCreateInfo UboDescLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	UboDescLayoutInfo.bindingCount = LayoutBindings.size();
	UboDescLayoutInfo.pBindings = LayoutBindings.data();

	VKCall(vkCreateDescriptorSetLayout(Renderer_Vulkan::Get()->GetDevice(), &UboDescLayoutInfo, nullptr, &DescLayout), "Ubo Descriptor Set Layout creation Failed.");

	VkDescriptorSetAllocateInfo DescAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescAllocInfo.descriptorPool = mOwningPool->DescPool;
	DescAllocInfo.descriptorSetCount = 1;
	DescAllocInfo.pSetLayouts = &DescLayout;

	VKCall(vkAllocateDescriptorSets(Renderer_Vulkan::Get()->GetDevice(), &DescAllocInfo, &DescSet), "Descriptor Set allocation Failed.");

	mOwningPool->AllocateDescriptorSet(this);
}