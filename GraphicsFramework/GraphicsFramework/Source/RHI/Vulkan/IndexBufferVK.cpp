#include "IndexBufferVK.h"

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
IndexBufferVK::IndexBufferVK() :
	m_Count(0),
	mType(IndexType::UInt16)
{
}

IndexBufferVK::~IndexBufferVK()
{
	Delete();
}

void IndexBufferVK::Delete()
{
	Device* VkDev = Device::Get();

	VkDev->DestroyBuffer(StagingBufferInfo);
	VkDev->DestroyBuffer(MainBufferInfo);
}

void IndexBufferVK::AddData(const void* data, unsigned int count, IndexType type)
{
	m_Count = count;
	mType = type;
	uint32_t bufferSize = m_Count * GetIndexTypeSize(mType);

	if (StagingBufferInfo.Buffer == VK_NULL_HANDLE)
	{
		StagingBufferInfo = Device::Get()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true /*bHostVisible*/);
	}
	if (MainBufferInfo.Buffer == VK_NULL_HANDLE)
	{
		MainBufferInfo = Device::Get()->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false /*bHostVisible*/);
	}

	// Map Memory and copy data
	void* MappedData = Device::Get()->MapBuffer(StagingBufferInfo);
	memcpy(MappedData, data, bufferSize);
	Device::Get()->UnmapBuffer(StagingBufferInfo);

	std::function<void()> OnCopyFinish = [this]()
	{
		Device::Get()->DestroyBuffer(StagingBufferInfo);
	};

	Device::Get()->QueueCopyCommands(StagingBufferInfo, MainBufferInfo, OnCopyFinish);
}
}
