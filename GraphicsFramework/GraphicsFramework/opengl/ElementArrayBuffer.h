#pragma once
#include "Renderer.h"

class ElementArrayBuffer
{
private:
	unsigned int m_RendererID;
	unsigned int count;
public:
	ElementArrayBuffer();
	~ElementArrayBuffer();

	void AddData(const void* data, unsigned int count, unsigned int size);
	void Bind() const;
	void Unbind() const;
	inline unsigned int GetCount() const { return count; }
};