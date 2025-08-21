#pragma once

#include "Core/Core.h"
#include "Rendering/RenderingFactory.h"
#include <vulkan/vulkan.h>

#include "Internal/DescriptorSet_Vulkan.h"

class DescriptorSet_Vulkan;
class DescriptorPool_Vulkan;

struct BufferInfo
{
	BufferInfo() : Buffer(VK_NULL_HANDLE), BufferMemory(VK_NULL_HANDLE), Size(0), MappedMemory(nullptr) {}

	VkBuffer Buffer;
	VkDeviceMemory BufferMemory;
	uint32_t Size;
	void* MappedMemory;
};

struct ImageInfo
{
	ImageInfo() : Image(VK_NULL_HANDLE), ImageMemory(VK_NULL_HANDLE), Size(0) {}

	VkImage Image;
	VkDeviceMemory ImageMemory;
	uint32_t Size;
};

class RenderingFactory_Vulkan : public RenderingFactory
{
public:
	virtual std::string GetApiName() { return "VULKAN"; }

	virtual void Init() override;
	virtual void Close() override;

	virtual ComputeShader* CreateComputeShader();
	virtual IndexBuffer* CreateIndexBuffer();
	virtual FrameBuffer* CreateFrameBuffer();
	virtual Shader* CreateShader();
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer();
	virtual Texture* CreateTexture();
	virtual UniformBuffer* CreateUniformBuffer();
	virtual VertexBuffer* CreateVertexBuffer();

	virtual BufferInfo CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);
	virtual ImageInfo CreateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags);

	virtual DescriptorSet_Vulkan* AllocateDescriptorSet(VkDescriptorType type, uint16_t count, uint32_t binding);

protected:
	uint32_t FindMemoryType(uint32_t Type, VkMemoryPropertyFlags Properties);

	std::vector<DescriptorPool_Vulkan*> DescriptorPools;
	uint32_t NewPoolSize = 10;
};