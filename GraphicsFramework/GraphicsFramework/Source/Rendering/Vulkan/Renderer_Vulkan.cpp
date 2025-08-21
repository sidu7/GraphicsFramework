/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Renderer_Vulkan.h"
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
#include "Rendering/RenderingFactory.h"
// Rendering Vulkan
#include "Rendering/Vulkan/FrameBuffer_Vulkan.h"
#include "Rendering/Vulkan/IndexBuffer_Vulkan.h"
#include "Rendering/Vulkan/VertexBuffer_Vulkan.h"
#include "Rendering/Vulkan/UniformBuffer_Vulkan.h"
#include "Rendering/Vulkan/Shader_Vulkan.h"
#include "Rendering/Vulkan/ComputeShader_Vulkan.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"
#include "Rendering/Vulkan/Internal/RenderPass_Vulkan.h"
#include "Rendering/Vulkan/Internal/DescriptorPool_Vulkan.h"
// Vulkan Helpers
#include "Internal/VulkanHelper.h"
#include "Internal/CommandBufferPool_Vulkan.h"
#include "Internal/CommandBuffer_Vulkan.h"
#include "Internal/CommandQueue_Vulkan.h"
#include "Internal/Fence_Vulkan.h"
#include "Internal/Semaphore_Vulkan.h"
#include "Internal/SwapChain_Vulkan.h"

class VertexArray;
class VertexBuffer;
class IndexBuffer;

#if _DEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL Renderer_Vulkan::DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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

