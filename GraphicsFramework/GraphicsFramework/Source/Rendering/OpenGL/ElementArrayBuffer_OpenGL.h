#pragma once
#include "Rendering/ElementArrayBuffer.h"

class ElementArrayBuffer_OpenGL : public ElementArrayBuffer
{
public:
	ElementArrayBuffer_OpenGL();
	~ElementArrayBuffer_OpenGL();

	virtual void AddData(const void* data, unsigned int count, unsigned int size) override;
	virtual inline unsigned int GetCount() const override { return m_Count; }

	unsigned int m_RendererID;
	unsigned int m_Count;
};
