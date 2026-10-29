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
// Profiling
#include "Profiling/Profiler.h"
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
#include "RHI/Vulkan/Internal/DescriptorSetVK.h"
// Vulkan Helpers
#include "Internal/HelperVK.h"
// ImGui backend hooks
#include "Imgui/backends/imgui_impl_sdl2.h"
#include "Imgui/backends/imgui_impl_vulkan.h"
#include <vulkan\vulkan.h>
#include "Internal/CommandBufferPoolVK.h"
#include "Internal/CommandBufferVK.h"
#include "Internal/CommandQueueVK.h"
#include "Internal/FenceVK.h"
#include "Internal/SemaphoreVK.h"
#include "Internal/SwapChainVK.h"

#include <vk_mem_alloc.h>

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

BufferInfo Device::CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, bool bHostVisible)
{
	BufferInfo BufferData;
	BufferData.Size = static_cast<uint32_t>(bufferSize);

	VkBufferCreateInfo BufferCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	BufferCreateInfo.size = bufferSize;
	BufferCreateInfo.usage = usageFlags;
	Helper::FillSharingMode(BufferCreateInfo.sharingMode, BufferCreateInfo.queueFamilyIndexCount, &BufferCreateInfo.pQueueFamilyIndices);

	VmaAllocationCreateInfo AllocCreateInfo = {};
	AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	if (bHostVisible)
	{
		AllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	}

	VKCall(vmaCreateBuffer(mAllocator, &BufferCreateInfo, &AllocCreateInfo, &BufferData.Buffer, &BufferData.Allocation, nullptr), "Buffer creation Failed.");

	return BufferData;
}

ImageInfo Device::CreateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags)
{
	ImageInfo ImageData;

	VkImageCreateInfo ImageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	ImageCreateInfo.format = imageFormat;
	ImageCreateInfo.extent.width = width;
	ImageCreateInfo.extent.height = height;
	ImageCreateInfo.extent.depth = 1;
	ImageCreateInfo.mipLevels = 1;
	ImageCreateInfo.arrayLayers = 1;
	ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	ImageCreateInfo.usage = usageFlags;
	Helper::FillSharingMode(ImageCreateInfo.sharingMode, ImageCreateInfo.queueFamilyIndexCount, &ImageCreateInfo.pQueueFamilyIndices);
	ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VmaAllocationCreateInfo AllocCreateInfo = {};
	AllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

	VKCall(vmaCreateImage(mAllocator, &ImageCreateInfo, &AllocCreateInfo, &ImageData.Image, &ImageData.Allocation, nullptr), "Failed to create Image.");

	return ImageData;
}

void Device::DestroyBuffer(BufferInfo& buffer)
{
	if (buffer.Buffer != VK_NULL_HANDLE || buffer.Allocation != VK_NULL_HANDLE)
	{
		vmaDestroyBuffer(mAllocator, buffer.Buffer, buffer.Allocation);
	}
	buffer = BufferInfo();
}

void Device::DestroyImage(ImageInfo& image)
{
	if (image.Image != VK_NULL_HANDLE || image.Allocation != VK_NULL_HANDLE)
	{
		vmaDestroyImage(mAllocator, image.Image, image.Allocation);
	}
	image = ImageInfo();
}

void* Device::MapBuffer(BufferInfo& buffer, size_t offset)
{
	void* MappedPointer = nullptr;
	VKCall(vmaMapMemory(mAllocator, buffer.Allocation, &MappedPointer), "Buffer memory mapping Failed.");

	buffer.MappedMemory = static_cast<uint8_t*>(MappedPointer) + offset;
	return buffer.MappedMemory;
}

void Device::UnmapBuffer(BufferInfo& buffer)
{
	if (buffer.Allocation != VK_NULL_HANDLE)
	{
		vmaUnmapMemory(mAllocator, buffer.Allocation);
	}
	buffer.MappedMemory = nullptr;
}

