#include "IndexBuffer_Vulkan.h"

#include "Rendering/Vulkan/Renderer_Vulkan.h"

IndexBuffer_Vulkan::IndexBuffer_Vulkan() :
	m_Count(0),
	mType(IndexType::UInt16)
{
}

IndexBuffer_Vulkan::~IndexBuffer_Vulkan()
{
	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.Buffer, nullptr);
	StagingBufferInfo.MappedMemory = nullptr;

	vkFreeMemory(Renderer_Vulkan::Get()->GetDevice(), MainBufferInfo.BufferMemory, nullptr);
	vkDestroyBuffer(Renderer_Vulkan::Get()->GetDevice(), MainBufferInfo.Buffer, nullptr);
	MainBufferInfo.MappedMemory = nullptr;
}

void IndexBuffer_Vulkan::AddData(const void* data, unsigned int count, IndexType type)
{
	m_Count = count;
	mType = type;
	uint32_t bufferSize = m_Count * GetIndexTypeSize(mType);

	RenderingFactory_Vulkan* VulkanFactory = static_cast<RenderingFactory_Vulkan*>(RenderingFactory::Instance());
	if (VulkanFactory)
	{
		if (StagingBufferInfo.Buffer == VK_NULL_HANDLE)
		{
			StagingBufferInfo = VulkanFactory->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);;
		}
		if (MainBufferInfo.Buffer == VK_NULL_HANDLE)
		{
			MainBufferInfo = VulkanFactory->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}

	// Map Memory and copy data
	VKCall(vkMapMemory(Renderer_Vulkan::Get()->GetDevice(), StagingBufferInfo.BufferMemory, 0, bufferSize, 0, &StagingBufferInfo.MappedMemory), "Index Buffer memory mapping Failed.");
	memcpy(StagingBufferInfo.MappedMemory, data, bufferSize);
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
