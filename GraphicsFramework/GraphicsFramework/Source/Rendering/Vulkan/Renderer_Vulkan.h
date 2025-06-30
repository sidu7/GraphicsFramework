/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Renderer.h"

#include "Core/Core.h"
#include <vulkan\vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VKCall(FuncCall, Error) \
{	\
	VkResult Result = FuncCall; \
	if(Result != VK_SUCCESS) \
	{	\
		std::cout << "Vulkan Error:[" << string_VkResult(Result) << "] " << Error << std::endl; \
		__debugbreak(); \
	}	\
}

#ifndef BACKBUFFER_COUNT
#define BACKBUFFER_COUNT (uint32_t)2
#endif

class RenderPass_Vulkan;
class CommandBufferPool_Vulkan;
class CommandBuffer_Vulkan;
class CommandQueue_Vulkan;
class FrameBuffer_Vulkan;
class Semaphore_Vulkan;
class Fence_Vulkan;
struct BufferInfo;
class SwapChain_Vulkan;

enum CommandQueueType
{
	Graphics,
	Compute,
	Transfer,
	Present,

	Count,
};

struct QueueFamilyIndices
{
	uint32_t QueueIndices[CommandQueueType::Count];

	QueueFamilyIndices()
	{
		QueueIndices[CommandQueueType::Graphics] = std::numeric_limits<uint32_t>::max();
		QueueIndices[CommandQueueType::Compute] = std::numeric_limits<uint32_t>::max();
		QueueIndices[CommandQueueType::Transfer] = std::numeric_limits<uint32_t>::max();
		QueueIndices[CommandQueueType::Present] = std::numeric_limits<uint32_t>::max();
	}

	bool IsValid()
	{
		return QueueIndices[CommandQueueType::Graphics] != std::numeric_limits<uint32_t>::max() &&
			QueueIndices[CommandQueueType::Compute] != std::numeric_limits<uint32_t>::max() &&
			QueueIndices[CommandQueueType::Transfer] != std::numeric_limits<uint32_t>::max() &&
			QueueIndices[CommandQueueType::Present] != std::numeric_limits<uint32_t>::max();
	}
};

typedef std::function<void()> OnCommandExecutionEnd;

class Renderer_Vulkan : public Renderer
{
	friend class VulkanHelper;
	friend class RenderingFactory_Vulkan;
	friend class SwapChain_Vulkan;

public:
	static Renderer_Vulkan* Get() { return static_cast<Renderer_Vulkan*>(Renderer::Instance()); }

	virtual void Init() override;
	virtual void Close() override;
	
	virtual void StartFrame() override;
	virtual void SwapBuffers() override;

	virtual void Clear() const override; 
	virtual void WaitFrameFinish() const override;

	// Bind Functions
	virtual void BindShader(const Shader* shader) override;
	virtual void BindComputeShader(const ComputeShader* shader) override;
	virtual void BindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void BindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void BindIndexBuffer(const IndexBuffer* elementArrayBuffer) override;
	virtual void BindUniformBuffer(const UniformBuffer* uniformBuffer, unsigned int bindpoint = 0) override;
	virtual void BindTexture(const Texture* texture, unsigned int slot = 0) override;
	
	// Unbind Functions
	virtual void UnbindShader(const Shader* shader) override;
	virtual void UnbindComputeShader(const ComputeShader* shader) override;
	virtual void UnbindFrameBuffer(const FrameBuffer* framebuffer) override;
	virtual void UnbindVertexBuffer(const VertexBuffer* vertexBuffer) override;
	virtual void UnbindIndexBuffer(const IndexBuffer* elementArrayBuffer) override;
	virtual void UnbindUniformBuffer(const UniformBuffer* uniformBuffer) override;
	virtual void UnbindTexture(const Texture* texture, unsigned int slot = 0) override;