void Renderer_Vulkan::Init()
{
	VulkanHelper InitHelper;
	
	InitHelper.InitVulkan();

	SwapChain = new SwapChain_Vulkan();
	SwapChain->Init();

	// Command Queues
	for (uint32_t i = 0; i < CommandQueueType::Count; ++i)
	{
		CommandQueues[i] = new CommandQueue_Vulkan();
		CommandQueues[i]->Init(static_cast<CommandQueueType>(i));
	}

	// Create Command Buffer Pool
	CommandBufferPool = new CommandBufferPool_Vulkan();
	CommandBufferPool->Init(CommandQueueType::Graphics);

	TransientGraphicsCmdBufferPool = new CommandBufferPool_Vulkan();
	TransientGraphicsCmdBufferPool->Init(CommandQueueType::Graphics, true /*isTransient*/);

	TransientTransferCmdBufferPool = new CommandBufferPool_Vulkan();
	TransientTransferCmdBufferPool->Init(CommandQueueType::Transfer, true /*isTransient*/);
	
	ScreenRenderPass = new RenderPass_Vulkan();
	ScreenRenderPass->Init(SwapChain->ImageFormat, true, false);

	ImGuiRenderPass = new RenderPass_Vulkan();
	ImGuiRenderPass->Init(SwapChain->ImageFormat, false, true);

	for (const VkImageView& ImageView : SwapChain->ImageViews)
	{
		FrameBuffer_Vulkan* frameBuffer = static_cast<FrameBuffer_Vulkan*>(RenderingFactory::Instance()->CreateFrameBuffer());
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
		ImageAvailableSemaphores.push_back(new Semaphore_Vulkan());
		RenderingFinishedSemaphores.push_back(new Semaphore_Vulkan());

		InFlightFences.push_back(new Fence_Vulkan(true));
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

void Renderer_Vulkan::Close()
{
	VulkanHelper VkHelper;
		
	// Write out descriptor pool data to setting file

	vkDestroyDescriptorPool(Device, RendererDescPool->DescPool, nullptr);

	vkDestroyDescriptorPool(Device, ImGuiDescPool, nullptr);
	delete ImGuiRenderPass;
	delete ScreenRenderPass;

	for (const FrameBuffer_Vulkan* FrameBuffer : SwapchainFrameBuffers)
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

	vkDestroyDevice(Device, nullptr);
	vkDestroySurfaceKHR(VulkanInstance, Surface, nullptr);
	VkHelper.DestroyDebugMessenger(DebugMessenger);
	vkDestroyInstance(VulkanInstance, nullptr);
}

void Renderer_Vulkan::Clear() const
{
}

void Renderer_Vulkan::WaitFrameFinish() const
{
	VKCall(vkDeviceWaitIdle(Device), "Wait for device Idle Failed.");
}

void Renderer_Vulkan::StartFrame()
{
	ExecuteTransientCommandBuffers();

	VKCall(vkWaitForFences(Device, 1, &InFlightFences[FrameIdx]->mFence, VK_TRUE, UINT64_MAX), "Waiting for Fence failed.");

	SwapChain->AcquireNextImage(ImageAvailableSemaphores[FrameIdx]);

	InFlightFences[FrameIdx]->Reset();
		
	GetCommandBuffer()->Reset();

	// Record Command Buffer
	GetCommandBuffer()->Begin();
}

void Renderer_Vulkan::SwapBuffers()
{
	//vkCmdEndRenderPass(GetRenderCommandBuffer());

	GetCommandBuffer()->End();

	// Submit Command Buffer
	GetQueue(CommandQueueType::Graphics)->SubmitCommandBuffer(GetCommandBuffer(), ImageAvailableSemaphores[FrameIdx], RenderingFinishedSemaphores[FrameIdx], InFlightFences[FrameIdx]);

	// Present to Screen
	GetQueue(CommandQueueType::Present)->PresentToScreen(RenderingFinishedSemaphores[FrameIdx], SwapChain);

	FrameIdx = (FrameIdx + 1) % BACKBUFFER_COUNT;
}

void Renderer_Vulkan::BindShader(const Shader* shader)
{
	const Shader_Vulkan* VkShader = static_cast<const Shader_Vulkan*>(shader);
	if (VkShader)
	{
		vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mPipeline);
	}
}

void Renderer_Vulkan::BindComputeShader(const ComputeShader* shader)
{
}

void Renderer_Vulkan::BindFrameBuffer(const FrameBuffer* framebuffer)
{
}

void Renderer_Vulkan::BindVertexBuffer(const VertexBuffer* vertexBuffer)
{
	if (vertexBuffer)
	{
		VkBuffer Buffers[] = { static_cast<const VertexBuffer_Vulkan*>(vertexBuffer)->MainBufferInfo.Buffer };
		VkDeviceSize Offsets[] = { 0 };
		vkCmdBindVertexBuffers(GetCommandBuffer()->mCommandBuffer, 0, ARRAY_SIZE(Buffers), Buffers, Offsets);
	}
}

void Renderer_Vulkan::BindIndexBuffer(const IndexBuffer* indexBuffer)
{
	if (indexBuffer)
	{
		const IndexBuffer_Vulkan* VkIndexBuffer = static_cast<const IndexBuffer_Vulkan*>(indexBuffer);
		vkCmdBindIndexBuffer(GetCommandBuffer()->mCommandBuffer, VkIndexBuffer->MainBufferInfo.Buffer, 0, VkIndexBuffer->GetVkIndexType());;
	}
}

void Renderer_Vulkan::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	const UniformBuffer_Vulkan* VkUbo = static_cast<const UniformBuffer_Vulkan*>(uniformBuffer);
	if (VkUbo)
	{
		//vkCmdBindDescriptorSets(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *VkUbo->PipelineLayout, 0, 1, &VkUbo->UboDescSet->DescSet, 0, nullptr);
	}
}

void Renderer_Vulkan::BindTexture(const Texture* texture, unsigned int slot)
{
}

void Renderer_Vulkan::UnbindShader(const Shader* shader)
{
}

void Renderer_Vulkan::UnbindComputeShader(const ComputeShader* shader)
{
}

void Renderer_Vulkan::UnbindFrameBuffer(const FrameBuffer* framebuffer)
{
}

void Renderer_Vulkan::UnbindVertexBuffer(const VertexBuffer* vertexBuffer)
{
}

void Renderer_Vulkan::UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer)
{
}

void Renderer_Vulkan::UnbindUniformBuffer(const UniformBuffer* uniformBuffer)
{
}

void Renderer_Vulkan::UnbindTexture(const Texture* texture, unsigned int slot)
{
}

