/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexBufferVK.h"

#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
VertexBufferVK::VertexBufferVK()
{
}

VertexBufferVK::~VertexBufferVK()
{
	Delete();
}

void VertexBufferVK::AddData(const void* data, unsigned int size)
{
	CreateBufferInfo(size);

	AddSubData(data, size);
}

void VertexBufferVK::AddSubData(const void* data, unsigned int size, unsigned offset)
{
	// Map Memory and copy data
	void* MappedData = Device::Get()->MapBuffer(StagingBufferInfo, offset);
	memcpy(MappedData, data, size);
	Device::Get()->UnmapBuffer(StagingBufferInfo);

	std::function<void()> OnCopyFinish = [this]()
	{
		Device::Get()->DestroyBuffer(StagingBufferInfo);
	};

	Device::Get()->QueueCopyCommands(StagingBufferInfo, MainBufferInfo, OnCopyFinish);
}

void VertexBufferVK::AddDynamicData(const void* data, unsigned int size)
{
	CreateBufferInfo(size);

	AddSubData(data, size);
}

VkFormat VertexBufferVK::GetVertexFormat(VertexFormat format)
{
	switch (format)
	{
		case VertexFormat::Bool:
			return VK_FORMAT_R8_UNORM;
			break;
		case VertexFormat::UInt:
			return VK_FORMAT_R32_UINT;
			break;
		case VertexFormat::Int:
			return VK_FORMAT_R32_SINT;
			break;
		case VertexFormat::iVec2:
			return VK_FORMAT_R32G32_SINT;
			break;
		case VertexFormat::iVec3:
			return VK_FORMAT_R32G32B32_SINT;
			break;
		case VertexFormat::iVec4:
			return VK_FORMAT_R32G32B32A32_SINT;
			break;
		case VertexFormat::Float:
			return VK_FORMAT_R32_SFLOAT;
			break;
		case VertexFormat::Vec2:
			return VK_FORMAT_R32G32_SFLOAT;
			break;
		case VertexFormat::Vec3:
			return VK_FORMAT_R32G32B32_SFLOAT;
			break;
		case VertexFormat::Vec4:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
			break;
		case VertexFormat::Mat2:
		case VertexFormat::Mat3:
		case VertexFormat::Mat4:
		default:
			return VK_FORMAT_UNDEFINED;
			break;
	}
	return VkFormat();
}

void VertexBufferVK::CreateBufferInfo(uint32_t vertexDataSize)
{
	if (StagingBufferInfo.Buffer == VK_NULL_HANDLE)
	{
		StagingBufferInfo = Device::Get()->CreateBuffer(vertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true /*bHostVisible*/);
	}
	if (MainBufferInfo.Buffer == VK_NULL_HANDLE)
	{
		MainBufferInfo = Device::Get()->CreateBuffer(vertexDataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, false /*bHostVisible*/);
	}
}

void VertexBufferVK::SetDataLayout(const std::vector<VertexFormat>& vertexFormats)
{
	uint16_t Offset = 0;

	mVertexLayouts.clear();
	mVertexLayouts.resize(vertexFormats.size());
	for (uint16_t i = 0; i < vertexFormats.size(); ++i)
	{
		mVertexLayouts[i].mOffset = Offset;
		mVertexLayouts[i].mFormat = vertexFormats[i];
		Offset += GetFormatSize(vertexFormats[i]);
	}

	mStride = Offset;
}

void VertexBufferVK::Delete()
{
	Device* VkDev = Device::Get();

	VkDev->DestroyBuffer(StagingBufferInfo);
	VkDev->DestroyBuffer(MainBufferInfo);
}
}
