/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#include "FrameBuffer_Vulkan.h"

#include "Core/Core.h"
#include "Rendering/RenderingFactory.h"
#include "Rendering/Vulkan/Texture_Vulkan.h"
#include "Rendering/Vulkan/Internal/RenderPass_Vulkan.h"

FrameBuffer_Vulkan::FrameBuffer_Vulkan() :
	mFrameBuffer(VK_NULL_HANDLE),
	mRenderPass(nullptr),
	mWidth(0), 
	mHeight(0), 
	mTexCount(0),
	bIsBackBuffer(false)
{

}

FrameBuffer_Vulkan::~FrameBuffer_Vulkan()
{
	std::for_each(m_Textures.begin(), m_Textures.end(),
				  [](Texture* x)
	{
		delete x;
	});
	Delete();
}

void FrameBuffer_Vulkan::Init(int width, int height, ImageFormat format, int Texcount)
{
	mWidth = width;
	mHeight = height;
	mTexCount = Texcount;

	Renderer_Vulkan* VkRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());
	if (VkRenderer)
	{
		mRenderPass = new RenderPass_Vulkan();
		mRenderPass->Init(Texture_Vulkan::GetVkFormat(format), true, false);

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

void FrameBuffer_Vulkan::CopyDepthTo(const FrameBuffer* target)
{
}

void FrameBuffer_Vulkan::TexBind(unsigned int index, unsigned int slot)
{
	if (index < m_Textures.size())
	{
		Renderer::Instance()->BindTexture(m_Textures[index], slot);
	}
}

void FrameBuffer_Vulkan::TexUnbind(unsigned int index,unsigned int slot)
{
	if (index < m_Textures.size())
	{
		Renderer::Instance()->UnbindTexture(m_Textures[index], slot);
	}
}

void FrameBuffer_Vulkan::Clear() const
{
}

void FrameBuffer_Vulkan::DepthBind()
{

}

void FrameBuffer_Vulkan::DepthUnbind()
{

}

void FrameBuffer_Vulkan::Delete() const
{
	vkDestroyFramebuffer(Renderer_Vulkan::Get()->GetDevice(), mFrameBuffer, nullptr);

	if (!bIsBackBuffer)
	{
		delete mRenderPass;
	}
}

void FrameBuffer_Vulkan::ChangeSize(unsigned int width, unsigned int height)
{
	mHeight = height;
	mWidth = width;
}

const Texture* FrameBuffer_Vulkan::GetTexture(unsigned int index)
{
	if (index < m_Textures.size())
	{
		return m_Textures[index];
	}
	return nullptr;
}

void FrameBuffer_Vulkan::InitAsBackBuffer(int16_t width, int16_t height, VkImageView ImageView)
{
	mWidth = width;
	mHeight = height;
	mTexCount = 1;
	bIsBackBuffer = true;

	Renderer_Vulkan* VkRenderer = static_cast<Renderer_Vulkan*>(Renderer::Instance());
	if (VkRenderer)
	{
		VkFramebufferCreateInfo FrameBufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		FrameBufferInfo.renderPass = VkRenderer->GetScreenRenderPass()->mRenderPass;
		FrameBufferInfo.attachmentCount = mTexCount;
		FrameBufferInfo.pAttachments = &ImageView;
		FrameBufferInfo.width = mWidth;
		FrameBufferInfo.height = mHeight;
		FrameBufferInfo.layers = 1;

		VKCall(vkCreateFramebuffer(VkRenderer->GetDevice(), &FrameBufferInfo, nullptr, &mFrameBuffer), "FrameBuffer creation Failed.");

		mRenderPass = const_cast<RenderPass_Vulkan*>(VkRenderer->GetScreenRenderPass());
	}
}
