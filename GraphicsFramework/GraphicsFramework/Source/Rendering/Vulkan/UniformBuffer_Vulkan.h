#pragma once

#include "Rendering/UniformBuffer.h"
#include "RenderingFactory_Vulkan.h"

class DescriptorSet_Vulkan;

class UniformBuffer_Vulkan : public UniformBuffer
{
public:
	UniformBuffer_Vulkan();
	virtual ~UniformBuffer_Vulkan();

	virtual void Init(uint32_t size, uint32_t binding, const void* data = nullptr) override;
	virtual void AddData(uint32_t size, const void* data, uint32_t offset = 0) const override;

	virtual uint32_t GetBinding() const override { return mBinding; }

	BufferInfo UboInfo;
	DescriptorSet_Vulkan* UboDescSet;
	VkPipelineLayout* PipelineLayout;
	uint32_t mBinding;
};
