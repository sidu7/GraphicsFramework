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
	EnsureCapacity(size);
	if (data && size > 0)
	{
		WriteAt(data, size, 0);
	}
}

void VertexBufferVK::AddSubData(const void* data, unsigned int size, unsigned offset)
{
	// A null pointer means "allocate/reserve only" (that is how AddDynamicData is used).
	if (data == nullptr || size == 0)
	{
		EnsureCapacity(offset + size);
		return;
	}

	EnsureCapacity(offset + size);
	WriteAt(data, size, offset);
}

void VertexBufferVK::AddDynamicData(const void* data, unsigned int size)
{
	EnsureCapacity(size);
	if (data && size > 0)
	{
		WriteAt(data, size, 0);
	}
}

VkFormat VertexBufferVK::GetVertexFormat(VertexFormat format)
{
	switch (format)
	{
		case VertexFormat::Bool:   return VK_FORMAT_R8_UNORM;
		case VertexFormat::UInt:   return VK_FORMAT_R32_UINT;
		case VertexFormat::Int:    return VK_FORMAT_R32_SINT;
		case VertexFormat::iVec2:  return VK_FORMAT_R32G32_SINT;
		case VertexFormat::iVec3:  return VK_FORMAT_R32G32B32_SINT;
		case VertexFormat::iVec4:  return VK_FORMAT_R32G32B32A32_SINT;
		case VertexFormat::Float:  return VK_FORMAT_R32_SFLOAT;
		case VertexFormat::Vec2:   return VK_FORMAT_R32G32_SFLOAT;
		case VertexFormat::Vec3:   return VK_FORMAT_R32G32B32_SFLOAT;
		case VertexFormat::Vec4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
		case VertexFormat::Mat2:
		case VertexFormat::Mat3:
		case VertexFormat::Mat4:
		default:                   return VK_FORMAT_UNDEFINED;
	}
}

void VertexBufferVK::EnsureCapacity(uint32_t requiredSize)
{
	if (requiredSize == 0)
	{
		return;
	}

	if (MainBufferInfo.Buffer != VK_NULL_HANDLE && MainBufferInfo.Size >= requiredSize)
	{
		return;
	}

	Device* VkDev = Device::Get();
	if (MainBufferInfo.Buffer != VK_NULL_HANDLE)
	{
		VkDev->DestroyBuffer(MainBufferInfo);
	}

	// Grow in powers of two so a per-frame upload pattern does not reallocate.
	uint32_t Capacity = MainBufferInfo.Size > 0 ? MainBufferInfo.Size : 256;
	while (Capacity < requiredSize)
	{
		Capacity *= 2;
	}

	MainBufferInfo = VkDev->CreateBuffer(Capacity,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		true /*bHostVisible*/);
}

void VertexBufferVK::WriteAt(const void* data, unsigned int size, unsigned int offset)
{
	Device* VkDev = Device::Get();

	uint8_t* Destination = static_cast<uint8_t*>(VkDev->MapBuffer(MainBufferInfo, offset));
	memcpy(Destination, data, size);
	VkDev->UnmapBuffer(MainBufferInfo);

	// vmaFlushAllocation is valid on host-visible memory whether or not it is coherent.
	VkDev->FlushBuffer(MainBufferInfo, offset, size);
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
