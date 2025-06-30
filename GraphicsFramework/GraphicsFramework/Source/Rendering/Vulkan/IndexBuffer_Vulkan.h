#pragma once
#include "Rendering/IndexBuffer.h"

#include "Rendering/Vulkan/RenderingFactory_Vulkan.h"

class IndexBuffer_Vulkan : public IndexBuffer
{
public:
	IndexBuffer_Vulkan();
	~IndexBuffer_Vulkan();

	virtual void AddData(const void* data, unsigned int count, IndexType type) override;
	virtual inline unsigned int GetCount() const override { return m_Count; }

	VkIndexType GetVkIndexType() const
	{
		switch (mType)
		{
			case IndexType::UInt8:
				return VK_INDEX_TYPE_UINT8_EXT;
			case IndexType::UInt16:
				return VK_INDEX_TYPE_UINT16;
			case IndexType::UInt32:
				return VK_INDEX_TYPE_UINT32;
		}

		return VK_INDEX_TYPE_NONE_KHR;
	}

	BufferInfo StagingBufferInfo;
	BufferInfo MainBufferInfo;

	IndexType mType;
	unsigned int m_Count;
};
