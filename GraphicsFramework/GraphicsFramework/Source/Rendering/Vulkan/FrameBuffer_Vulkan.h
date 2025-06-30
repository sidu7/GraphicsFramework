/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Rendering/FrameBuffer.h"
#include "Rendering/Vulkan/Renderer_Vulkan.h"

class RenderPass_Vulkan;
class Texture_Vulkan;

class FrameBuffer_Vulkan : public FrameBuffer
{
public:
	FrameBuffer_Vulkan();
	virtual ~FrameBuffer_Vulkan();

	virtual void Init(int width, int height, ImageFormat format, int Texcount = 1) override;

	virtual void CopyDepthTo(const FrameBuffer* target) override;
	virtual void TexBind(unsigned int index = 0, unsigned int slot = 0) override;
	virtual void TexUnbind(unsigned int index, unsigned int slot) override;
	virtual void Clear() const override;
	virtual void DepthBind() override;
	virtual void DepthUnbind() override;
	virtual void Delete() const override;
	virtual void ChangeSize(unsigned int width, unsigned int height) override;
	virtual const Texture* GetTexture(unsigned int index = 0) override;

	virtual inline int GetWidth() const override { return mWidth; }
	virtual inline int GetHeight() const override { return mHeight; }

	void InitAsBackBuffer(int16_t width, int16_t height, VkImageView ImageView);

	std::vector<Texture_Vulkan*> m_Textures;
	unsigned int mWidth, mHeight;
	unsigned int mTexCount;

	VkFramebuffer mFrameBuffer;
	RenderPass_Vulkan* mRenderPass;

protected:
	bool bIsBackBuffer;
};