void Device::FlushBuffer(const BufferInfo& buffer, VkDeviceSize offset, VkDeviceSize size)
{
	if (buffer.Allocation != VK_NULL_HANDLE)
	{
		VKCall(vmaFlushAllocation(mAllocator, buffer.Allocation, offset, size), "Buffer flush Failed.");
	}
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

	VmaAllocatorCreateInfo AllocatorInfo = {};
	AllocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
	AllocatorInfo.instance = VulkanInstance;
	AllocatorInfo.physicalDevice = PhysicalDevice;
	AllocatorInfo.device = mDevice;
	VKCall(vmaCreateAllocator(&AllocatorInfo, &mAllocator), "Vulkan Memory Allocator creation Failed.");

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
	ScreenRenderPass->Init(SwapChain->ImageFormat, true, false, true);

	ImGuiRenderPass = new RenderPassVK();
	ImGuiRenderPass->Init(SwapChain->ImageFormat, false, true, true);

	for (const VkImageView& ImageView : SwapChain->ImageViews)
	{
		// Depth is attached to the back buffer so scenes do not depend on draw order.
		ImageInfo DepthImage = CreateImage(SwapChain->ImageExtent.width, SwapChain->ImageExtent.height,
			VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		VkImageViewCreateInfo DepthViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		DepthViewInfo.image = DepthImage.Image;
		DepthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		DepthViewInfo.format = VK_FORMAT_D32_SFLOAT;
		DepthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		DepthViewInfo.subresourceRange.baseMipLevel = 0;
		DepthViewInfo.subresourceRange.levelCount = 1;
		DepthViewInfo.subresourceRange.baseArrayLayer = 0;
		DepthViewInfo.subresourceRange.layerCount = 1;

		VkImageView DepthView = VK_NULL_HANDLE;
		VKCall(vkCreateImageView(mDevice, &DepthViewInfo, nullptr, &DepthView), "Depth Image View creation Failed.");

		DepthImages.push_back(DepthImage);
		DepthImageViews.push_back(DepthView);

		FrameBufferVK* frameBuffer = static_cast<FrameBufferVK*>(CreateFrameBuffer());
		frameBuffer->InitAsBackBuffer(SwapChain->ImageExtent.width, SwapChain->ImageExtent.height, ImageView, DepthView);
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
	CreateFrameDescriptorPools();

	ClearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
	ClearValues[1].depthStencil = { 1.0f, 0 };


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
	DepthTestEnable = true;

	ActiveCopyCommandBuffer = nullptr;
	mBoundShader = nullptr;
}

void Device::Close()
{
	Helper VkHelper;
		
	// Write out descriptor pool data to setting file

	vkDestroyDescriptorPool(mDevice, RendererDescPool->DescPool, nullptr);

	for (const VkDescriptorPool& FrameDescPool : FrameDescriptorPools)
	{
		vkDestroyDescriptorPool(mDevice, FrameDescPool, nullptr);
	}
	FrameDescriptorPools.clear();

	vkDestroyDescriptorPool(mDevice, ImGuiDescPool, nullptr);
	delete ImGuiRenderPass;
	delete ScreenRenderPass;

	for (const FrameBufferVK* FrameBuffer : SwapchainFrameBuffers)
	{
		delete FrameBuffer;
	}
	SwapchainFrameBuffers.clear();

	for (const VkImageView DepthView : DepthImageViews)
	{
		vkDestroyImageView(mDevice, DepthView, nullptr);
	}
	DepthImageViews.clear();

	for (ImageInfo& DepthImage : DepthImages)
	{
		DestroyImage(DepthImage);
	}
	DepthImages.clear();

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

	// All buffers/images are released by their owners before the device shuts down,
	// so the allocator can go away here.
	if (mAllocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(mAllocator);
		mAllocator = VK_NULL_HANDLE;
	}

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

void Device::WaitForPreviousFrame()
{
	if (InFlightFences.size() > FrameIdx)
	{
		InFlightFences[FrameIdx]->Wait();
	}
}

void Device::WaitForFenceAt(uint32_t frameIdx)
{
	if (frameIdx < InFlightFences.size())
	{
		InFlightFences[frameIdx]->Wait();
	}
}

void Device::StartFrame()
{
	ExecuteTransientCommandBuffers();

	VKCall(vkWaitForFences(mDevice, 1, &InFlightFences[FrameIdx]->mFence, VK_TRUE, UINT64_MAX), "Waiting for Fence failed.");

	SwapChain->AcquireNextImage(ImageAvailableSemaphores[FrameIdx]);

	InFlightFences[FrameIdx]->Reset();

	// The fence for this frame index was just waited on, so the descriptor sets
	// recorded into it are no longer in use and the pool can be recycled.
	if (FrameIdx < FrameDescriptorPools.size())
	{
		VKCall(vkResetDescriptorPool(mDevice, FrameDescriptorPools[FrameIdx], 0), "Descriptor Pool reset Failed.");
	}

	GetCommandBuffer()->Reset();

	// Record Command Buffer
	GetCommandBuffer()->Begin();

	// The screen pass is cleared once per frame and stays open while the scene
	// draws its objects, so all of them accumulate into the same image.
	mPresentedToScreen = false;
	BeginScreenPass();
}

void Device::BeginScreenPass()
{
	if (mScreenPassOpen)
	{
		return;
	}

	ScreenRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBufferVK*>(GetBackBuffer()));
	mScreenPassOpen = true;
}

void Device::EndScreenPass()
{
	if (!mScreenPassOpen)
	{
		return;
	}

	ScreenRenderPass->EndPass(GetCommandBuffer());
	mScreenPassOpen = false;
}

void Device::SwapBuffers()
{
	EndScreenPass();

	// Without ImGui the screen pass is the last pass of the frame; a pass that
	// stores to the image and ends in PRESENT_SRC_KHR is needed before presenting.
	if (!mPresentedToScreen)
	{
		ImGuiRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBufferVK*>(GetBackBuffer()));
		ImGuiRenderPass->EndPass(GetCommandBuffer());
	}

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
		mBoundShader = shader;
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
	// Descriptors cannot be written here: the shader that will consume them is
	// only known when Draw() records the command (BindUniformBuffer is usually
	// called before the draw that binds the shader). Store the binding and let
	// AllocateDrawDescriptorSet() fill the set at draw-recording time.
	if (uniformBuffer)
	{
		mBoundUniformBuffers[bindpoint] = uniformBuffer;
	}
	else
	{
		mBoundUniformBuffers.erase(bindpoint);
	}
}

void Device::BindTexture(const Texture* texture, unsigned int slot)
{
	if (texture)
	{
		mBoundTextures[slot] = texture;
	}
	else
	{
		mBoundTextures.erase(slot);
	}
}

VkDescriptorSet Device::AllocateDrawDescriptorSet(const ShaderVK* shader)
{
	if (!shader || !shader->mDescriptorSet)
	{
		return VK_NULL_HANDLE;
	}

	DescriptorSetVK* DescriptorSet = shader->mDescriptorSet;
	VkDescriptorSet DrawSet = DescriptorSet->Allocate(FrameDescriptorPools[FrameIdx]);
	if (DrawSet == VK_NULL_HANDLE)
	{
		return VK_NULL_HANDLE;
	}

	for (const std::pair<const uint32_t, const UniformBuffer*>& BoundUbo : mBoundUniformBuffers)
	{
		if (DescriptorSet->GetBindingType(BoundUbo.first) != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			continue;
		}

		const UniformBufferVK* VkUbo = static_cast<const UniformBufferVK*>(BoundUbo.second);
		if (VkUbo && VkUbo->UboInfo.Buffer != VK_NULL_HANDLE)
		{
			DescriptorSet->WriteUniformBuffer(DrawSet, BoundUbo.first, VkUbo->UboInfo.Buffer,
				static_cast<VkDeviceSize>(VkUbo->UboInfo.Size));
		}
	}

	for (const std::pair<const uint32_t, const Texture*>& BoundTex : mBoundTextures)
	{
		if (DescriptorSet->GetBindingType(BoundTex.first) != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			continue;
		}

		const TextureVK* VkTex = static_cast<const TextureVK*>(BoundTex.second);
		if (VkTex && VkTex->TexImageView != VK_NULL_HANDLE && VkTex->TexSampler != VK_NULL_HANDLE)
		{
			DescriptorSet->WriteTexture(DrawSet, BoundTex.first, VkTex->TexImageView, VkTex->TexSampler);
		}
	}

	return DrawSet;
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
	for (auto It = mBoundUniformBuffers.begin(); It != mBoundUniformBuffers.end();)
	{
		if (It->second == uniformBuffer)
		{
			It = mBoundUniformBuffers.erase(It);
		}
		else
		{
			++It;
		}
	}
}

void Device::UnbindTexture(const Texture* texture, unsigned int slot)
{
	mBoundTextures.erase(slot);
}

void Device::Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount, int32_t InstanceStart)
{
	Profiler::Instance()->BeginDrawCall("", Shader ? Shader->GetName() : "", "");

	// The pass is opened in StartFrame and shared by every draw of the frame.
	BeginScreenPass();

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	const ShaderVK* VkShader = static_cast<const ShaderVK*>(Shader);
	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mPipeline);

	mBoundShader = Shader;

	VkDescriptorSet DrawDescriptorSet = AllocateDrawDescriptorSet(VkShader);
	if (DrawDescriptorSet != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			VkShader->mPipelineLayout, 0, 1, &DrawDescriptorSet, 0, nullptr);
	}

	BindVertexBuffer(vertexBuffer);

	vkCmdDraw(GetCommandBuffer()->mCommandBuffer, VertexCount, InstanceCount, VertexStart, InstanceStart);

	Profiler::Instance()->EndDrawCall();
}