void Renderer_Vulkan::Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount, int32_t InstanceStart)
{
	ScreenRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBuffer_Vulkan*>(GetBackBuffer()));

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const Shader_Vulkan*>(Shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);

	vkCmdDraw(GetCommandBuffer()->mCommandBuffer, VertexCount, InstanceCount, VertexStart, InstanceStart);

	ScreenRenderPass->EndPass(GetCommandBuffer());
}

void Renderer_Vulkan::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	ScreenRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBuffer_Vulkan*>(GetBackBuffer()));

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const Shader_Vulkan*>(shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);

	vkCmdDrawIndexed(GetCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	ScreenRenderPass->EndPass(GetCommandBuffer());
}

void Renderer_Vulkan::DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
}

void Renderer_Vulkan::DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
}

void Renderer_Vulkan::DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader)
{
}

void Renderer_Vulkan::DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader)
{
}

void Renderer_Vulkan::SetDepthTest(bool bEnabled)
{
}

void Renderer_Vulkan::SetBlending(bool bEnabled)
{
}

void Renderer_Vulkan::SetCullingFace(CullFace Face)
{
	CullingFace = Face;
}

CullFace Renderer_Vulkan::GetCullingFace()
{
	return CullingFace;
}

const FrameBuffer* Renderer_Vulkan::GetBackBuffer()
{
	return SwapchainFrameBuffers[FrameIdx];
}

void Renderer_Vulkan::SetViewportSize(Rect3D viewport)
{
	Viewport.x = viewport.Offset.x;
	Viewport.y = viewport.Offset.y;
	Viewport.width = viewport.Size.x;
	Viewport.height = viewport.Size.y;
	Viewport.minDepth = viewport.Depth.x;
	Viewport.maxDepth = viewport.Depth.y;
}

void Renderer_Vulkan::SetScissorSize(Rect2D scissor)
{
	ScissorRect.offset.x = scissor.Offset.x;
	ScissorRect.offset.y = scissor.Offset.y;
	ScissorRect.extent.width = scissor.Size.x;
	ScissorRect.extent.height = scissor.Size.y;
}

Rect3D Renderer_Vulkan::GetViewportSize()
{
	Rect3D OutViewport;
	OutViewport.Offset = glm::vec2(Viewport.x, Viewport.y);
	OutViewport.Size = glm::vec2(Viewport.width, Viewport.height);
	OutViewport.Depth = glm::vec2(Viewport.minDepth, Viewport.maxDepth);
	return OutViewport;
}

Rect2D Renderer_Vulkan::GetScissorSize()
{
	Rect2D OutScissor;
	OutScissor.Offset = glm::vec2(ScissorRect.offset.x, ScissorRect.offset.y);
	OutScissor.Size = glm::vec2(ScissorRect.extent.width, ScissorRect.extent.height);
	return OutScissor;
}

void Renderer_Vulkan::ExecuteTransientCommandBuffers()
{
	if (ActiveCopyCommandBuffer)
	{
		ActiveCopyCommandBuffer->End();

		Fence_Vulkan* CopyFence = new Fence_Vulkan();

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

CommandBuffer_Vulkan* Renderer_Vulkan::GetCopyCommandBuffer()
{
	if (!ActiveCopyCommandBuffer)
	{
		ActiveCopyCommandBuffer = TransientTransferCmdBufferPool->AllocateCommandBuffer();
		ActiveCopyCommandBuffer->Begin();
	}
	return ActiveCopyCommandBuffer;
}

void Renderer_Vulkan::QueueCopyCommands(const BufferInfo& srcBuffer, const BufferInfo& dstBuffer, const OnCommandExecutionEnd& FinishDelegate)
{
	CommandBuffer_Vulkan* CopyCommandBuffer = GetCopyCommandBuffer();

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

CommandBuffer_Vulkan* Renderer_Vulkan::AllocateTransientCommandBuffer(CommandQueueType type)
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

void Renderer_Vulkan::CreateDescriptorPools()
{
	RendererDescPool = new DescriptorPool_Vulkan();

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