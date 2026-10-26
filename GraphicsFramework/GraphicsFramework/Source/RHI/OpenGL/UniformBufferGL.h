#pragma once

#include "Rendering/UniformBuffer.h"

namespace rhi::gl
{
class UniformBufferGL : public UniformBuffer
{
public:
	UniformBufferGL();
	virtual ~UniformBufferGL();

	virtual void Init(uint32_t size, uint32_t binding, const void* data = nullptr) override;
	virtual void AddData(uint32_t size, const void* data, uint32_t offset = 0) const override;

	virtual uint32_t GetBinding() const override { return mBinding; };

	unsigned int m_RendererId;
	uint32_t mBinding;
};
}