void Device::Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	Profiler::Instance()->BeginDrawCall("", shader ? shader->GetName() : "", "");

	// The pass is opened in StartFrame and shared by every draw of the frame.
	BeginScreenPass();

	vkCmdSetViewport(GetCommandBuffer()->mCommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(GetCommandBuffer()->mCommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(GetCommandBuffer()->mCommandBuffer, Topology);
	vkCmdSetCullMode(GetCommandBuffer()->mCommandBuffer, CullMode);
	vkCmdSetFrontFace(GetCommandBuffer()->mCommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(GetCommandBuffer()->mCommandBuffer, DepthTestEnable);

	const ShaderVK* VkShader = static_cast<const ShaderVK*>(shader);
	vkCmdBindPipeline(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mPipeline);

	// Track the bound shader and bind this draw's descriptor set
	mBoundShader = shader;

	VkDescriptorSet DrawDescriptorSet = AllocateDrawDescriptorSet(VkShader);
	if (DrawDescriptorSet != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(GetCommandBuffer()->mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			VkShader->mPipelineLayout, 0, 1, &DrawDescriptorSet, 0, nullptr);
	}

	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);

	vkCmdDrawIndexed(GetCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	Profiler::Instance()->EndDrawCall();
}

void Device::BeginDebugDraw(const Shader* shader, VkPrimitiveTopology topology)
{
	const ShaderVK* VkShader = static_cast<const ShaderVK*>(shader);
	if (!VkShader || VkShader->mLinePipeline == VK_NULL_HANDLE)
	{
		return;
	}

	BeginScreenPass();

	VkCommandBuffer CommandBuffer = GetCommandBuffer()->mCommandBuffer;
	vkCmdSetViewport(CommandBuffer, 0, 1, &Viewport);
	vkCmdSetScissor(CommandBuffer, 0, 1, &ScissorRect);
	vkCmdSetPrimitiveTopology(CommandBuffer, topology);
	// Line primitives are not affected by culling; disabling it keeps the debug
	// draws independent of the winding of the shape they visualise.
	vkCmdSetCullMode(CommandBuffer, VK_CULL_MODE_NONE);
	vkCmdSetFrontFace(CommandBuffer, FrontFace);
	vkCmdSetDepthTestEnable(CommandBuffer, DepthTestEnable);

	vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VkShader->mLinePipeline);
	mBoundShader = shader;

	VkDescriptorSet DrawDescriptorSet = AllocateDrawDescriptorSet(VkShader);
	if (DrawDescriptorSet != VK_NULL_HANDLE)
	{
		vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			VkShader->mPipelineLayout, 0, 1, &DrawDescriptorSet, 0, nullptr);
	}
}

