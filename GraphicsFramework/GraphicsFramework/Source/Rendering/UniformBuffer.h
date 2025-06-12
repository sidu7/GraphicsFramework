#pragma once

class UniformBuffer
{
public:
	virtual ~UniformBuffer() {}

	virtual void Init(unsigned int size, const void* data = nullptr) = 0;
	virtual void AddData(unsigned int size, const void* data, unsigned int offset = 0) const = 0;
};
