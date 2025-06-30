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
		mOwningPool->Count -= Count;
		vkFreeDescriptorSets(Renderer_Vulkan::Get()->GetDevice(), mOwningPool->DescPool, 1, &DescSet);
	}
}

void DescriptorSet_Vulkan::Init(DescriptorPool_Vulkan* descPool, VkDescriptorType type, uint32_t count, uint32_t binding)
{
	mOwningPool = descPool;
	Count = count;

	VkDescriptorSetLayoutBinding LayoutBindings = {};
	LayoutBindings.binding = binding;
	LayoutBindings.descriptorType = type;
	LayoutBindings.descriptorCount = count;
	LayoutBindings.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	

	VkDescriptorSetLayoutCreateInfo UboDescLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	UboDescLayoutInfo.bindingCount = 1;
	UboDescLayoutInfo.pBindings = &LayoutBindings;

	VKCall(vkCreateDescriptorSetLayout(Renderer_Vulkan::Get()->GetDevice(), &UboDescLayoutInfo, nullptr, &DescLayout), "Ubo Descriptor Set Layout creation Failed.");

	VkDescriptorSetAllocateInfo DescAllocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	DescAllocInfo.descriptorPool = mOwningPool->DescPool;
	DescAllocInfo.descriptorSetCount = 1;
	DescAllocInfo.pSetLayouts = &DescLayout;

	VKCall(vkAllocateDescriptorSets(Renderer_Vulkan::Get()->GetDevice(), &DescAllocInfo, &DescSet), "Descriptor Set allocation Failed.");

	mOwningPool->Count += Count;
}