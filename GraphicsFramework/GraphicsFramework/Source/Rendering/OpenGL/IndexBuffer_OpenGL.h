#pragma once
#include "Rendering/IndexBuffer.h"

class IndexBuffer_OpenGL : public IndexBuffer
{
public:
	IndexBuffer_OpenGL();
	~IndexBuffer_OpenGL();

	virtual void AddData(const void* data, unsigned int count, IndexType type) override;
	virtual inline unsigned int GetCount() const override { return m_Count; }

	unsigned int m_RendererID;
	unsigned int m_Count;
};
