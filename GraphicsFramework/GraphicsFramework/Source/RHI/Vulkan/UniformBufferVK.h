#pragma once

#include "Rendering/UniformBuffer.h"
#include "RHI/Vulkan/VulkanTypes.h"

namespace rhi::vk
{
class DescriptorSetVK;
class UniformBufferVK : public UniformBuffer
{
public:
	UniformBufferVK();
	virtual ~UniformBufferVK();

	virtual void Init(uint32_t size, uint32_t binding, const void* data = nullptr) override;
	virtual void AddData(uint32_t size, const void* data, uint32_t offset = 0) const override;

	virtual uint32_t GetBinding() const override { return mBinding; }

	BufferInfo UboInfo;
	VkPipelineLayout* PipelineLayout;
	uint32_t mBinding;
};
}
