#pragma once
#include "Core/Core.h"
#include "RHI/Vulkan/DeviceVK.h"

namespace rhi::vk
{
class CommandBufferVK;
class FrameBufferVK;
class RenderPassVK
{
public:
	RenderPassVK();
	~RenderPassVK();

	void Init(VkFormat Format, bool bClearOnLoad, bool bPresentToScreen, bool bWithDepth = false);
	void BeginPass(const CommandBufferVK* commandBuffer, const FrameBufferVK* frameBuffer) const;
	void EndPass(const CommandBufferVK* commandBuffer) const;

	VkRenderPass mRenderPass;

private:
	bool mHasDepth;
};
}
