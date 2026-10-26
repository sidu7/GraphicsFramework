/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once

#include "Rendering/Renderer.h"

#include "Core/Core.h"
#include "RHI/Vulkan/VulkanTypes.h"
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

namespace rhi::vk
{
class RenderPassVK;
class CommandBufferPoolVK;
class CommandBufferVK;
class CommandQueueVK;
class FrameBufferVK;
class SemaphoreVK;
class FenceVK;
struct BufferInfo;
class SwapChainVK;
class DescriptorPoolVK;
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

class Device : public Renderer
{
	friend class Helper;
	friend class SwapChainVK;

public:
	static Device* Get() { return static_cast<Device*>(Renderer::Instance()); }

	virtual const char* GetApiName() const override { return "VULKAN"; }

	// Resource creation
	virtual ComputeShader* CreateComputeShader() override;
	virtual IndexBuffer* CreateIndexBuffer() override;
	virtual FrameBuffer* CreateFrameBuffer() override;
	virtual Shader* CreateShader() override;
	virtual ShaderStorageBuffer* CreateShaderStorageBuffer() override;
	virtual Texture* CreateTexture() override;
	virtual UniformBuffer* CreateUniformBuffer() override;
	virtual VertexBuffer* CreateVertexBuffer() override;

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
	virtual void SetViewportSize(Rect3D viewport) override;
	virtual void SetScissorSize(Rect2D scissor) override;

	virtual Rect3D GetViewportSize() override;
	virtual Rect2D GetScissorSize() override;

	// Backend capability hooks for ImGui (called by ImguiManager)
	virtual void InitImGui() override;
	virtual void ShutdownImGui() override;
	virtual void NewFrame() override;
	virtual void Render() override;

	// Clip-space capability
	virtual float GetProjectionYFlip() const override;

	void ExecuteTransientCommandBuffers();

	// Device memory helpers (Vulkan-only). Allocation is handled by VMA:
	// host-visible buffers are created with a persistent host-access flag and are
	// mapped explicitly through MapBuffer()/UnmapBuffer().
	BufferInfo CreateBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, bool bHostVisible);
	ImageInfo CreateImage(uint32_t width, uint32_t height, VkFormat imageFormat, VkImageUsageFlags usageFlags);

	void DestroyBuffer(BufferInfo& buffer);
	void DestroyImage(ImageInfo& image);

	void* MapBuffer(BufferInfo& buffer, size_t offset = 0);
	void UnmapBuffer(BufferInfo& buffer);

	// Copy
	CommandBufferVK* GetCopyCommandBuffer();
	void QueueCopyCommands(const BufferInfo& srcBuffer, const BufferInfo& dstBuffer, const OnCommandExecutionEnd& FinishDelegate = OnCommandExecutionEnd());

	CommandBufferVK* AllocateTransientCommandBuffer(CommandQueueType type);

	inline const VkInstance& GetVulkanInstance() { return VulkanInstance; }
	inline const VkPhysicalDevice& GetPhysicalDevice() { return PhysicalDevice; }
	inline const VkDevice& GetDevice() { return mDevice; }
	inline const VmaAllocator& GetAllocator() const { return mAllocator; }

	inline const CommandBufferVK* GetCommandBuffer() { return RenderCommandBuffers[FrameIdx]; }
	inline const VkClearValue& GetClearValue() { return ClearValue; }
	inline const uint32_t GetQueueFamilyIndex(CommandQueueType type) { return QueueFamilyData.QueueIndices[type]; }
	inline const CommandQueueVK* GetQueue(CommandQueueType type) { return CommandQueues[type]; }
	inline DescriptorPoolVK* GetDescriptorPool() { return RendererDescPool; }

	inline const RenderPassVK* GetScreenRenderPass() { return ScreenRenderPass; }

	// ImGui
	inline const RenderPassVK* GetImGuiRenderPass() { return ImGuiRenderPass; }
protected:
#if _DEBUG
	// Debug Messenger Callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif


protected:
	void CreateDescriptorPools();

	CullFace CullingFace;

	// Core
	std::vector<const char*> InstanceExtensions;
	std::vector<const char*> DeviceExtensions;
	VkInstance VulkanInstance;
	VkDebugUtilsMessengerEXT DebugMessenger;
	VkPhysicalDevice PhysicalDevice;
	VkDevice mDevice;
	VkSurfaceKHR Surface;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	VmaAllocator mAllocator;

	// Command Buffers, Queues and Pool
	CommandBufferPoolVK* CommandBufferPool;
	CommandBufferPoolVK* TransientTransferCmdBufferPool;
	CommandBufferPoolVK* TransientGraphicsCmdBufferPool;

	std::vector<CommandBufferVK*> RenderCommandBuffers;
	CommandBufferVK* TransferCommandBuffer;

	QueueFamilyIndices QueueFamilyData;
	CommandQueueVK* CommandQueues[CommandQueueType::Count];

	DescriptorPoolVK* RendererDescPool;

	// SwapChain
	SwapChainVK* SwapChain;
	std::vector<FrameBufferVK*> SwapchainFrameBuffers;

	std::vector<SemaphoreVK*> ImageAvailableSemaphores;
	std::vector<SemaphoreVK*> RenderingFinishedSemaphores;
	std::vector<FenceVK*> InFlightFences;

	// RenderPass
	RenderPassVK* ScreenRenderPass;
	VkClearValue ClearValue;

	// ImGui
	RenderPassVK* ImGuiRenderPass;
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
	CommandBufferVK* ActiveCopyCommandBuffer;
};
}
