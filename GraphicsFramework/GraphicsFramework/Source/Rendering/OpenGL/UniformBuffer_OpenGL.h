#pragma once

#include "Rendering/UniformBuffer.h"

class UniformBuffer_OpenGL : public UniformBuffer
{
public:
	UniformBuffer_OpenGL();
	virtual ~UniformBuffer_OpenGL();

	virtual void Init(unsigned int size, const void* data = nullptr) override;
	virtual void AddData(unsigned int size, const void* data, unsigned int offset = 0) const override;

	unsigned int m_RendererId;
};
