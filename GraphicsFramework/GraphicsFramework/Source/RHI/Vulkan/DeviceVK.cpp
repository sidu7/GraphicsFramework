/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "DeviceVK.h"
// Core
#include "Core/Window.h"
#include "Core/ObjectManager.h"
#include "Core/ShapeManager.h"
#include "Core/Components/Transform.h"
#include "Core/Components/Shape.h"
#include "Core/Components/Material.h"
#include "Core/Object.h"
#include "Core/Engine.h"
#include "Utils/JSONHelper.h"
// Rendering base
#include "Rendering/IndexBuffer.h"
#include "Rendering/VertexBuffer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
// Rendering Vulkan
#include "RHI/Vulkan/FrameBufferVK.h"
#include "RHI/Vulkan/IndexBufferVK.h"
#include "RHI/Vulkan/VertexBufferVK.h"
#include "RHI/Vulkan/UniformBufferVK.h"
#include "RHI/Vulkan/ShaderStorageBufferVK.h"
#include "RHI/Vulkan/ShaderVK.h"
#include "RHI/Vulkan/ComputeShaderVK.h"
#include "RHI/Vulkan/TextureVK.h"
#include "RHI/Vulkan/Internal/RenderPassVK.h"
#include "RHI/Vulkan/Internal/DescriptorPoolVK.h"
// Vulkan Helpers
#include "Internal/HelperVK.h"
#include "Internal/CommandBufferPoolVK.h"
#include "Internal/CommandBufferVK.h"
#include "Internal/CommandQueueVK.h"
#include "Internal/FenceVK.h"
#include "Internal/SemaphoreVK.h"
#include "Internal/SwapChainVK.h"

class VertexArray;
class VertexBuffer;
class IndexBuffer;

namespace rhi::vk
{
ComputeShader* Device::CreateComputeShader()
{
	return new ComputeShaderVK();
}

IndexBuffer* Device::CreateIndexBuffer()
{
	return new IndexBufferVK();
}

FrameBuffer* Device::CreateFrameBuffer()
{
	return new FrameBufferVK();
}

Shader* Device::CreateShader()
{
	return new ShaderVK();
}

ShaderStorageBuffer* Device::CreateShaderStorageBuffer()
{
	return new ShaderStorageBufferVK();
}

Texture* Device::CreateTexture()
{
	return new TextureVK();
}

UniformBuffer* Device::CreateUniformBuffer()
{
	return new UniformBufferVK();
}

VertexBuffer* Device::CreateVertexBuffer()
{
	return new VertexBufferVK();
}

uint32_t Device::FindMemoryType(uint32_t Type, VkMemoryPropertyFlags Properties)
{
	for (int32_t i = 0; i < MemoryProperties.memoryTypeCount; ++i)
	{
		if (Type & (1 << i) && (MemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
		{
			return i;
		}
	}

	std::cout << "Vulkan Failed to find the desired memory type." << std::endl;
	return 0;
}

BufferInfo Device::CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
{
	BufferInfo BufferData;
	BufferData.Size = bufferSize;

	VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferCreateInfo.size = bufferSize;
	BufferCreateInfo.usage = usageFlags;
	Helper::FillSharingMode(BufferCreateInfo.sharingMode, BufferCreateInfo.queueFamilyIndexCount, &BufferCreateInfo.pQueueFamilyIndices);

	uint32_t QueueIndices[] = { GetQueueFamilyIndex(CommandQueueType::Graphics), GetQueueFamilyIndex(CommandQueueType::Transfer) };
	BufferCreateInfo.queueFamilyIndexCount = ARRAY_SIZE(QueueIndices);
	BufferCreateInfo.pQueueFamilyIndices = QueueIndices;

	VKCall(vkCreateBuffer(mDevice, &BufferCreateInfo, nullptr, &BufferData.Buffer), "Buffer creation Failed.");

	VkMemoryRequirements BufferMemoryRequirements;
	vkGetBufferMemoryRequirements(mDevice, BufferData.Buffer, &BufferMemoryRequirements);

	VkMemoryAllocateInfo AllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	AllocInfo.allocationSize = BufferMemoryRequirements.size;
	AllocInfo.memoryTypeIndex = FindMemoryType(BufferMemoryRequirements.memoryTypeBits, memoryPropertyFlags);

	VKCall(vkAllocateMemory(mDevice, &AllocInfo, nullptr, &BufferData.BufferMemory), "Buffer memory allocation Failed.");

	VKCall(vkBindBufferMemory(mDevice, BufferData.Buffer, BufferData.BufferMemory, 0), "Buffer memory binding Failed.");

	return BufferData;
}

ImageInfo Device::CreateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags)
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
	Helper::FillSharingMode(TexImageInfo.sharingMode, TexImageInfo.queueFamilyIndexCount, &TexImageInfo.pQueueFamilyIndices);
	TexImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VKCall(vkCreateImage(mDevice, &TexImageInfo, nullptr, &ImageData.Image), "Failed to create Image.");

	VkMemoryRequirements TexImageMemReq;
	vkGetImageMemoryRequirements(mDevice, ImageData.Image, &TexImageMemReq);

	VkMemoryAllocateInfo TexImageAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	TexImageAllocInfo.allocationSize = TexImageMemReq.size;
	TexImageAllocInfo.memoryTypeIndex = FindMemoryType(TexImageMemReq.memoryTypeBits, memoryPropertyFlags);
	VKCall(vkAllocateMemory(mDevice, &TexImageAllocInfo, nullptr, &ImageData.ImageMemory), "Failed to allocate image memory.");
	VKCall(vkBindImageMemory(mDevice, ImageData.Image, ImageData.ImageMemory, 0), "Failed to bind image memory.");

	return ImageData;
}


#if _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL Device::DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	if (pCallbackData)
	{
		if (Severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			return false;
		}

		const char* SeverityStr = "";
		switch (Severity)
		{
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				SeverityStr = "Error";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				SeverityStr = "Warning";
				break;
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				SeverityStr = "Info";
				break;
		}

		std::cout << "Vulkan " << SeverityStr << " " << pCallbackData->pMessage << std::endl;
	}

