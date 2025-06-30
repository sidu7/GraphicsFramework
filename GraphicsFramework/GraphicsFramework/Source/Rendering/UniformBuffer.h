#pragma once
#include <stdint.h>

class UniformBuffer
{
public:
	virtual ~UniformBuffer() {}

	virtual void Init(uint32_t size, uint32_t binding, const void* data = nullptr) = 0;
	virtual void AddData(uint32_t size, const void* data, uint32_t offset = 0) const = 0;

	virtual uint32_t GetBinding() const = 0;
};
