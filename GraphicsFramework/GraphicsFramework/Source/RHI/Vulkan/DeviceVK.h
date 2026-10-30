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

#include <unordered_map>

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
class ShaderVK;
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

	// Wait for the GPU to finish the previous frame (used by GPU profiler to
	// ensure query results are available before reading them).
	void WaitForPreviousFrame();

	// Wait for a specific frame's fence (by index) — used by GPU profiler to
	// wait on the fence that was actually used for the previous frame.
	void WaitForFenceAt(uint32_t frameIdx);

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

	// Vulkan clears through the render pass load op, so the colour lives in the
	// clear-value array rather than in Renderer::mClearColor.
	virtual void SetClearColor(glm::vec3 color) override;

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

	// Makes CPU writes to a mapped host-visible buffer visible to the GPU.
	// A no-op on coherent memory, required on non-coherent heaps.
	void FlushBuffer(const BufferInfo& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

	// Copy
	CommandBufferVK* GetCopyCommandBuffer();
	void QueueCopyCommands(const BufferInfo& srcBuffer, const BufferInfo& dstBuffer, const OnCommandExecutionEnd& FinishDelegate = OnCommandExecutionEnd());
	
	// Schedule a cleanup callback to run after the current copy command buffer finishes execution
	void ScheduleCopyCommandCleanup(const OnCommandExecutionEnd& FinishDelegate);

	CommandBufferVK* AllocateTransientCommandBuffer(CommandQueueType type);

	inline const VkInstance& GetVulkanInstance() { return VulkanInstance; }
	inline const VkPhysicalDevice& GetPhysicalDevice() { return PhysicalDevice; }
	inline const VkDevice& GetDevice() { return mDevice; }
	inline const VmaAllocator& GetAllocator() const { return mAllocator; }

	inline const CommandBufferVK* GetCommandBuffer() { return RenderCommandBuffers[FrameIdx]; }
	inline const VkClearValue* GetClearValues() const { return ClearValues; }
	inline const uint32_t GetQueueFamilyIndex(CommandQueueType type) { return QueueFamilyData.QueueIndices[type]; }
	inline const CommandQueueVK* GetQueue(CommandQueueType type) { return CommandQueues[type]; }
	inline DescriptorPoolVK* GetDescriptorPool() { return RendererDescPool; }

	inline const RenderPassVK* GetScreenRenderPass() { return ScreenRenderPass; }

	// ImGui
	inline const RenderPassVK* GetImGuiRenderPass() { return ImGuiRenderPass; }

	// Frame index (used by GPU profiler to determine the previous frame's fence)
	inline uint32_t GetCurrentFrameIndex() const { return FrameIdx; }
protected:
#if _DEBUG
	// Debug Messenger Callback
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity, VkDebugUtilsMessageTypeFlagsEXT MessageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
#endif


protected:
	void CreateDescriptorPools();
	void CreateFrameDescriptorPools();

	// Allocates a fresh descriptor set for the current draw and fills it with the
	// currently bound uniform buffers/textures. Returns VK_NULL_HANDLE when the
	// shader has no descriptors.
	VkDescriptorSet AllocateDrawDescriptorSet(const ShaderVK* shader);

	// Records the shared dynamic state for a debug-line draw and binds the
	// shader's line pipeline (Vulkan only allows a topology change within the
	// same topology class).
	void BeginDebugDraw(const Shader* shader, VkPrimitiveTopology topology);

	CullFace CullingFace;

	// Bind state. Vulkan descriptors are written when a draw is recorded, not
	// when BindUniformBuffer/BindTexture is called, so the bindings are kept here.
	std::unordered_map<uint32_t, const UniformBuffer*> mBoundUniformBuffers;
	std::unordered_map<uint32_t, const Texture*> mBoundTextures;

	// Descriptor pools reset once per frame; descriptor sets are transient.
	std::vector<VkDescriptorPool> FrameDescriptorPools;

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
	//
	// The screen render pass stays open for the whole 3D portion of a frame so
	// that every draw accumulates into the same swapchain image (the pass is
	// only cleared once, when it is begun in StartFrame). It is closed when
	// ImGui renders (a separate pass that presents to screen) or at the end of
	// the frame.
	void BeginScreenPass();
	void EndScreenPass();

	RenderPassVK* ScreenRenderPass;
	bool mScreenPassOpen = false;
	bool mPresentedToScreen = false;

	// One depth buffer per swapchain image, owned here because the framebuffers
	// only reference the views.
	std::vector<ImageInfo> DepthImages;
	std::vector<VkImageView> DepthImageViews;

	// [0] colour, [1] depth
	VkClearValue ClearValues[2];

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

	// Currently bound shader (for descriptor set writes)
	const Shader* mBoundShader;
};
}