	return false;
}
#endif

void Device::Init()
{
	Helper InitHelper;
	
	InitHelper.InitVulkan();

	SwapChain = new SwapChainVK();
	SwapChain->Init();

	// Command Queues
	for (uint32_t i = 0; i < CommandQueueType::Count; ++i)
	{
		CommandQueues[i] = new CommandQueueVK();
		CommandQueues[i]->Init(static_cast<CommandQueueType>(i));
	}

	// Create Command Buffer Pool
	CommandBufferPool = new CommandBufferPoolVK();
	CommandBufferPool->Init(CommandQueueType::Graphics);

	TransientGraphicsCmdBufferPool = new CommandBufferPoolVK();
	TransientGraphicsCmdBufferPool->Init(CommandQueueType::Graphics, true /*isTransient*/);

	TransientTransferCmdBufferPool = new CommandBufferPoolVK();
	TransientTransferCmdBufferPool->Init(CommandQueueType::Transfer, true /*isTransient*/);
	
	ScreenRenderPass = new RenderPassVK();
	ScreenRenderPass->Init(SwapChain->ImageFormat, true, false);

	ImGuiRenderPass = new RenderPassVK();
	ImGuiRenderPass->Init(SwapChain->ImageFormat, false, true);

	for (const VkImageView& ImageView : SwapChain->ImageViews)
	{
		FrameBufferVK* frameBuffer = static_cast<FrameBufferVK*>(CreateFrameBuffer());
		frameBuffer->InitAsBackBuffer(SwapChain->ImageExtent.width, SwapChain->ImageExtent.height, ImageView);
		SwapchainFrameBuffers.push_back(frameBuffer);
	}

	// Create Render Command Buffer
	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		RenderCommandBuffers.push_back(CommandBufferPool->AllocateCommandBuffer());
	}

	// Create Sync Objects
	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		ImageAvailableSemaphores.push_back(new SemaphoreVK());
		RenderingFinishedSemaphores.push_back(new SemaphoreVK());

		InFlightFences.push_back(new FenceVK(true));
	}

	// Create Descriptor Pools from settings
	CreateDescriptorPools();

	ClearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
	ClearValue.depthStencil = { 0.0f, 0 };


	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = SwapChain->ImageExtent.width;
	Viewport.height = SwapChain->ImageExtent.height;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	ScissorRect.extent = SwapChain->ImageExtent;
	ScissorRect.offset = { 0, 0 };

	Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	CullMode = VK_CULL_MODE_NONE;
	FrontFace = VK_FRONT_FACE_CLOCKWISE;
	DepthTestEnable = false;

	ActiveCopyCommandBuffer = nullptr;
}

