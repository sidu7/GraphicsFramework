#pragma once
#include "Renderer.h"

class ElementArrayBuffer
{
private:
	unsigned int m_RendererID;
public:
	ElementArrayBuffer();
	~ElementArrayBuffer();

	void AddData(const void* data, unsigned int size) const;
	void Bind() const;
	void Unbind() const;
};