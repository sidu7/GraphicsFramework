#pragma once

#include "Rendering/UniformBuffer.h"

class UniformBuffer_OpenGL : public UniformBuffer
{
public:
	UniformBuffer_OpenGL();
	virtual ~UniformBuffer_OpenGL();

	virtual void Init(uint32_t size, uint32_t binding, const void* data = nullptr) override;
	virtual void AddData(uint32_t size, const void* data, uint32_t offset = 0) const override;

	unsigned int m_RendererId;
};