void Device::DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	// OpenGL draws this as GL_LINE_LOOP; Vulkan has no line-loop topology, so
	// use a line strip (same vertices, without the closing segment).
	if (!shader || !vertexBuffer || !indexBuffer)
	{
		return;
	}

	Profiler::Instance()->BeginDrawCall("", shader->GetName(), "");

	BeginDebugDraw(shader, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	vkCmdDrawIndexed(GetCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	Profiler::Instance()->EndDrawCall();
}

void Device::DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader)
{
	if (!shader || !vertexBuffer || !indexBuffer)
	{
		return;
	}

	// The wire primitives store their indices in line-pair order, matching the
	// GL_LINES draw the OpenGL backend records.
	Profiler::Instance()->BeginDrawCall("", shader->GetName(), "");

	BeginDebugDraw(shader, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	BindVertexBuffer(vertexBuffer);
	BindIndexBuffer(indexBuffer);
	vkCmdDrawIndexed(GetCommandBuffer()->mCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);

	Profiler::Instance()->EndDrawCall();
}

void Device::DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	if (!shader || !vertexBuffer)
	{
		return;
	}

	Profiler::Instance()->BeginDrawCall("", shader->GetName(), "");

	BeginDebugDraw(shader, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	BindVertexBuffer(vertexBuffer);
	vkCmdDraw(GetCommandBuffer()->mCommandBuffer, 16, 1, 0, 0);

	Profiler::Instance()->EndDrawCall();
}

void Device::DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader)
{
	if (!shader || !vertexBuffer)
	{
		return;
	}

	Profiler::Instance()->BeginDrawCall("", shader->GetName(), "");

	BeginDebugDraw(shader, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
	BindVertexBuffer(vertexBuffer);
	vkCmdDraw(GetCommandBuffer()->mCommandBuffer, 2, 1, 0, 0);

	Profiler::Instance()->EndDrawCall();
}

void Device::SetDepthTest(bool bEnabled)
{
	DepthTestEnable = bEnabled;
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

void Device::SetClearColor(glm::vec3 color)
{
	Renderer::SetClearColor(color);
	ClearValues[0].color = { color.r, color.g, color.b, 1.0f };
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

void Device::ScheduleCopyCommandCleanup(const OnCommandExecutionEnd& FinishDelegate)
{
	if (ActiveCopyCommandBuffer && FinishDelegate)
	{
		ActiveCopyCommandBuffer->ExecutionFinishDelegates.push_back(FinishDelegate);
	}
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

void Device::CreateFrameDescriptorPools()
{
	// Per-frame transient descriptor sets. The pool is reset once the frame's
	// fence has been waited on, so a set can be allocated per recorded draw
	// without the whole shader seeing the last descriptor write of the frame.
	// Each recorded draw allocates its own descriptor set, so the pools must be
	// large enough for a whole frame of draws (FlockingParticles alone records
	// 250+). They are reset per frame, so this is a high-water mark, not a leak.
	const std::vector<VkDescriptorPoolSize> PoolSizes = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 256 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 256 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 256 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 256 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 128 },
	};

	FrameDescriptorPools.resize(BACKBUFFER_COUNT, VK_NULL_HANDLE);
	for (VkDescriptorPool& FrameDescPool : FrameDescriptorPools)
	{
		VkDescriptorPoolCreateInfo FrameDescPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		FrameDescPoolInfo.maxSets = 1024;
		FrameDescPoolInfo.poolSizeCount = static_cast<uint32_t>(PoolSizes.size());
		FrameDescPoolInfo.pPoolSizes = PoolSizes.data();

		VKCall(vkCreateDescriptorPool(mDevice, &FrameDescPoolInfo, nullptr, &FrameDescPool), "Frame Descriptor Pool creation Failed.");
	}
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

void Device::InitImGui()
{
	// Initialize SDL2 platform backend first
	ImGui_ImplSDL2_InitForVulkan(Window::Instance()->GetWindow());

	// Then initialize Vulkan rendering backend
	rhi::vk::Helper VkHelper;
	VkHelper.InitImGui();
}

void Device::NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
}

void Device::Render()
{
	// ImGui draws in its own pass, which loads the scene image and presents it.
	EndScreenPass();

	ImGuiRenderPass->BeginPass(GetCommandBuffer(), static_cast<const FrameBufferVK*>(GetBackBuffer()));
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), GetCommandBuffer()->mCommandBuffer);
	ImGuiRenderPass->EndPass(GetCommandBuffer());

	mPresentedToScreen = true;
}

void Device::ShutdownImGui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
}

float Device::GetProjectionYFlip() const
{
	return -1.0f; // Vulkan clip space: Y goes down
}
}
