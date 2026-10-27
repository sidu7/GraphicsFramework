#include "RenderPassVK.h"
#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/FrameBufferVK.h"
#include "RHI/Vulkan/Internal/CommandBufferVK.h"

namespace rhi::vk
{
RenderPassVK::RenderPassVK() :
	mRenderPass(VK_NULL_HANDLE),
	mHasDepth(false)
{
}

RenderPassVK::~RenderPassVK()
{
	vkDestroyRenderPass(Device::Get()->GetDevice(), mRenderPass, nullptr);
}

void RenderPassVK::Init(VkFormat Format, bool bClearOnLoad, bool bPresentToScreen, bool bWithDepth)
{
	mHasDepth = bWithDepth;

	VkAttachmentDescription Attachments[2] = {};

	VkAttachmentDescription& ColorAttachment = Attachments[0];
	ColorAttachment.format = Format;
	ColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	ColorAttachment.loadOp = bClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	ColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	ColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	ColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	ColorAttachment.initialLayout = bClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	ColorAttachment.finalLayout = bPresentToScreen ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference AttachmentRef = {};
	AttachmentRef.attachment = 0;
	AttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	uint32_t AttachmentCount = 1;
	VkAttachmentReference DepthRef = {};
	DepthRef.attachment = 1;
	DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	if (bWithDepth)
	{
		// The contents of the depth buffer are only needed inside a single pass,
		// so it is always discarded at the start and never stored.
		VkAttachmentDescription& DepthAttachment = Attachments[1];
		DepthAttachment.format = VK_FORMAT_D32_SFLOAT;
		DepthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		DepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		DepthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		AttachmentCount = 2;
	}

	VkSubpassDescription SubpassDesc = {};
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &AttachmentRef;
	if (bWithDepth)
	{
		SubpassDesc.pDepthStencilAttachment = &DepthRef;
	}

	VkSubpassDependency SubpassDependency = {};
	SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependency.dstSubpass = 0;
	SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.srcAccessMask = 0;
	SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo RenderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	RenderPassInfo.attachmentCount = AttachmentCount;
	RenderPassInfo.pAttachments = Attachments;
	RenderPassInfo.subpassCount = 1;
	RenderPassInfo.pSubpasses = &SubpassDesc;
	/*RenderPassInfo.dependencyCount = 1;
	RenderPassInfo.pDependencies = &SubpassDependency;*/

	VKCall(vkCreateRenderPass(Device::Get()->GetDevice(), &RenderPassInfo, nullptr, &mRenderPass), "ImGui RenderPass creation Failed.");
}

void RenderPassVK::BeginPass(const CommandBufferVK* commandBuffer, const FrameBufferVK* frameBuffer) const
{
	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = mRenderPass;
	RenderPassBeginInfo.framebuffer = frameBuffer->mFrameBuffer;
	RenderPassBeginInfo.renderArea.extent.width = frameBuffer->GetWidth();
	RenderPassBeginInfo.renderArea.extent.height = frameBuffer->GetHeight();
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.clearValueCount = mHasDepth ? 2 : 1;
	RenderPassBeginInfo.pClearValues = Device::Get()->GetClearValues();

	vkCmdBeginRenderPass(commandBuffer->mCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPassVK::EndPass(const CommandBufferVK* commandBuffer) const
{
	vkCmdEndRenderPass(commandBuffer->mCommandBuffer);
}
}