void Device::Close()
{
	Helper VkHelper;
		
	// Write out descriptor pool data to setting file

	vkDestroyDescriptorPool(mDevice, RendererDescPool->DescPool, nullptr);

	vkDestroyDescriptorPool(mDevice, ImGuiDescPool, nullptr);
	delete ImGuiRenderPass;
	delete ScreenRenderPass;

	for (const FrameBufferVK* FrameBuffer : SwapchainFrameBuffers)
	{
		delete FrameBuffer;
	}

	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		delete RenderCommandBuffers[i];
	}

	for (int32_t i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		delete ImageAvailableSemaphores[i];
		delete RenderingFinishedSemaphores[i];
		delete InFlightFences[i];
	}


	for (uint32_t i = 0; i < CommandQueueType::Count; ++i)
	{
		delete CommandQueues[i];
	}

	delete CommandBufferPool;
	delete TransientTransferCmdBufferPool;
	delete TransientGraphicsCmdBufferPool;

	delete SwapChain;

	vkDestroyDevice(mDevice, nullptr);
	vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
	VkHelper.DestroyDebugMessenger(DebugMessenger);
	vkDestroyInstance(VulkanInstance, nullptr);
}

void Device::Clear() const
{
}

void Device::WaitFrameFinish() const
{
	VKCall(vkDeviceWaitIdle(mDevice), "Wait for device Idle Failed.");
}

void Device::StartFrame()
{
	ExecuteTransientCommandBuffers();

	VKCall(vkWaitForFences(mDevice, 1, &InFlightFences[FrameIdx]->mFence, VK_TRUE, UINT64_MAX), "Waiting for Fence failed.");

	SwapChain->AcquireNextImage(ImageAvailableSemaphores[FrameIdx]);

	InFlightFences[FrameIdx]->Reset();
		
	GetCommandBuffer()->Reset();

	// Record Command Buffer
	GetCommandBuffer()->Begin();
}

void Device::SwapBuffers()
{
	//vkCmdEndRenderPass(GetRenderCommandBuffer());

	GetCommandBuffer()->End();

	// Submit Command Buffer
	GetQueue(CommandQueueType::Graphics)->SubmitCommandBuffer(GetCommandBuffer(), ImageAvailableSemaphores[FrameIdx], RenderingFinishedSemaphores[FrameIdx], InFlightFences[FrameIdx]);

	// Present to Screen
	GetQueue(CommandQueueType::Present)->PresentToScreen(RenderingFinishedSemaphores[FrameIdx], SwapChain);

	FrameIdx = (FrameIdx + 1) % BACKBUFFER_COUNT;
}

void Device::BindShader(const Shader* shader)
{
	const ShaderVK* VkShader = static_cast<const ShaderVK*>(shader);
	if (VkShader)
	{
		vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mPipeline);
	}
}

void Device::BindComputeShader(const ComputeShader* shader)
{
}

void Device::BindFrameBuffer(const FrameBuffer* framebuffer)
{
}

void Device::BindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	if (vertexBuffer)
	{
		VkBuffer Buffers[] = { static_cast<const VertexBufferVK*>(vertexBuffer)->MainBufferInfo.Buffer };
		VkDeviceSize Offsets[] = { 0 };
		vkCmdBindVertexBuffers(GetCommandBuffer()->mCommandBuffer, 0, ARRAY_SIZE(Buffers), Buffers, Offsets);
	}
}

void Device::BindIndexBuffer(const IndexBuffer* indexBuffer)
{
	if (indexBuffer)
	{
		const IndexBufferVK* VkIndexBuffer = static_cast<const IndexBufferVK*>(indexBuffer);
		vkCmdBindIndexBuffer(GetCommandBuffer()->mCommandBuffer, VkIndexBuffer->MainBufferInfo.Buffer, 0, VkIndexBuffer->GetVkIndexType());;
	}
}

void Device::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	const UniformBufferVK* VkUbo = static_cast<const UniformBufferVK*>(uniformBuffer);
	if (VkUbo)
	{
		//vkCmdBindDescriptorSets(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *VkUbo->PipelineLayout, 0, 1, &VkUbo->UboDescSet->DescSet, 0, nullptr);
	}
}

void Device::BindTexture(const Texture* texture, unsigned int slot)
{
}

void Device::UnbindShader(const Shader* shader)
{
}

