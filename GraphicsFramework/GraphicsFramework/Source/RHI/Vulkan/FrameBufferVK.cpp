/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "FrameBufferVK.h"

#include "Core/Core.h"
#include "RHI/Vulkan/TextureVK.h"
#include "RHI/Vulkan/Internal/RenderPassVK.h"

namespace rhi::vk
{
FrameBufferVK::FrameBufferVK() :
	mFrameBuffer(VK_NULL_HANDLE),
	mRenderPass(nullptr),
	mWidth(0), 
	mHeight(0), 
	mTexCount(0),
	bIsBackBuffer(false),
	mHasDepth(false)
{

}

FrameBufferVK::~FrameBufferVK()
{
	std::for_each(m_Textures.begin(), m_Textures.end(),
				  [](Texture* x)
	{
		delete x;
	});
	Delete();
}

void FrameBufferVK::Init(int width, int height, ImageFormat format, int Texcount)
{
	mWidth = width;
	mHeight = height;
	mTexCount = Texcount;

	Device* VkRenderer = static_cast<Device*>(Renderer::Instance());
	if (VkRenderer)
	{
		mRenderPass = new RenderPassVK();
		mRenderPass->Init(TextureVK::GetVkFormat(format), true, false);

		VkImageView ImageView;
		VkFramebufferCreateInfo FrameBufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FrameBufferInfo.renderPass = mRenderPass->mRenderPass;
		FrameBufferInfo.attachmentCount = mTexCount;
		FrameBufferInfo.pAttachments = &ImageView;
		FrameBufferInfo.width = mWidth;
		FrameBufferInfo.height = mHeight;
		FrameBufferInfo.layers = 1;

		VKCall(vkCreateFramebuffer(VkRenderer->GetDevice(), &FrameBufferInfo, nullptr, &mFrameBuffer), "FrameBuffer creation Failed.");
	}
}

void FrameBufferVK::CopyDepthTo(const FrameBuffer* target)
{
}

void FrameBufferVK::Clear() const
{
}

void FrameBufferVK::DepthBind()
{

}

void FrameBufferVK::DepthUnbind()
{

}

void FrameBufferVK::Delete() const
{
	vkDestroyFramebuffer(Device::Get()->GetDevice(), mFrameBuffer, nullptr);

	if (!bIsBackBuffer)
	{
		delete mRenderPass;
	}
}

void FrameBufferVK::ChangeSize(unsigned int width, unsigned int height)
{
	mHeight = height;
	mWidth = width;
}

const Texture* FrameBufferVK::GetTexture(unsigned int index)
{
	if (index < m_Textures.size())
	{
		return m_Textures[index];
	}
	return nullptr;
}

void FrameBufferVK::InitAsBackBuffer(int16_t width, int16_t height, VkImageView ImageView, VkImageView DepthView)
{
	mWidth = width;
	mHeight = height;
	mTexCount = 1;
	bIsBackBuffer = true;
	mHasDepth = DepthView != VK_NULL_HANDLE;

	Device* VkRenderer = static_cast<Device*>(Renderer::Instance());
	if (VkRenderer)
	{
		const VkImageView Attachments[2] = { ImageView, DepthView };

		VkFramebufferCreateInfo FrameBufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FrameBufferInfo.renderPass = VkRenderer->GetScreenRenderPass()->mRenderPass;
		FrameBufferInfo.attachmentCount = mHasDepth ? 2 : 1;
		FrameBufferInfo.pAttachments = Attachments;
		FrameBufferInfo.width = mWidth;
		FrameBufferInfo.height = mHeight;
		FrameBufferInfo.layers = 1;

		VKCall(vkCreateFramebuffer(VkRenderer->GetDevice(), &FrameBufferInfo, nullptr, &mFrameBuffer), "FrameBuffer creation Failed.");

		mRenderPass = const_cast<RenderPassVK*>(VkRenderer->GetScreenRenderPass());
	}
}
}
