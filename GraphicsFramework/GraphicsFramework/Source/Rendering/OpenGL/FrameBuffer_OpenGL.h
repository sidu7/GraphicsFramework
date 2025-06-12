/* Start Header -------------------------------------------------------
Copyright (C) 2018-2019 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents without the prior
written consent of DigiPen Institute of Technology is prohibited.
Author: Sidhant Tumma
- End Header --------------------------------------------------------*/

#pragma once
#include "Rendering/FrameBuffer.h"

class FrameBuffer_OpenGL : public FrameBuffer
{
public:
	FrameBuffer_OpenGL();
	virtual ~FrameBuffer_OpenGL();

	virtual void Init(int width, int height, int Texcount = 1) override;

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

	std::vector<Texture*> m_Textures;
	unsigned int mWidth, mHeight, mDepthBuffer;
	unsigned int mTexCount;

	unsigned int m_RendererID;
};