void Device::UnbindComputeShader(const ComputeShader* shader)
{
}

void Device::UnbindFrameBuffer(const FrameBuffer* framebuffer)
{
}

void Device::UnbindVertexBuffer(const VertexBuffer* vertexBuffer)
{
}

void Device::UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer)
{
}

void Device::UnbindUniformBuffer(const UniformBuffer* uniformBuffer)
{
}

void Device::UnbindTexture(const Texture* texture, unsigned int slot)
{
}

void Device::Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount, int32_t InstanceStart)
{
	ScreenRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBufferVK*>(GetBackBuffer()));

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const ShaderVK*>(Shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);

	vkCmdDraw(GetCommandBuffer()->mCommandBuffer, VertexCount, InstanceCount, VertexStart, InstanceStart);

	ScreenRenderPass->EndPass(GetCommandBuffer());
}

void Device::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	ScreenRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBufferVK*>(GetBackBuffer()));

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const ShaderVK*>(shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);

	vkCmdDrawIndexed(GetCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	ScreenRenderPass->EndPass(GetCommandBuffer());
}

void Device::DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
}

void Device::DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
}

void Device::DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader)
{
}

void Device::DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader)
{
}

void Device::SetDepthTest(bool bEnabled)
{
}

void Device::SetBlending(bool bEnabled)
{
}

void Device::SetCullingFace(CullFace Face)
{
	CullingFace = Face;
}

CullFace Device::GetCullingFace()
{
	return CullingFace;
}

const FrameBuffer* Device::GetBackBuffer()
{
	return SwapchainFrameBuffers[FrameIdx];
}

void Device::SetViewportSize(Rect3D viewport)
{
	Viewport.x = viewport.Offset.x;
	Viewport.y = viewport.Offset.y;
	Viewport.width = viewport.Size.x;
	Viewport.height = viewport.Size.y;
	Viewport.minDepth = viewport.Depth.x;
	Viewport.maxDepth = viewport.Depth.y;
}

void Device::SetScissorSize(Rect2D scissor)
{
	ScissorRect.offset.x = scissor.Offset.x;
	ScissorRect.offset.y = scissor.Offset.y;
	ScissorRect.extent.width = scissor.Size.x;
	ScissorRect.extent.height = scissor.Size.y;
}

Rect3D Device::GetViewportSize()
{
	Rect3D OutViewport;
	OutViewport.Offset = glm::vec2(Viewport.x, Viewport.y);
	OutViewport.Size = glm::vec2(Viewport.width, Viewport.height);
	OutViewport.Depth = glm::vec2(Viewport.minDepth, Viewport.maxDepth);
	return OutViewport;
}

Rect2D Device::GetScissorSize()
{
	Rect2D OutScissor;
	OutScissor.Offset = glm::vec2(ScissorRect.offset.x, ScissorRect.offset.y);
	OutScissor.Size = glm::vec2(ScissorRect.extent.width, ScissorRect.extent.height);
	return OutScissor;
}

void Device::ExecuteTransientCommandBuffers()
{
	if (ActiveCopyCommandBuffer)
	{
		ActiveCopyCommandBuffer->End();

		FenceVK* CopyFence = new FenceVK();

		GetQueue(CommandQueueType::Transfer)->SubmitCommandBuffer(ActiveCopyCommandBuffer, nullptr, nullptr, CopyFence);

		CopyFence->Wait();

		for (const OnCommandExecutionEnd& Func : ActiveCopyCommandBuffer->ExecutionFinishDelegates)
		{
			Func();
		}

		delete CopyFence;
		delete ActiveCopyCommandBuffer;
		ActiveCopyCommandBuffer = nullptr;
	}
}

CommandBufferVK* Device::GetCopyCommandBuffer()
{
	if (!ActiveCopyCommandBuffer)
	{
		ActiveCopyCommandBuffer = TransientTransferCmdBufferPool->AllocateCommandBuffer();
		ActiveCopyCommandBuffer->Begin();
	}
	return ActiveCopyCommandBuffer;
}

