#pragma once
#include "Rendering/IndexBuffer.h"

namespace rhi::gl
{
class IndexBufferGL : public IndexBuffer
{
public:
	IndexBufferGL();
	~IndexBufferGL();

	virtual void AddData(const void* data, unsigned int count, IndexType type) override;
	virtual inline unsigned int GetCount() const override { return m_Count; }

	unsigned int m_RendererID;
	unsigned int m_Count;
};
}
