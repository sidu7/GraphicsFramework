#pragma once
#include "Core/Core.h"
#include "Rendering/Vulkan/Renderer_Vulkan.h"

class CommandBuffer_Vulkan;
class FrameBuffer_Vulkan;

class RenderPass_Vulkan
{
public:
	RenderPass_Vulkan();
	~RenderPass_Vulkan();

	void Init(VkFormat Format, bool bClearOnLoad, bool bPresentToScreen);
	void BeginPass(const CommandBuffer_Vulkan* commandBuffer, const FrameBuffer_Vulkan* frameBuffer) const;
	void EndPass(const CommandBuffer_Vulkan* commandBuffer) const;

	VkRenderPass mRenderPass;
};