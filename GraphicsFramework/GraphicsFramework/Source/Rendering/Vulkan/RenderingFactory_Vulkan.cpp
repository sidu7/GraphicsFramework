#include "RenderingFactory_Vulkan.h"

#include "Rendering/Vulkan/ComputeShader_Vulkan.h"
#include "Rendering/Vulkan/IndexBuffer_Vulkan.h"
#include "Rendering/Vulkan/FrameBuffer_Vulkan.h"
#include "Rendering/Vulkan/Shader_Vulkan.h"
#include "Rendering/Vulkan/ShaderStorageBuffer_Vulkan.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"
#include "Rendering/Vulkan/UniformBuffer_Vulkan.h"
#include "Rendering/Vulkan/VertexBuffer_Vulkan.h"

#include "Rendering/Vulkan/Internal/DescriptorPool_Vulkan.h"
#include "Rendering/Vulkan/Internal/DescriptorSet_Vulkan.h"

void RenderingFactory_Vulkan::Init()
{

}

void RenderingFactory_Vulkan::Close()
{
	for (DescriptorPool_Vulkan* Pool : DescriptorPools)
	{
		delete Pool;
	}
}

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
	BufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

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

DescriptorSet_Vulkan* RenderingFactory_Vulkan::AllocateDescriptorSet(VkDescriptorType type, uint16_t count, uint32_t binding)
{
	DescriptorPool_Vulkan* CompatiblePool = nullptr;
	for (DescriptorPool_Vulkan* Pool : DescriptorPools)
	{
		if (Pool->Supports(type, count))
		{
			CompatiblePool = Pool;
		}
	}

	if (!CompatiblePool)
	{
		CompatiblePool = new DescriptorPool_Vulkan();
		CompatiblePool->Init({ type, NewPoolSize });
		DescriptorPools.push_back(CompatiblePool);
	}

	return CompatiblePool->AllocateDescriptor(count, binding);
}
