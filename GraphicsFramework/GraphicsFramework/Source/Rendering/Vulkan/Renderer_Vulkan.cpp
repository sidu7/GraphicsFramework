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
// Vulkan Helpers
#include "Internal/VulkanHelper.h"
#include "Internal/CommandBufferPool_Vulkan.h"
#include "Internal/CommandBuffer_Vulkan.h"
#include "Internal/CommandQueue_Vulkan.h"
#include "Internal/Fence_Vulkan.h"
#include "Internal/Semaphore_Vulkan.h"

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
	ScreenRenderPass->Init(SwapChainImageFormat, true, false);

	ImGuiRenderPass = new RenderPass_Vulkan();
	ImGuiRenderPass->Init(SwapChainImageFormat, false, true);

	for (const VkImageView& ImageView : SwapChainImageViews)
	{
		FrameBuffer_Vulkan* frameBuffer = static_cast<FrameBuffer_Vulkan*>(RenderingFactory::Instance()->CreateFrameBuffer());
		frameBuffer->InitAsBackBuffer(SwapChainImageExtent.width, SwapChainImageExtent.height, ImageView);
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

	ClearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
	ClearValue.depthStencil = { 0.0f, 0 };


	Viewport.x = 0.0f;
	Viewport.y = 0.0f;
	Viewport.width = SwapChainImageExtent.width;
	Viewport.height = SwapChainImageExtent.height;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;

	ScissorRect.extent = SwapChainImageExtent;
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
		
	vkDestroyDescriptorPool(Device, ImGuiDescPool, nullptr);
	delete ImGuiRenderPass;
	delete ScreenRenderPass;

	for (const VkImageView& ImageView : SwapChainImageViews)
	{
		vkDestroyImageView(Device, ImageView, nullptr);
	}

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

	vkDestroySwapchainKHR(Device, SwapChain, nullptr);
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

	VkResult AcquireResult = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[FrameIdx]->mSemaphore, VK_NULL_HANDLE, &SwapChainImageIdx);

	VKCall(vkResetFences(Device, 1, &InFlightFences[FrameIdx]->mFence), "Fence Reset Failed.");

	GetRenderCommandBuffer()->Reset();

	// Record Command Buffer
	GetRenderCommandBuffer()->Begin();
}

void Renderer_Vulkan::SwapBuffers()
{
	//vkCmdEndRenderPass(GetRenderCommandBuffer());

	GetRenderCommandBuffer()->End();

	// Submit Command Buffer
	GetQueue(CommandQueueType::Graphics)->SubmitCommandBuffer(GetRenderCommandBuffer(), ImageAvailableSemaphores[FrameIdx], RenderingFinishedSemaphores[FrameIdx], InFlightFences[FrameIdx]);

	// Present to Screen
	GetQueue(CommandQueueType::Present)->PresentToScreen(RenderingFinishedSemaphores[FrameIdx], SwapChain, SwapChainImageIdx);

	FrameIdx = (FrameIdx + 1) % BACKBUFFER_COUNT;
}

void Renderer_Vulkan::BindShader(const Shader* shader)
{
	const Shader_Vulkan* VkShader = static_cast<const Shader_Vulkan*>(shader);
	if (VkShader)
	{
		vkCmdBindPipeline(GetRenderCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mPipeline);
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
		vkCmdBindVertexBuffers(GetRenderCommandBuffer()->mCommandBuffer, 0, ARRAY_SIZE(Buffers), Buffers, Offsets);
	}
}

void Renderer_Vulkan::BindIndexBuffer(const IndexBuffer* indexBuffer)
{
	if (indexBuffer)
	{
		const IndexBuffer_Vulkan* VkIndexBuffer = static_cast<const IndexBuffer_Vulkan*>(indexBuffer);
		vkCmdBindIndexBuffer(GetRenderCommandBuffer()->mCommandBuffer, VkIndexBuffer->MainBufferInfo.Buffer, 0, VkIndexBuffer->GetVkIndexType());;
	}
}

void Renderer_Vulkan::BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint)
{
	const UniformBuffer_Vulkan* VkUbo = static_cast<const UniformBuffer_Vulkan*>(uniformBuffer);
	if (VkUbo)
	{
		vkCmdBindDescriptorSets(GetRenderCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *VkUbo->PipelineLayout, 0, 1, &VkUbo->UboDescSet->DescSet, 0, nullptr);
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
	ScreenRenderPass->BeginPass(GetRenderCommandBuffer(), static_cast<const FrameBuffer_Vulkan*>(GetBackBuffer()));

	vkCmdSetViewport(GetRenderCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetRenderCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetRenderCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetRenderCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetRenderCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetRenderCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetRenderCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const Shader_Vulkan*>(Shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);

	vkCmdDraw(GetRenderCommandBuffer()->mCommandBuffer, VertexCount, InstanceCount, VertexStart, InstanceStart);

	ScreenRenderPass->EndPass(GetRenderCommandBuffer());
}

void Renderer_Vulkan::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	ScreenRenderPass->BeginPass(GetRenderCommandBuffer(), static_cast<const FrameBuffer_Vulkan*>(GetBackBuffer()));

	vkCmdSetViewport(GetRenderCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetRenderCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetRenderCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetRenderCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetRenderCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetRenderCommandBuffer()->mCommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(GetRenderCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, static_cast<const Shader_Vulkan*>(shader)->mPipeline);

	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);

	vkCmdDrawIndexed(GetRenderCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	ScreenRenderPass->EndPass(GetRenderCommandBuffer());
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

void Renderer_Vulkan::SetViewportSize(glm::vec2 Offset, glm::vec2 Size)
{
	Viewport.x = Offset.x;
	Viewport.y = Offset.y;
	Viewport.width = Size.x;
	Viewport.height = Size.y;
	Viewport.minDepth = 0.0f;
	Viewport.maxDepth = 1.0f;
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