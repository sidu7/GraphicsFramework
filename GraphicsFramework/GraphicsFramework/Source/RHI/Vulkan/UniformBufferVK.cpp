#include "UniformBufferVK.h"

#include "Internal\DescriptorPoolVK.h"

namespace rhi::vk
{
UniformBufferVK::UniformBufferVK() :
	PipelineLayout(nullptr),
	mBinding(0)
{
}

UniformBufferVK::~UniformBufferVK()
{
	vkUnmapMemory(Device::Get()->GetDevice(), UboInfo.BufferMemory);
	vkFreeMemory(Device::Get()->GetDevice(), UboInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Device::Get()->GetDevice(), UboInfo.Buffer, nullptr);
	UboInfo.MappedMemory = nullptr;
}

void UniformBufferVK::Init(uint32_t size, uint32_t binding, const void* data)
{
	mBinding = binding;

	UboInfo = Device::Get()->CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VKCall(vkMapMemory(Device::Get()->GetDevice(), UboInfo.BufferMemory, 0, UboInfo.Size, 0, &UboInfo.MappedMemory), "Uniform Buffer memory mapping Failed.");
}

void UniformBufferVK::AddData(uint32_t size, const void* data, uint32_t offset) const
{
	memcpy(UboInfo.MappedMemory, data, size);
}
}
