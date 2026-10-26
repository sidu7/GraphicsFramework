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
	Device* VkDev = Device::Get();

	if (UboInfo.MappedMemory)
	{
		VkDev->UnmapBuffer(UboInfo);
	}
	VkDev->DestroyBuffer(UboInfo);
}

void UniformBufferVK::Init(uint32_t size, uint32_t binding, const void* data)
{
	mBinding = binding;

	UboInfo = Device::Get()->CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true /*bHostVisible*/);

	UboInfo.MappedMemory = Device::Get()->MapBuffer(UboInfo);
}

void UniformBufferVK::AddData(uint32_t size, const void* data, uint32_t offset) const
{
	memcpy(UboInfo.MappedMemory, data, size);
}
}