void Device::QueueCopyCommands(const BufferInfo& srcBuffer, const BufferInfo& dstBuffer, const OnCommandExecutionEnd& FinishDelegate)
{
	CommandBufferVK* CopyCommandBuffer = GetCopyCommandBuffer();

	VkBufferCopy CopyRegion = {};
	CopyRegion.dstOffset = 0;
	CopyRegion.srcOffset = 0;
	CopyRegion.size = srcBuffer.Size;

	if (FinishDelegate)
	{
		CopyCommandBuffer->ExecutionFinishDelegates.push_back(FinishDelegate);
	}

	vkCmdCopyBuffer(CopyCommandBuffer->mCommandBuffer, srcBuffer.Buffer, dstBuffer.Buffer, 1, &CopyRegion);
}

CommandBufferVK* Device::AllocateTransientCommandBuffer(CommandQueueType type)
{
	switch (type)
	{
		case CommandQueueType::Graphics:
			return TransientGraphicsCmdBufferPool->AllocateCommandBuffer();
		break;

		case CommandQueueType::Transfer:
			return TransientTransferCmdBufferPool->AllocateCommandBuffer();
		break;

		default:
			std::cout << "Unsupported transient command buffer requrested" << std::endl;
		break;
	}
	return nullptr;
}

void Device::CreateDescriptorPools()
{
	RendererDescPool = new DescriptorPoolVK();

	const std::string& SettingsFilePath = Engine::Instance()->GetSettingsFilePath();
	rapidjson::Document SettingsFile = JSONHelper::ParseFile(SettingsFilePath);
	if (!SettingsFile.HasParseError())
	{
		rapidjson::Value::Object Root = SettingsFile.GetObject();
		if (Root.HasMember("RenderingSettings"))
		{
			rapidjson::Value::Object RenderingSettings = Root["RenderingSettings"].GetObject();
			if (RenderingSettings.HasMember("DescriptorPools"))
			{
				rapidjson::Value::Array DescriptorPoolSettings = RenderingSettings["DescriptorPools"].GetArray();
				for (uint32_t i = 0; i < DescriptorPoolSettings.Size(); ++i)
				{
					rapidjson::Value::Object DescTypeInfo = DescriptorPoolSettings[i].GetObject();
					if (DescTypeInfo.HasMember("Type") && DescTypeInfo.HasMember("TypeId") && DescTypeInfo.HasMember("Count"))
					{
						std::string typeIdStr = string_VkDescriptorType(static_cast<VkDescriptorType>(DescTypeInfo["TypeId"].GetInt()));
						std::string typeStr = DescTypeInfo["Type"].GetString();
						if (typeIdStr == typeStr)
						{
							RendererDescPool->AddPoolType(static_cast<VkDescriptorType>(DescTypeInfo["TypeId"].GetInt()), DescTypeInfo["Count"].GetInt());
						}
					}
				}
			}
		}
		// Create data with basic pool info
		else
		{
			const std::vector<std::pair<VkDescriptorType, uint32_t>> PoolData = {
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10 }
			};

			/*FILE* JsonFile = fopen(SettingsFilePath.c_str(), "w");
			char buffer[1024];
			rapidjson::FileWriteStream fileStream(JsonFile, buffer, sizeof(buffer));
			rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer = rapidjson::PrettyWriter<rapidjson::FileWriteStream>(fileStream);
			rapidjson::Document::AllocatorType& Alloc = SettingsFile.GetAllocator();
		
			rapidjson::Value RendererSettings;
			RendererSettings.SetObject();
			SettingsFile.AddMember("RendererSettings", RendererSettings, Alloc);
			rapidjson::Value DescriptorPools;
			DescriptorPools.SetArray();
			RendererSettings.AddMember("DescriptorPools", DescriptorPools, Alloc);*/
		
			for (uint32_t i = 0; i < PoolData.size(); ++i)
			{
				const std::pair<VkDescriptorType, uint32_t>& Pool = PoolData[i];

				/*rapidjson::Value PoolInfo;
				PoolInfo.SetObject();
				rapidjson::Value DescType;
				
				DescType.SetString(string_VkDescriptorType(Pool.first), Alloc);
				PoolInfo.AddMember("Type", DescType, Alloc);
				
				DescType.SetInt(static_cast<int>(Pool.first));
				PoolInfo.AddMember("TypeId", DescType, Alloc);
				DescType.SetInt(Pool.second);
				PoolInfo.AddMember("Count", DescType, Alloc); 
				DescriptorPools.PushBack(PoolInfo, Alloc);*/

				RendererDescPool->AddPoolType(Pool.first, Pool.second);
			}		
			
			/*SettingsFile.Accept(writer);
			fclose(JsonFile);*/
		}
	}

	RendererDescPool->Init();
}
}
