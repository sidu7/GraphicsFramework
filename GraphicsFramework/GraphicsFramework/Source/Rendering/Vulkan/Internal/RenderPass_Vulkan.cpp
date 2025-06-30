#include "RenderPass_Vulkan.h"
#include "Rendering/Vulkan/Renderer_Vulkan.h"
#include "Rendering/Vulkan/FrameBuffer_Vulkan.h"
#include "Rendering/Vulkan/Internal/CommandBuffer_Vulkan.h"

RenderPass_Vulkan::RenderPass_Vulkan() :
	mRenderPass(VK_NULL_HANDLE)
{
}

RenderPass_Vulkan::~RenderPass_Vulkan()
{
	vkDestroyRenderPass(Renderer_Vulkan::Get()->GetDevice(), mRenderPass, nullptr);
}

void RenderPass_Vulkan::Init(VkFormat Format, bool bClearOnLoad, bool bPresentToScreen)
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

	VKCall(vkCreateRenderPass(Renderer_Vulkan::Get()->GetDevice(), &RenderPassInfo, nullptr, &mRenderPass), "ImGui RenderPass creation Failed.");
}

void RenderPass_Vulkan::BeginPass(const CommandBuffer_Vulkan* commandBuffer, const FrameBuffer_Vulkan* frameBuffer) const
{
	VkRenderPassBeginInfo RenderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	RenderPassBeginInfo.renderPass = mRenderPass;
	RenderPassBeginInfo.framebuffer = frameBuffer->mFrameBuffer;
	RenderPassBeginInfo.renderArea.extent.width = frameBuffer->GetWidth();
	RenderPassBeginInfo.renderArea.extent.height = frameBuffer->GetHeight();
	RenderPassBeginInfo.renderArea.offset = { 0, 0 };
	RenderPassBeginInfo.clearValueCount = 1;
	RenderPassBeginInfo.pClearValues = &Renderer_Vulkan::Get()->GetClearValue();

	vkCmdBeginRenderPass(commandBuffer->mCommandBuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass_Vulkan::EndPass(const CommandBuffer_Vulkan* commandBuffer) const
{
	vkCmdEndRenderPass(commandBuffer->mCommandBuffer);
}
