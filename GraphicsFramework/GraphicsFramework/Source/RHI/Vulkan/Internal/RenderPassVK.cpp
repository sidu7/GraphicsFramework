#include "RenderPassVK.h"
#include "RHI/Vulkan/DeviceVK.h"
#include "RHI/Vulkan/FrameBufferVK.h"
#include "RHI/Vulkan/Internal/CommandBufferVK.h"

namespace rhi::vk
{
RenderPassVK::RenderPassVK() :
	mRenderPass(VK_NULL_HANDLE)
{
}

RenderPassVK::~RenderPassVK()
{
	vkDestroyRenderPass(Device::Get()->GetDevice(), mRenderPass, nullptr);
}

void RenderPassVK::Init(VkFormat Format, bool bClearOnLoad, bool bPresentToScreen)
{
	VkAttachmentDescription AttachmentDesc = {};
	AttachmentDesc.format = Format;
	AttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	AttachmentDesc.loadOp = bClearOnLoad ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
	AttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	AttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	AttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	AttachmentDesc.initialLayout = bClearOnLoad ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	AttachmentDesc.finalLayout = bPresentToScreen ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference AttachmentRef = {};
	AttachmentRef.attachment = 0;
	AttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription SubpassDesc = {};
	SubpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	SubpassDesc.colorAttachmentCount = 1;
	SubpassDesc.pColorAttachments = &AttachmentRef;

	VkSubpassDependency SubpassDependency = {};
	SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	SubpassDependency.dstSubpass = 0;
	SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	SubpassDependency.srcAccessMask = 0;
	SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo RenderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	RenderPassInfo.attachmentCount = 1;
	RenderPassInfo.pAttachments = &AttachmentDesc;
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
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &Device::Get()->GetClearValue();

	vkCmdBeginRenderPass(commandBuffer->mCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPassVK::EndPass(const CommandBufferVK* commandBuffer) const
{
	vkCmdEndRenderPass(commandBuffer->mCommandBuffer);
}
}
