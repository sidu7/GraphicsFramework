#pragma once
#include "Rendering/Renderer.h"

class ElementArrayBuffer
{
public:
	virtual ~ElementArrayBuffer() {}

	virtual void AddData(const void* data, unsigned int count, unsigned int size) = 0;
	virtual inline unsigned int GetCount() const = 0;
};
