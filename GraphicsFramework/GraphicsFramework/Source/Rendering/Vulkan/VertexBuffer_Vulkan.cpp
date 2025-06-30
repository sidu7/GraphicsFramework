/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "VertexBuffer_Vulkan.h"

#include "Rendering/Vulkan/Renderer_Vulkan.h"

VertexBuffer_Vulkan::VertexBuffer_Vulkan()
{
}

VertexBuffer_Vulkan::~VertexBuffer_Vulkan()
{
	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.Buffer, nullptr);
	StagingBufferInfo.MappedMemory = nullptr;

	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), MainBufferInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), MainBufferInfo.Buffer, nullptr);
	MainBufferInfo.MappedMemory = nullptr;
}

void VertexBuffer_Vulkan::AddData(const void* data, unsigned int size)
{
	CreateBufferInfo(size);

	AddSubData(data, size);
}

void VertexBuffer_Vulkan::AddSubData(const void* data, unsigned int size, unsigned offset)
{
	// Map Memory and copy data
	VKCall(vkMapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory, offset, size, 0, &StagingBufferInfo.MappedMemory), "Vertex Buffer memory mapping Failed.");
	memcpy(StagingBufferInfo.MappedMemory, data, size);
	vkUnmapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory);

	std::function<void()> OnCopyFinish = [this]()
	{
		vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory, nullptr);
		StagingBufferInfo.BufferMemory = VK_NULL_HANDLE;
		vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.Buffer, nullptr);
		StagingBufferInfo.Buffer = VK_NULL_HANDLE;
		StagingBufferInfo.MappedMemory = nullptr;
	};

	Renderer_Vulkan::Get()->QueueCopyCommands(StagingBufferInfo, MainBufferInfo, OnCopyFinish);
}

void VertexBuffer_Vulkan::AddDynamicData(const void* data, unsigned int size)
{
	CreateBufferInfo(size);

	AddSubData(data, size);
}

VkFormat VertexBuffer_Vulkan::GetVertexFormat(VertexFormat format)
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

void VertexBuffer_Vulkan::CreateBufferInfo(uint32_t vertexDataSize)
{
	RenderingFactory_Vulkan* VulkanFactory = static_cast<RenderingFactory_Vulkan*>(RenderingFactory::Instance());
	if (VulkanFactory)
	{
		if (StagingBufferInfo.Buffer == VK_NULL_HANDLE)
		{
			StagingBufferInfo = VulkanFactory->CreateBuffer(vertexDataSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);;
		}
		if (MainBufferInfo.Buffer == VK_NULL_HANDLE)
		{
			MainBufferInfo = VulkanFactory->CreateBuffer(vertexDataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}
}

void VertexBuffer_Vulkan::SetDataLayout(const std::vector<VertexFormat>& vertexFormats)
{
	uint16_t Offset = 0;

	mVertexLayouts.resize(vertexFormats.size());
	for (uint16_t i = 0; i < vertexFormats.size(); ++i)
	{
		mVertexLayouts[i].mOffset = Offset;
		mVertexLayouts[i].mFormat = vertexFormats[i];
		Offset += GetFormatSize(vertexFormats[i]);
	}

	mStride = Offset;
}

void VertexBuffer_Vulkan::Delete() const
{
}