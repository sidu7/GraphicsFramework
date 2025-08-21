#include "RenderingFactory_Vulkan.h"

#include "Rendering/Vulkan/ComputeShader_Vulkan.h"
#include "Rendering/Vulkan/IndexBuffer_Vulkan.h"
#include "Rendering/Vulkan/FrameBuffer_Vulkan.h"
#include "Rendering/Vulkan/Shader_Vulkan.h"
#include "Rendering/Vulkan/ShaderStorageBuffer_Vulkan.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"
#include "Rendering/Vulkan/UniformBuffer_Vulkan.h"
#include "Rendering/Vulkan/VertexBuffer_Vulkan.h"

#include "Rendering/Vulkan/Internal/VulkanHelper.h"
#include "Rendering/Vulkan/Internal/DescriptorPool_Vulkan.h"
#include "Rendering/Vulkan/Internal/DescriptorSet_Vulkan.h"

ComputeShader* RenderingFactory_Vulkan::CreateComputeShader()
{
	return new ComputeShader_Vulkan();
}

IndexBuffer* RenderingFactory_Vulkan::CreateIndexBuffer()
{
	return new IndexBuffer_Vulkan();
}

FrameBuffer* RenderingFactory_Vulkan::CreateFrameBuffer()
{
	return new FrameBuffer_Vulkan();
}

Shader* RenderingFactory_Vulkan::CreateShader()
{
	return new Shader_Vulkan();
}

ShaderStorageBuffer* RenderingFactory_Vulkan::CreateShaderStorageBuffer()
{
	return new ShaderStorageBuffer_Vulkan();
}

Texture* RenderingFactory_Vulkan::CreateTexture()
{
	return new Texture_Vulkan();
}

UniformBuffer* RenderingFactory_Vulkan::CreateUniformBuffer()
{
	return new UniformBuffer_Vulkan();
}

VertexBuffer* RenderingFactory_Vulkan::CreateVertexBuffer()
{
	return new VertexBuffer_Vulkan();
}

uint32_t RenderingFactory_Vulkan::FindMemoryType(uint32_t Type, VkMemoryPropertyFlags Properties)
{
	for (int32_t i = 0; i < Renderer_Vulkan::Get()->MemoryProperties.memoryTypeCount; ++i)
	{
		if (Type & (1 << i) && (Renderer_Vulkan::Get()->MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
		{
			return i;
		}
	}

	std::cout << "Vulkan Failed to find the desired memory type." << std::endl;
	return -1;
}

BufferInfo RenderingFactory_Vulkan::CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
{
	BufferInfo BufferData;
	BufferData.Size = bufferSize;

	VkBufferCreateInfo BufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferInfo.size = bufferSize;
	BufferInfo.usage = usageFlags;
	VulkanHelper::FillSharingMode(BufferInfo.sharingMode, BufferInfo.queueFamilyIndexCount, &BufferInfo.pQueueFamilyIndices);

	uint32_t QueueIndices[] = { Renderer_Vulkan::Get()->GetQueueFamilyIndex(CommandQueueType::Graphics), Renderer_Vulkan::Get()->GetQueueFamilyIndex(CommandQueueType::Transfer) };
	BufferInfo.queueFamilyIndexCount = ARRAY_SIZE(QueueIndices);
	BufferInfo.pQueueFamilyIndices = QueueIndices;

	VKCall(vkCreateBuffer(Renderer_Vulkan::Get()->GetDevice(), &BufferInfo, nullptr, &BufferData.Buffer), "Buffer creation Failed.");

	VkMemoryRequirements VBMemoryRequirements;
	vkGetBufferMemoryRequirements(Renderer_Vulkan::Get()->GetDevice(), BufferData.Buffer, &VBMemoryRequirements);

	VkMemoryAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	AllocInfo.allocationSize = VBMemoryRequirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(VBMemoryRequirements.memoryTypeBits, memoryPropertyFlags);

	VKCall(vkAllocateMemory(Renderer_Vulkan::Get()->GetDevice(), &AllocInfo, nullptr, &BufferData.BufferMemory), "Buffer memory allocation Failed.");

	VKCall(vkBindBufferMemory(Renderer_Vulkan::Get()->GetDevice(), BufferData.Buffer, BufferData.BufferMemory, 0), "Buffer memory binding Failed.");

	return BufferData;
}

ImageInfo RenderingFactory_Vulkan::CreateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
{
	ImageInfo ImageData;

	VkImageCreateInfo TexImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	TexImageInfo.imageType = VK_IMAGE_TYPE_2D;
	TexImageInfo.format = imageFormat;
	TexImageInfo.extent.width = width;
	TexImageInfo.extent.height = height;
	TexImageInfo.extent.depth = 1;
	TexImageInfo.mipLevels = 1;
	TexImageInfo.arrayLayers = 1;
	TexImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	TexImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	TexImageInfo.usage = usageFlags;
	VulkanHelper::FillSharingMode(TexImageInfo.sharingMode, TexImageInfo.queueFamilyIndexCount, &TexImageInfo.pQueueFamilyIndices);
	TexImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VKCall(vkCreateImage(Renderer_Vulkan::Get()->GetDevice(), &TexImageInfo, nullptr, &ImageData.Image), "Failed to create Image.");

	VkMemoryRequirements TexImageMemReq;
	vkGetImageMemoryRequirements(Renderer_Vulkan::Get()->GetDevice(), ImageData.Image, &TexImageMemReq);

	VkMemoryAllocateInfo TexImageAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	TexImageAllocInfo.allocationSize = TexImageMemReq.size;
	TexImageAllocInfo.memoryTypeIndex = FindMemoryType(TexImageMemReq.memoryTypeBits, memoryPropertyFlags);
	VKCall(vkAllocateMemory(Renderer_Vulkan::Get()->GetDevice(), &TexImageAllocInfo, nullptr, &ImageData.ImageMemory), "Failed to allocate image memory.");
	VKCall(vkBindImageMemory(Renderer_Vulkan::Get()->GetDevice(), ImageData.Image, ImageData.ImageMemory, 0), "Failed to bind image memory.");

	return ImageData;
}