	// Draw Functions
	virtual void Draw(const Shader* Shader, const VertexBuffer* vertexBuffer, int32_t VertexCount, int32_t VertexStart, int32_t InstanceCount = 1, int32_t InstanceStart = 0) override;
	virtual void Draw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader) override;
	virtual void DebugDraw(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader* shader) override;
	virtual void DebugDrawLines(const VertexBuffer* vertexBuffer, const IndexBuffer* indexBuffer, const Shader * shader) override;
	virtual void DrawDebugCircle(const VertexBuffer* vertexBuffer, const Shader* shader) override;
	virtual void DrawDebugLine(const VertexBuffer* vertexBuffer, const Shader* shader) override;

	// Rendering Features
	virtual void SetDepthTest(bool bEnabled) override;
	virtual void SetBlending(bool bEnabled) override;
	virtual void SetCullingFace(CullFace Face) override;
	virtual CullFace GetCullingFace() override;

	virtual const FrameBuffer* GetBackBuffer() override;
	virtual void SetViewportSize(glm::vec2 Offset, glm::vec2 Size) override;

	void ExecuteTransientCommandBuffers();

	// Copy
	CommandBuffer_Vulkan* GetCopyCommandBuffer();
	void QueueCopyCommands(const BufferInfo& srcBuffer, const BufferInfo& dstBuffer, const OnCommandExecutionEnd& FinishDelegate = OnCommandExecutionEnd());

	CommandBuffer_Vulkan* AllocateTransientCommandBuffer(CommandQueueType type);

	inline const VkInstance& GetVulkanInstance() { return VulkanInstance; }
	inline const VkPhysicalDevice& GetPhysicalDevice() { return PhysicalDevice; }
	inline const VkDevice& GetDevice() { return Device; }

	inline const CommandBuffer_Vulkan* GetCommandBuffer() { return RenderCommandBuffers[FrameIdx]; }
	inline const VkClearValue& GetClearValue() { return ClearValue; }
	inline const uint32_t GetQueueFamilyIndex(CommandQueueType type) { return QueueFamilyData.QueueIndices[type]; }
	inline const CommandQueue_Vulkan* GetQueue(CommandQueueType type) { return CommandQueues[type]; }

	inline const RenderPass_Vulkan* GetScreenRenderPass() { return ScreenRenderPass; }

	// ImGui
	inline const RenderPass_Vulkan* GetImGuiRenderPass() { return ImGuiRenderPass; }
protected:
#if _DEBUG
	// Debug Messenger Callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif


protected:
	CullFace CullingFace;

	// Core
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
	VkInstance VulkanInstance;
	VkDebugUtilsMessengerEXT DebugMessenger;
	VkPhysicalDevice PhysicalDevice;
	VkDevice Device;
	VkSurfaceKHR Surface;
	VkPhysicalDeviceMemoryProperties MemoryProperties;

	// Command Buffers, Queues and Pool
	CommandBufferPool_Vulkan* CommandBufferPool;
	CommandBufferPool_Vulkan* TransientTransferCmdBufferPool;
	CommandBufferPool_Vulkan* TransientGraphicsCmdBufferPool;

	std::vector<CommandBuffer_Vulkan*> RenderCommandBuffers;
	CommandBuffer_Vulkan* TransferCommandBuffer;

	QueueFamilyIndices QueueFamilyData;
	CommandQueue_Vulkan* CommandQueues[CommandQueueType::Count];

	// SwapChain
	SwapChain_Vulkan* SwapChain;
	std::vector<FrameBuffer_Vulkan*> SwapchainFrameBuffers;

	std::vector<Semaphore_Vulkan*> ImageAvailableSemaphores;
	std::vector<Semaphore_Vulkan*> RenderingFinishedSemaphores;
	std::vector<Fence_Vulkan*> InFlightFences;

	// RenderPass
	RenderPass_Vulkan* ScreenRenderPass;
	VkClearValue ClearValue;

	// ImGui
	RenderPass_Vulkan* ImGuiRenderPass;
	VkDescriptorPool ImGuiDescPool;

	uint8_t FrameIdx;
	uint32_t SwapChainImageIdx;

	// Dynamic Pipeline
	VkViewport Viewport;
	VkRect2D ScissorRect;
	VkPrimitiveTopology Topology;
	VkCullModeFlags CullMode;
	VkFrontFace FrontFace;
	VkBool32 DepthTestEnable;

	// Transient Operations
	CommandBuffer_Vulkan* ActiveCopyCommandBuffer;
};