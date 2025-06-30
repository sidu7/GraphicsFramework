#pragma once
#include "Rendering/Renderer.h"

enum class IndexType
{
	UInt8,
	UInt16,
	UInt32
};

class IndexBuffer
{
public:
	virtual ~IndexBuffer() {}

	virtual void AddData(const void* data, unsigned int count, IndexType type) = 0;
	virtual inline unsigned int GetCount() const = 0;

	static uint32_t GetIndexTypeSize(IndexType type)
	{
		switch (type)
		{
			case IndexType::UInt8:
				return sizeof(uint8_t);
			case IndexType::UInt16:
				return sizeof(uint16_t);
			case IndexType::UInt32:
				return sizeof(uint32_t);
		}

		return 0;
	}
};
