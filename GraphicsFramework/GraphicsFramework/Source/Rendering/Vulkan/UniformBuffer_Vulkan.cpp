#include "UniformBuffer_Vulkan.h"

#include "Internal\DescriptorPool_Vulkan.h"

UniformBuffer_Vulkan::UniformBuffer_Vulkan() :
	PipelineLayout(nullptr),
	mBinding(0)
{
}

UniformBuffer_Vulkan::~UniformBuffer_Vulkan()
{
	vkUnmapMemory(Renderer_Vulkan::Get()->GetDevice(), UboInfo.BufferMemory);
	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), UboInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), UboInfo.Buffer, nullptr);
	UboInfo.MappedMemory = nullptr;
}

void UniformBuffer_Vulkan::Init(uint32_t size, uint32_t binding, const void* data)
{
	RenderingFactory_Vulkan* VkFactory = static_cast<RenderingFactory_Vulkan*>(RenderingFactory::Instance());
	if (VkFactory)
	{
		mBinding = binding;

		UboInfo = VkFactory->CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VKCall(vkMapMemory(Renderer_Vulkan::Get()->GetDevice(), UboInfo.BufferMemory, 0, UboInfo.Size, 0, &UboInfo.MappedMemory), "Uniform Buffer memory mapping Failed.");
	}
}

void UniformBuffer_Vulkan::AddData(uint32_t size, const void* data, uint32_t offset) const
{
	memcpy(UboInfo.MappedMemory, data, size);
}
